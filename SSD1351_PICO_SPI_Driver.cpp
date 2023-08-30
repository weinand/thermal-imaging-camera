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
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "SSD1351_API.h"

// configure the OLED SSD1351 on the SPI bus 1 (SCK: 10, MOSI: 11, DC: 9, CS: 13, RST: 15)
#define PIN_SCK   10
#define PIN_MOSI  11
#define PIN_CS    13
#define PIN_DC    9
#define PIN_RST   15

#define SPI_PORT spi1

void SSD1351_SPIInit(void) {
    
    spi_init(SPI_PORT, 10 * 1000 * 1000);        // 10 MHz
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_put(PIN_DC, 0);

    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_put(PIN_RST, 1);

    gpio_put(PIN_DC, 1);

    gpio_put(PIN_RST, 1);
    sleep_ms(50);
    gpio_put(PIN_RST, 0);
    sleep_ms(50);

    gpio_put(PIN_RST, 1);
    gpio_put(PIN_DC, 0);
    // sleep_ms(50);

    SSD1351_init();
}

void SSD1351_write_command(uint8_t cmd, const char* data, uint16_t len)
{
    gpio_put(PIN_DC, 0);
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
    if (data) {
        gpio_put(PIN_DC, 1);
        gpio_put(PIN_CS, 0);
        spi_write_blocking(SPI_PORT, (const uint8_t*) data, len);
        gpio_put(PIN_CS, 1);
    }
}
