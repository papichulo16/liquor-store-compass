#include "main.h"

#include <stdbool.h>
#include <stdint.h>

bool gy271m_verify(I2C_HandleTypeDef* i2c);
bool gy271m_init(I2C_HandleTypeDef* i2c);
bool gy271m_read(I2C_HandleTypeDef* i2c, int16_t* x, int16_t* y, int16_t* z);
float gy271m_heading(int16_t x, int16_t y, int16_t z);

