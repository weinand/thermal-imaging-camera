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
#include "hardware/i2c.h"
#include "pico/stdlib.h"

extern "C"{
#include <MLX90640_I2C_Driver.h>
}

// configure the Thermopile MLX90640 on the I2C bus 0 (SCL: 17, SDA: 16)

#define PIN_I2C_SDA 16
#define PIN_I2C_SCL 17

#define I2C_PORT i2c0


void MLX90640_I2CInit() {
    i2c_init(I2C_PORT, 1000 * 1000);     // 1 MHz
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
}

void MLX90640_I2CFreqSet(int freq) {
}

int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data) {
    uint8_t cmd[2];

    cmd[0] = startAddress >> 8;
    cmd[1] = startAddress & 0x00FF;

    i2c_write_blocking(I2C_PORT, slaveAddr, cmd, 2, true);
    i2c_read_blocking(I2C_PORT, slaveAddr, (uint8_t*) data, 2*nMemAddressRead, false);

    for (int i = 0; i < nMemAddressRead; i++) {
        data[i] = __builtin_bswap16(data[i]);
    }
    return 0;
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data) {
    uint8_t cmd[4];

    cmd[0] = writeAddress >> 8;
    cmd[1] = writeAddress & 0x00FF;
    cmd[2] = data >> 8;
    cmd[3] = data & 0x00FF;

    i2c_write_blocking(I2C_PORT, slaveAddr, cmd, 4, false);
    return 0;
}

int MLX90640_I2CGeneralReset(void) {
    return 0;
}