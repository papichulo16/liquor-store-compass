#include "main.h"
#include "project.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "math.h"

#define GY271M_ADDR (0x1E << 1)
#define GY271M_ID_REG_A 10
#define GY271M_MODE_REG   0x02
#define GY271M_DATA_REG   0x03
#define GY271M_CONTINUOUS 0x00

#define OFFSET_DEG  0.0f  

bool gy271m_verify(I2C_HandleTypeDef* i2c) {

    uint8_t regAddr = GY271M_ID_REG_A;
    uint8_t data[4] = {0};

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        i2c,         
        GY271M_ADDR,  
        &regAddr,       
        1,              
        HAL_MAX_DELAY   
    );

    if (status != HAL_OK)
        return false;

    status = HAL_I2C_Master_Receive(
        i2c,        
        GY271M_ADDR,  
        data,         
        3,            
        HAL_MAX_DELAY 
    );

    if (status != HAL_OK)
        return false;

    if (data[0] == 'H' && data[1] == '4' && data[2] == '3')
        return true;

    return false;
}

bool gy271m_init(I2C_HandleTypeDef* i2c)
{
    uint8_t config[2] = { GY271M_MODE_REG, GY271M_CONTINUOUS };

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        i2c,
        GY271M_ADDR,
        config,
        2,
        HAL_MAX_DELAY
    );

    return (status == HAL_OK);
}

bool gy271m_read(I2C_HandleTypeDef* i2c, int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t regAddr = GY271M_DATA_REG;
    uint8_t data[6] = {0};

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        i2c,
        GY271M_ADDR,
        &regAddr,
        1,
        HAL_MAX_DELAY
    );
    if (status != HAL_OK) return false;

    status = HAL_I2C_Master_Receive(
        i2c,
        GY271M_ADDR,
        data,
        6,
        HAL_MAX_DELAY
    );
    if (status != HAL_OK) return false;

    *x = (int16_t)(data[0] << 8 | data[1]);
    *z = (int16_t)(data[2] << 8 | data[3]);  
    *y = (int16_t)(data[4] << 8 | data[5]);

    return true;
}

float gy271m_heading(int16_t x, int16_t y, int16_t z) {
    (void)z;

    float x_f = (float)x - (-179.0f);
    float y_f = (float)y - (-1812.0f);

    x_f /= 301.0f;
    y_f /= 257.0f;

    float heading = atan2f(x_f, -y_f) * (180.0f / M_PI);

    heading += OFFSET_DEG;

    if (heading < 0)
        heading += 360.0f;
    if (heading >= 360.0f)
        heading -= 360.0f;

    return heading;
}

