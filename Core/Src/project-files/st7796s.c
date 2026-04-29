/* vim: set ai et ts=4 sw=4: */
#include "main.h"
#include "project.h"

static void ST7796S_Select() {
    HAL_GPIO_WritePin(ST7796S_CS_GPIO_Port, ST7796S_CS_Pin, GPIO_PIN_RESET);
}

void ST7796S_Unselect() {
    HAL_GPIO_WritePin(ST7796S_CS_GPIO_Port, ST7796S_CS_Pin, GPIO_PIN_SET);
}

static void ST7796S_Reset() {
    HAL_GPIO_WritePin(ST7796S_RES_GPIO_Port, ST7796S_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ST7796S_RES_GPIO_Port, ST7796S_RES_Pin, GPIO_PIN_SET);
}

static void ST7796S_WriteCommand(SPI_HandleTypeDef* spi, uint8_t cmd) {
    HAL_GPIO_WritePin(ST7796S_DC_GPIO_Port, ST7796S_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(spi, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

static void ST7796S_WriteData(SPI_HandleTypeDef* spi, uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(ST7796S_DC_GPIO_Port, ST7796S_DC_Pin, GPIO_PIN_SET);

    // split data in small chunks because HAL can't send more then 64K at once
    while(buff_size > 0) {
        uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
        HAL_SPI_Transmit(spi, buff, chunk_size, HAL_MAX_DELAY);
        buff += chunk_size;
        buff_size -= chunk_size;
    }
}

static void ST7796S_SetAddressWindow(SPI_HandleTypeDef* spi, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // column address set
    ST7796S_WriteCommand(spi, 0x2A); // CASET
    {
        uint8_t data[] = { (x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF };
        ST7796S_WriteData(spi, data, sizeof(data));
    }

    // row address set
    ST7796S_WriteCommand(spi, 0x2B); // RASET
    {
        uint8_t data[] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
        ST7796S_WriteData(spi, data, sizeof(data));
    }

    // write to RAM
    ST7796S_WriteCommand(spi, 0x2C); // RAMWR
}

void ST7796S_Init(SPI_HandleTypeDef* spi) {

    ST7796S_Select();
    ST7796S_Reset();

    // SOFTWARE RESET
    ST7796S_WriteCommand(spi, 0x01);
    HAL_Delay(5);

    // Command set control (enable extended commands)
    ST7796S_WriteCommand(spi, 0xF0);
    {
        uint8_t data[] = {0xC3};
        ST7796S_WriteData(spi, data, 1);
    }

    ST7796S_WriteCommand(spi, 0xF0);
    {
        uint8_t data[] = {0x96};
        ST7796S_WriteData(spi, data, 1);
    }

    // Memory Access Control (rotation / RGB order)
    ST7796S_WriteCommand(spi, 0x36);
    {
        uint8_t data[] = {0x68};  // same as driver
        ST7796S_WriteData(spi, data, 1);
    }

    // Pixel format (16-bit color)
    ST7796S_WriteCommand(spi, 0x3A);
    {
        uint8_t data[] = {0x05};  // RGB565
        ST7796S_WriteData(spi, data, 1);
    }

    // Interface Mode Control
    ST7796S_WriteCommand(spi, 0xB0);
    {
        uint8_t data[] = {0x80};
        ST7796S_WriteData(spi, data, 1);
    }

    // Display Function Control
    ST7796S_WriteCommand(spi, 0xB6);
    {
        uint8_t data[] = {0x20, 0x02};
        ST7796S_WriteData(spi, data, 2);
    }

    // Blanking Porch Control
    ST7796S_WriteCommand(spi, 0xB5);
    {
        uint8_t data[] = {0x02, 0x03, 0x00, 0x04};
        ST7796S_WriteData(spi, data, 4);
    }

    // Frame Rate Control
    ST7796S_WriteCommand(spi, 0xB1);
    {
        uint8_t data[] = {0x80, 0x10};
        ST7796S_WriteData(spi, data, 2);
    }

    // Display Inversion Control
    ST7796S_WriteCommand(spi, 0xB4);
    {
        uint8_t data[] = {0x00};
        ST7796S_WriteData(spi, data, 1);
    }

    // Entry Mode Set
    ST7796S_WriteCommand(spi, 0xB7);
    {
        uint8_t data[] = {0xC6};
        ST7796S_WriteData(spi, data, 1);
    }

    // VCOM Control
    ST7796S_WriteCommand(spi, 0xC5);
    {
        uint8_t data[] = {0x24};
        ST7796S_WriteData(spi, data, 1);
    }

    // VCOM Offset
    ST7796S_WriteCommand(spi, 0xE4);
    {
        uint8_t data[] = {0x31};
        ST7796S_WriteData(spi, data, 1);
    }

    // Display Output Control Adjust
    ST7796S_WriteCommand(spi, 0xE8);
    {
        uint8_t data[] = {0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33};
        ST7796S_WriteData(spi, data, 8);
    }

    // Power Control 3
    ST7796S_WriteCommand(spi, 0xC2);
    {
        uint8_t data[] = {0xA7};
        ST7796S_WriteData(spi, data, 1);
    }

    // Positive Gamma
    ST7796S_WriteCommand(spi, 0xE0);
    {
        uint8_t data[] = {
            0xF0,0x09,0x13,0x12,0x12,0x2B,0x3C,
            0x44,0x4B,0x1B,0x18,0x17,0x1D,0x21
        };
        ST7796S_WriteData(spi, data, 14);
    }

    // Negative Gamma
    ST7796S_WriteCommand(spi, 0xE1);
    {
        uint8_t data[] = {
            0xF0,0x09,0x13,0x0C,0x0D,0x27,0x3B,
            0x44,0x4D,0x0B,0x17,0x17,0x1D,0x21
        };
        ST7796S_WriteData(spi, data, 14);
    }

    // Re-apply MADCTL (important on some panels)
    ST7796S_WriteCommand(spi, 0x36);
    {
        uint8_t data[] = {0x48};
        ST7796S_WriteData(spi, data, 1);
    }

    // Disable extended command set
    ST7796S_WriteCommand(spi, 0xF0);
    {
        uint8_t data[] = {0xC3};
        ST7796S_WriteData(spi, data, 1);
    }

    ST7796S_WriteCommand(spi, 0xF0);
    {
        uint8_t data[] = {0x69};
        ST7796S_WriteData(spi, data, 1);
    }

    // Normal display mode
    ST7796S_WriteCommand(spi, 0x13);

    // Display inversion OFF
    ST7796S_WriteCommand(spi, 0x20);

    // Exit sleep
    ST7796S_WriteCommand(spi, 0x11);
    HAL_Delay(200);

    // Display ON
    ST7796S_WriteCommand(spi, 0x29);
    HAL_Delay(10);

    ST7796S_Unselect();
}

/*
void ST7796S_Init(SPI_HandleTypeDef* spi) {

    ST7796S_Select();
    ST7796S_Reset();

    // SOFTWARE RESET
    ST7796S_WriteCommand(spi, 0x01);
    HAL_Delay(1000);

    // cmd set ctrl
    ST7796S_WriteCommand(spi, 0xf0);
    {
        uint8_t data[] = { 0xc3 };
        ST7796S_WriteData(spi, data, sizeof(data));
    }

    ST7796S_WriteCommand(spi, 0xf0);
    {
        uint8_t data[] = { 0x96 };
        ST7796S_WriteData(spi, data, sizeof(data));
    }       

    // interface mode ctrl
    ST7796S_WriteCommand(spi, 0xb0);
    {
        uint8_t data[] = { 0x80 };
        ST7796S_WriteData(spi, data, sizeof(data));
    }       

    // display func ctrl
    ST7796S_WriteCommand(spi, 0xb6);
    {
        uint8_t data[] = { 0x20, 0x02 };
        ST7796S_WriteData(spi, data, sizeof(data));
    }       

    // blanking porch ctrl
    ST7796S_WriteCommand(spi, 0xb5);
    {
        uint8_t data[] = { 0x02, 0x03, 0, 0x04 };
        ST7796S_WriteData(spi, data, sizeof(data));
    }       


    ST7796S_Unselect();
}
*/

void ST7796S_DrawPixel(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, uint16_t color) {
    if((x >= ST7796S_WIDTH) || (y >= ST7796S_HEIGHT))
        return;

    ST7796S_Select();

    ST7796S_SetAddressWindow(spi, x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ST7796S_WriteData(spi, data, sizeof(data));

    ST7796S_Unselect();
}

static void ST7796S_WriteChar(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    ST7796S_SetAddressWindow(spi, x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                ST7796S_WriteData(spi, data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                ST7796S_WriteData(spi, data, sizeof(data));
            }
        }
    }
}

void ST7796S_WriteString(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    ST7796S_Select();

    while(*str) {
        if(x + font.width >= ST7796S_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ST7796S_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        ST7796S_WriteChar(spi, x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    ST7796S_Unselect();
}

void ST7796S_FillRectangle(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ST7796S_WIDTH) || (y >= ST7796S_HEIGHT)) return;
    if((x + w - 1) >= ST7796S_WIDTH) w = ST7796S_WIDTH - x;
    if((y + h - 1) >= ST7796S_HEIGHT) h = ST7796S_HEIGHT - y;

    ST7796S_Select();
    ST7796S_SetAddressWindow(spi, x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(ST7796S_DC_GPIO_Port, ST7796S_DC_Pin, GPIO_PIN_SET);
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            HAL_SPI_Transmit(spi, data, sizeof(data), HAL_MAX_DELAY);
        }
    }

    ST7796S_Unselect();
}

void ST7796S_FillScreen(SPI_HandleTypeDef* spi, uint16_t color) {
    ST7796S_FillRectangle(spi, 0, 0, ST7796S_WIDTH, ST7796S_HEIGHT, color);
}

void ST7796S_DrawImage(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if((x >= ST7796S_WIDTH) || (y >= ST7796S_HEIGHT)) return;
    if((x + w - 1) >= ST7796S_WIDTH) return;
    if((y + h - 1) >= ST7796S_HEIGHT) return;

    ST7796S_Select();
    ST7796S_SetAddressWindow(spi, x, y, x+w-1, y+h-1);
    ST7796S_WriteData(spi, (uint8_t*)data, sizeof(uint16_t)*w*h);
    ST7796S_Unselect();
}

void ST7796S_InvertColors(SPI_HandleTypeDef* spi, bool invert) {
    ST7796S_Select();
    ST7796S_WriteCommand(spi, invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
    ST7796S_Unselect();
}

