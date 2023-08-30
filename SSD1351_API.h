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
#ifndef _SSD1351_API_H_
#define _SSD1351_API_H_

#include <stdint.h>

// SSD1351 128 x 128 OLED RGB display
constexpr int DISPLAY_WIDTH = 128;
constexpr int DISPLAY_HEIGHT = 128;

constexpr uint16_t BLACK = 0x0000;
constexpr uint16_t WHITE = 0xFFFF;


void SSD1351_init(void);
void SSD1351_clear(void);
void SSD1351_pixel(int16_t x, int16_t y, uint16_t color);
void SSD1351_fillrect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
int SSD1351_char(int16_t x, int16_t y, int8_t c, uint16_t color);
int SSD1351_charwidth(int8_t c);
int SSD1351_text(int16_t x, int16_t y, char *s, uint16_t color);
int SSD1351_textwidth(char *s);
void SSD1351_update(void);
uint16_t SSD1351_color(uint8_t r, uint8_t g, uint8_t b);

#endif