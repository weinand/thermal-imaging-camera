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
#ifndef _SSD1351_SPI_Driver_H_
#define _SSD1351_SPI_Driver_H_

#include <stdint.h>
#include "SSD1351_API.h"

void SSD1351_SPIInit(void);
void SSD1351_write_command(uint8_t cmd, const char* data, uint16_t len);

#endif