#include "main.h"
#include "fonts.h"

#include <stdbool.h>
#include <stdint.h>

#define MAG_X_MIN (-480.0f)
#define MAG_X_MAX  (122.0f)
#define MAG_Y_MIN (-2069.0f)
#define MAG_Y_MAX (-1555.0f)

#define ST7796S_MADCTL_MY  0x80
#define ST7796S_MADCTL_MX  0x40
#define ST7796S_MADCTL_MV  0x20
#define ST7796S_MADCTL_ML  0x10
#define ST7796S_MADCTL_RGB 0x00
#define ST7796S_MADCTL_BGR 0x08
#define ST7796S_MADCTL_MH  0x04

#define ST7796S_RES_Pin       GPIO_PIN_4
#define ST7796S_RES_GPIO_Port GPIOA
#define ST7796S_CS_Pin        GPIO_PIN_1
#define ST7796S_CS_GPIO_Port  GPIOA
#define ST7796S_DC_Pin        GPIO_PIN_3
#define ST7796S_DC_GPIO_Port  GPIOA

// default orientation
#define ST7796S_WIDTH  320
#define ST7796S_HEIGHT 480
#define ST7796S_ROTATION (ST7796S_MADCTL_MX | ST7796S_MADCTL_BGR)

// Color definitions
#define	ST7796S_BLACK   0x0000
#define	ST7796S_BLUE    0x001F
#define	ST7796S_RED     0xF800
#define	ST7796S_GREEN   0x07E0
#define ST7796S_CYAN    0x07FF
#define ST7796S_MAGENTA 0xF81F
#define ST7796S_YELLOW  0xFFE0
#define ST7796S_WHITE   0xFFFF
#define ST7796S_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))


#define W25QXX_TIMEOUT_MS   100U       

#define KD_SIZE 0x214

typedef struct {
    float q0, q1, q2, q3;  // Quaternion
    float integralFBx;      // Integral feedback (gyro bias correction)
    float integralFBy;
    float integralFBz;
} MahonyState;

typedef struct {
    double lat;
    double lon;
} location_t;
 
typedef struct {
    uint32_t PageSize;        
    uint32_t SectorSize;       
    uint32_t BlockSize32K;     
    uint32_t BlockSize64K;      
    uint32_t SectorCount;
    uint32_t BlockCount32K;
    uint32_t BlockCount64K;
    uint32_t PageCount;
    uint32_t CapacityBytes;
    uint16_t DeviceID;         
    uint8_t  UniqID[8];        
} w25qxx_t;
 
extern w25qxx_t w25qxx;       
extern location_t kd_points[];
 
typedef enum {
    W25QXX_OK      = 0,
    W25QXX_ERR_SPI,
    W25QXX_ERR_TIMEOUT,
    W25QXX_ERR_PARAM,
    W25QXX_ERR_ID,
} w25qxx_status_t;

bool gy271m_verify(I2C_HandleTypeDef* i2c);
bool gy271m_init(I2C_HandleTypeDef* i2c);
bool gy271m_read(I2C_HandleTypeDef* i2c, int16_t* x, int16_t* y, int16_t* z);
float gy271m_heading(int16_t x, int16_t y, int16_t z);
 
int MPU6050_init(I2C_HandleTypeDef* i2c); //Initialize the MPU 
void MPU6050_Read_Accel (I2C_HandleTypeDef* i2c, float *Ax, float *Ay, float *Az); //Read MPU Accelerator 
void MPU6050_Read_Gyro (I2C_HandleTypeDef* i2c, float *Gx, float *Gy, float *Gz); //Read MPU Gyroscope

uint32_t w25qxx_init(SPI_HandleTypeDef* spi);
w25qxx_status_t w25qxx_read(SPI_HandleTypeDef* spi, uint32_t address, uint8_t *pData, uint32_t size);
w25qxx_status_t w25qxx_read_device_id(SPI_HandleTypeDef* spi, uint16_t *pID);
w25qxx_status_t w25qxx_read_unique_id(SPI_HandleTypeDef* spi);
bool w25qxx_is_busy(SPI_HandleTypeDef* spi);
w25qxx_status_t w25qxx_wait_for_ready(SPI_HandleTypeDef* spi, uint32_t timeoutMs);
w25qxx_status_t w25qxx_power_down(SPI_HandleTypeDef* spi);
w25qxx_status_t w25qxx_power_up(SPI_HandleTypeDef* spi);

// call before initializing any SPI devices
void ST7796S_Unselect();

void ST7796S_Init(SPI_HandleTypeDef* spi);
void ST7796S_DrawPixel(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, uint16_t color);
void ST7796S_WriteString(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor);
void ST7796S_FillRectangle(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7796S_FillScreen(SPI_HandleTypeDef* spi, uint16_t color);
void ST7796S_DrawImage(SPI_HandleTypeDef* spi, uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
void ST7796S_InvertColors(SPI_HandleTypeDef* spi, bool invert);

location_t* find_nearest(double lat, double lon);
double distance(double lat1, double lon1, double lat2, double lon2);
double distance_miles(double lat1, double lon1, double lat2, double lon2);

void Mahony_Init(MahonyState *state);
void Mahony_Update(MahonyState *s, float gx, float gy, float gz, float ax, float ay, float az, float mx, float my, float mz, float dt);
float Mahony_GetHeading(MahonyState *s);

