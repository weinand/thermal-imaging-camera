/**
 * Copyright 2023 André Weinand
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cstdio>
#include <ctime>
#include <math.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"

extern "C"{
#include <MLX90640_I2C_Driver.h>
#include <MLX90640_API.h>
}

#include "SSD1351_SPI_DRIVER.h"
#include "SSD1351_API.h"

// ---- configuration ----

// MLX90640 32 x 24 Thermopile Array
constexpr uint8_t REFRESH_RATE = 6;             // 0: 0.5 Hz, 1: 1 Hz, 2: 2 Hz, 3: 4 Hz, 4: 8 Hz, 5: 16 Hz, 6: 32 Hz, 7: 64 Hz
constexpr float EMISSIVITY = 0.95;              // the emissivity of the measured object (1.0 = black body)
constexpr float OPENAIR_TA_SHIFT = -8.0;        // for a MLX90640 in the open air the shift is -8 deg Celsius

// SSD1351 128 x 128 OLED Display
constexpr uint16_t HEAT_MAP_SIZE = 256;         // the number of colors in the heat map (must be <= 256)
constexpr bool BILINEAR_INTERPOLATION = true;   // if true use bilinear interpolation and nearest neighbor interpolation otherwise

#define FLIP_GRAPH_HORIZONTAL 1
#define FLIP_GRAPH_VERTICAL 0

// ---- other constants (don't change) ----

constexpr uint8_t MLX_I2C_ADDR = 0x33;          // I2C address of the MLX90640

constexpr uint8_t PIXEL_SIZE = DISPLAY_WIDTH / MLX90640_COLUMN_NUM;     // width and height of a single thermal camera pixel on the OLED display
constexpr uint16_t GRAPH_WIDTH = MLX90640_COLUMN_NUM * PIXEL_SIZE;      // width of the thermal camera graph on the OLED display
constexpr uint16_t GRAPH_HEIGHT = MLX90640_LINE_NUM * PIXEL_SIZE;       // height of the thermal camera graph on the OLED display

// ---- implementation ----

// handle touch button interrupts
static bool bilinear_interpolation = BILINEAR_INTERPOLATION;
static bool freeze_image = false;

void gpio_callback(uint gpio, uint32_t events) {
    switch (gpio) {
        case 14:    // touch button disables bilinear interpolation
            if (events & GPIO_IRQ_EDGE_RISE) {
                bilinear_interpolation = false;
            }
            if (events & GPIO_IRQ_EDGE_FALL) {
                bilinear_interpolation = true;
            }
            break;
        case 18:   // touch button "freeze image"
            if (events & GPIO_IRQ_EDGE_RISE) {
                freeze_image = true;
            }
            if (events & GPIO_IRQ_EDGE_FALL) {
                freeze_image = false;
            }
            break;
    }
}

// measure display frame rate with a timer interrupt
static repeating_timer_t timer;
static int frame_cnt = 0;
static int fps = 0;

bool timer_callback(repeating_timer_t *rt)
{   
    fps = frame_cnt;
    frame_cnt = 0;
    return true;
}

// a "heat" color map inspired by: http://www.andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients

typedef struct {
    uint8_t r, g, b;
} RGBColor;

#define RGB_BLACK {0, 0, 0} 
#define RGB_RED {255, 38, 0} 
#define RGB_YELLOW {255, 255, 0} 
#define RGB_GREEN {0, 249, 0} 
#define RGB_BLUE {4, 51, 255}
#define RGB_ORANGE {255, 147, 0}
#define RGB_CYAN {0, 253, 255}
#define RGB_MAGENTA {255, 64, 255}
#define RGB_PURPLE {148, 33, 146}
#define RGB_WHITE {255, 255, 255} 

constexpr RGBColor colors[] = {RGB_BLACK, RGB_BLUE, RGB_CYAN, RGB_GREEN, RGB_YELLOW, RGB_RED, RGB_WHITE};
//constexpr RGBColor colors[] = {RGB_BLUE, RGB_CYAN, RGB_GREEN, RGB_YELLOW, RGB_RED};
//constexpr RGBColor colors[] = {RGB_BLUE, RGB_RED};
//constexpr RGBColor colors[] = {RGB_VIOLET, RGB_ORANGE};
//constexpr RGBColor colors[] = {RGB_BLACK, RGB_WHITE};

static uint16_t palette[HEAT_MAP_SIZE];

void heatmap_init() {
    constexpr int numColors = sizeof(colors) / sizeof(RGBColor);
    for (int c = 0; c < HEAT_MAP_SIZE; c++) {
        float value = c * (numColors-1) / float(HEAT_MAP_SIZE-1);
        int idx1 = int(value);
        uint8_t red, green, blue;
        if (idx1 == value) {
            red = colors[idx1].r;
            green = colors[idx1].g;
            blue = colors[idx1].b;
        } else {
            int idx2 = idx1 + 1;
            float fractBetween = value - idx1;
            red = int(round((colors[idx2].r - colors[idx1].r) * fractBetween + colors[idx1].r));
            green = int(round((colors[idx2].g - colors[idx1].g) * fractBetween + colors[idx1].g));
            blue = int(round((colors[idx2].b - colors[idx1].b) * fractBetween + colors[idx1].b));
        }
        palette[c] = SSD1351_color(red, green, blue);
    }
}

#if FLIP_GRAPH_HORIZONTAL
#define TR_X(x) (GRAPH_WIDTH-1-(x))
#else
#define TR_X(x) (x)
#endif
#if FLIP_GRAPH_VERTICAL
#define TR_Y(y) (GRAPH_HEIGHT-1-(y))
#else
#define TR_Y(y) (y)
#endif

typedef struct {
    float min, max;                     // min and max temperature values
    uint8_t values[MLX90640_PIXEL_NUM]; // temperature values from [min .. max] scaled to [0 .. HEAT_MAP_SIZE-1]

    inline uint8_t value(int16_t x, int16_t y) {
        return values[(y << 5) + x];
    }
} FrameDTO;

void renderer() {

    SSD1351_SPIInit();

    heatmap_init();

    for (int x = 0; x < DISPLAY_WIDTH; x++) {
        int l = (x * HEAT_MAP_SIZE) / DISPLAY_WIDTH;
        SSD1351_fillrect(x, 112, 1, 16, palette[l]);
    }

    add_repeating_timer_ms(1000, &timer_callback, NULL, &timer);

    gpio_set_irq_enabled(14, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(18, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_callback(&gpio_callback);
    irq_set_enabled(IO_IRQ_BANK0, true);

    // create a single DTO for the communication between main thread (core0) and renderer thread (core1)
    multicore_fifo_push_blocking((uint32_t)new FrameDTO());

    // process DTOs received from core0
    while (true) {

        // wait for a frame DTO received from core0
        FrameDTO *dto = (FrameDTO*) multicore_fifo_pop_blocking();

        if (freeze_image) {
            // just send back the DTO without rendering it
            multicore_fifo_push_blocking((uint32_t)dto);
            continue;
        }

        if (bilinear_interpolation) {
            // integer bilinear interpolation
            int32_t mx = ((MLX90640_COLUMN_NUM - 1) << 7) / GRAPH_WIDTH;
            int32_t my = ((MLX90640_LINE_NUM - 1) << 7) / GRAPH_HEIGHT;

            for (int y = 0; y < GRAPH_HEIGHT; y++) {
                
                int32_t y0 = (y * my) >> 7;
                int32_t ty = (y * my) & 0x7f;
 
                for (int x = 0; x < GRAPH_WIDTH; x++) {

                    int32_t x0 = (x * mx) >> 7;
                    int32_t tx = (x * mx) & 0x7f;

                    int16_t v00 = dto->value(x0,   y0);
                    int16_t v10 = dto->value(x0+1, y0);
                    int16_t v01 = dto->value(x0,   y0+1);
                    int16_t v11 = dto->value(x0+1, y0+1);

                    int32_t s = v00 + ((tx * (v10-v00)) >> 7);
                    int32_t e = v01 + ((tx * (v11-v01)) >> 7);
                    int32_t v = s + ((ty * (e-s)) >> 7);
          
                    SSD1351_pixel(TR_X(x), TR_Y(y), palette[v]);
                }
            }
        } else {
            // nearest neighbor interpolation
            for (int y = 0; y < MLX90640_LINE_NUM; y++) {
                for (int x = 0; x < MLX90640_COLUMN_NUM; x++) {
                    SSD1351_fillrect(TR_X(x*PIXEL_SIZE), TR_Y(y*PIXEL_SIZE), PIXEL_SIZE, PIXEL_SIZE, palette[dto->value(x, y)]);
                }
            }
        }

        float min = dto->min;
        float max = dto->max;

        // send frame back to core0
        multicore_fifo_push_blocking((uint32_t)dto);

        int y = GRAPH_HEIGHT;
        SSD1351_fillrect(0, y, DISPLAY_WIDTH, 16, BLACK);

        y += 4;
        char buf[32];
        sprintf(buf, "%.0f", min);
        SSD1351_text(1, y, buf, WHITE);

        sprintf(buf, "%d fps", fps);
        int w = SSD1351_textwidth(buf);
        SSD1351_text((DISPLAY_WIDTH-w) / 2, y, buf, WHITE);

        sprintf(buf, "%.0f", max);
        w = SSD1351_textwidth(buf);
        SSD1351_text(DISPLAY_WIDTH-w-1, y, buf, WHITE);

        SSD1351_update();

        frame_cnt++;
    }
}

int main() {

    stdio_init_all();

    multicore_launch_core1(renderer);

    sleep_ms(40 + 500); // after Power-On wait a bit for the MLX90640 to initialize

    MLX90640_I2CInit();
    MLX90640_SetResolution(MLX_I2C_ADDR, 3);    // 0: 16 bit, 1: 17 bit, 2 = 18 bit, 3 = 19 bit
    MLX90640_SetRefreshRate(MLX_I2C_ADDR, REFRESH_RATE);
    MLX90640_SetChessMode(MLX_I2C_ADDR);

    uint16_t *eeMLX90640 = new uint16_t[832];       // too large for allocating on stack 
    MLX90640_DumpEE(MLX_I2C_ADDR, eeMLX90640);
    paramsMLX90640 *params = new paramsMLX90640;    // too large for allocating on stack 
    MLX90640_ExtractParameters(eeMLX90640, params);
    delete eeMLX90640;

    uint16_t *captureFrame = new uint16_t[834];     // too large for allocating on stack 
    float *values = new float[MLX90640_PIXEL_NUM];  // too large for allocating on stack 
    int patternMode = MLX90640_GetCurMode(MLX_I2C_ADDR);

    while (true) {

        // read pages (half frames) from the MLX90640
        int status = MLX90640_GetFrameData(MLX_I2C_ADDR, captureFrame);
        if (status < 0) {
            printf("Error: MLX90640_GetFrameData returned %d\n", status);
            continue;   // skip this frame
        }
        float eTa = MLX90640_GetTa(captureFrame, params) + OPENAIR_TA_SHIFT;
        MLX90640_CalculateTo(captureFrame, params, EMISSIVITY, eTa, values);
        MLX90640_BadPixelsCorrection(params->brokenPixels, values, patternMode, params);
        MLX90640_BadPixelsCorrection(params->outlierPixels, values, patternMode, params);

        // find min and max temperature values of the frame
        float min, max;
        min = max = values[0];
        for (int i = 1; i < MLX90640_PIXEL_NUM; i++) {
            float value = values[i];
            if (value > max)
                max = value;
            if (value < min)
                min = value;
        }

        // create a DTO for the renderer
        FrameDTO *dto = (FrameDTO*)multicore_fifo_pop_blocking();
        dto->min = min;
        dto->max = max;

        float step = (ceil(max + 1.0) - floor(min - 1.0)) / float(HEAT_MAP_SIZE-1);
        for (int i = 0; i < MLX90640_PIXEL_NUM; i++) {
            dto->values[i] = uint8_t((values[i] - min) / step);
        }
        multicore_fifo_push_blocking((uint32_t)dto);
    }

    delete values;
    delete captureFrame;
    delete params;
    return 0;
}
