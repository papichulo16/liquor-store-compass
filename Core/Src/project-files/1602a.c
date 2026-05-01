#include "main.h"
#include "project.h"

#define LCD_RS_PIN GPIO_PIN_3
#define LCD_EN_PIN GPIO_PIN_4
#define LCD_D4_PIN GPIO_PIN_5
#define LCD_D5_PIN GPIO_PIN_0
#define LCD_D6_PIN GPIO_PIN_1
#define LCD_D7_PIN GPIO_PIN_5
#define LCD_GPIO_PORT1 GPIOA
#define LCD_GPIO_PORT2 GPIOB

void LCD_Pulse_Enable(void) {
    HAL_GPIO_WritePin(LCD_GPIO_PORT1, LCD_EN_PIN, GPIO_PIN_SET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(LCD_GPIO_PORT1, LCD_EN_PIN, GPIO_PIN_RESET);
    HAL_Delay(2);
}

void LCD_Send_Nibble(uint8_t nibble) {
    HAL_GPIO_WritePin(LCD_GPIO_PORT1, LCD_D4_PIN, (nibble >> 0) & 1);
    HAL_GPIO_WritePin(LCD_GPIO_PORT2, LCD_D5_PIN, (nibble >> 1) & 1);
    HAL_GPIO_WritePin(LCD_GPIO_PORT2, LCD_D6_PIN, (nibble >> 2) & 1);
    HAL_GPIO_WritePin(LCD_GPIO_PORT2, LCD_D7_PIN, (nibble >> 3) & 1);
    LCD_Pulse_Enable();
}

void LCD_Send_Byte(uint8_t byte, uint8_t rs) {
    HAL_GPIO_WritePin(LCD_GPIO_PORT1, LCD_RS_PIN, rs);
    LCD_Send_Nibble(byte >> 4);   // High nibble first
    LCD_Send_Nibble(byte & 0x0F); // Low nibble second
}

void LCD_Command(uint8_t cmd)  { LCD_Send_Byte(cmd, 0); HAL_Delay(2); }
void LCD_Char(uint8_t ch)      { LCD_Send_Byte(ch,  1); }

void LCD_Init(void) {
    HAL_Delay(100);

    // Send 0x03 MORE times than you think you need
    // This re-syncs the LCD no matter what state it's in
    LCD_Send_Nibble(0x03); HAL_Delay(10);
    LCD_Send_Nibble(0x03); HAL_Delay(10);
    LCD_Send_Nibble(0x03); HAL_Delay(10);
    LCD_Send_Nibble(0x03); HAL_Delay(10); // extra one for safety
    LCD_Send_Nibble(0x02); HAL_Delay(10); // switch to 4-bit

    LCD_Command(0x28); HAL_Delay(5);  // 4-bit, 2 lines, 5x8
    LCD_Command(0x08); HAL_Delay(5);  // Display OFF
    LCD_Command(0x01); HAL_Delay(5);  // Clear
    LCD_Command(0x06); HAL_Delay(5);  // Entry mode
    LCD_Command(0x0C); HAL_Delay(5);  // Display ON
}

void LCD_Print(const char *str) {
    while (*str) LCD_Char(*str++);
}

void LCD_Set_Cursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    LCD_Command(0x80 | (addr + col));
}

void LCD_Clear() {

  LCD_Set_Cursor(0, 0);
  LCD_Print("                 ");
  LCD_Set_Cursor(1, 0);
  LCD_Print("                 ");
  LCD_Set_Cursor(0, 0);
}

