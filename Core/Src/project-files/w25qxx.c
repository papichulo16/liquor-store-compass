#include "main.h"
#include "project.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define CMD_WRITE_ENABLE        0x06U
#define CMD_WRITE_DISABLE       0x04U
#define CMD_READ_SR1            0x05U   
#define CMD_READ_SR2            0x35U   
#define CMD_WRITE_SR            0x01U
#define CMD_READ_DATA           0x03U
#define CMD_FAST_READ           0x0BU
#define CMD_PAGE_PROGRAM        0x02U
#define CMD_SECTOR_ERASE_4K     0x20U
#define CMD_BLOCK_ERASE_32K     0x52U
#define CMD_BLOCK_ERASE_64K     0xD8U
#define CMD_CHIP_ERASE          0xC7U
#define CMD_POWER_DOWN          0xB9U
#define CMD_RELEASE_POWER_DOWN  0xABU   
#define CMD_MANUF_DEVICE_ID     0x90U
#define CMD_JEDEC_ID            0x9FU
#define CMD_UNIQUE_ID           0x4BU

#define SR1_WIP                 (1U << 0)   
#define SR1_WEL                 (1U << 1)   

#define W25QXX_PAGE_SIZE        256U
#define W25QXX_SECTOR_SIZE      4096U
#define W25QXX_BLOCK32_SIZE     32768U
#define W25QXX_BLOCK64_SIZE     65536U

#define CHIP_ERASE_TIMEOUT_MS   200000U

#define DUMMY_BYTE 0xa5U

w25qxx_t w25qxx = {0};

static inline void CS_LOW()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}

static inline void CS_HIGH()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
}


static w25qxx_status_t spi_transmit(SPI_HandleTypeDef* spi, const uint8_t *pData, uint16_t size)
{
    if (HAL_SPI_Transmit(spi, (uint8_t *)pData, size, W25QXX_TIMEOUT_MS) != HAL_OK)
        return W25QXX_ERR_SPI;
    return W25QXX_OK;
}

static w25qxx_status_t spi_receive(SPI_HandleTypeDef* spi, uint8_t *pData, uint16_t size)
{
    if (HAL_SPI_Receive(spi, pData, size, W25QXX_TIMEOUT_MS) != HAL_OK)
        return W25QXX_ERR_SPI;
    return W25QXX_OK;
}

uint8_t	tr_spi(SPI_HandleTypeDef* spi, uint8_t	data)
{
	uint8_t	ret;
	HAL_SPI_TransmitReceive(spi,&data,&ret,1,100);
	return ret;	
}

static uint8_t pack_address(uint8_t *buf, uint32_t address)
{
    if (w25qxx.CapacityBytes > 0x00FFFFFFUL) {
        buf[0] = (uint8_t)(address >> 24);
        buf[1] = (uint8_t)(address >> 16);
        buf[2] = (uint8_t)(address >>  8);
        buf[3] = (uint8_t)(address);
        return 4U;
    } else {
        buf[0] = (uint8_t)(address >> 16);
        buf[1] = (uint8_t)(address >>  8);
        buf[2] = (uint8_t)(address);
        return 3U;
    }
}

static w25qxx_status_t write_enable(SPI_HandleTypeDef* spi)
{
    uint8_t cmd = CMD_WRITE_ENABLE;
    CS_LOW();
    w25qxx_status_t st = spi_transmit(spi, &cmd, 1);
    CS_HIGH();
    return st;
}


bool w25qxx_is_busy(SPI_HandleTypeDef* spi)
{
    uint8_t cmd = CMD_READ_SR1;
    uint8_t sr  = 0;
    CS_LOW();
    spi_transmit(spi, &cmd, 1);
    spi_receive(spi, &sr, 1);
    CS_HIGH();
    return (sr & SR1_WIP) != 0U;
}

w25qxx_status_t w25qxx_wait_for_ready(SPI_HandleTypeDef* spi, uint32_t timeoutMs)
{
    uint32_t t0 = HAL_GetTick();
    while (w25qxx_is_busy(spi)) {
        if ((HAL_GetTick() - t0) >= timeoutMs)
            return W25QXX_ERR_TIMEOUT;
    }
    return W25QXX_OK;
}

w25qxx_status_t w25qxx_read_device_id(SPI_HandleTypeDef* spi, uint16_t *pID)
{
    uint8_t cmd[5] = { CMD_MANUF_DEVICE_ID, 0x00, 0x00, 0x00 }; /* 3 dummy addr bytes */
    uint8_t rx[2]  = {0};

    CS_LOW();

    w25qxx_status_t st = spi_transmit(spi, cmd, 4);

    if (st == W25QXX_OK)
        st = spi_receive(spi, rx, 2);

    CS_HIGH();

    if (st == W25QXX_OK)
        *pID = (uint16_t)((rx[0] << 8) | rx[1]);

    return st;
}

w25qxx_status_t w25qxx_read_unique_id(SPI_HandleTypeDef* spi)
{
    /* Command + 4 dummy bytes, then 8-byte UID */
    uint8_t cmd[5] = { CMD_UNIQUE_ID, 0x00, 0x00, 0x00, 0x00 };
    CS_LOW();
    w25qxx_status_t st = spi_transmit(spi, cmd, 5);
    if (st == W25QXX_OK)
        st = spi_receive(spi, w25qxx.UniqID, 8);
    CS_HIGH();
    return st;
}


uint32_t w25qxx_init(SPI_HandleTypeDef* spi)
{

    while (HAL_GetTick() < 100)
      HAL_Delay(1);

    CS_HIGH();   /* ensure deselected */
    HAL_Delay(100);

    /* Wake up in case the device is in power-down */
    w25qxx_power_up(spi);

    /* Read JEDEC ID to determine capacity */
    uint8_t cmd = CMD_JEDEC_ID;
    uint8_t jedec[3] = {0};

    CS_LOW();

    //w25qxx_status_t st = spi_transmit(spi, &cmd, 1);
    w25qxx_status_t st = W25QXX_OK;
    tr_spi(spi, 0x9f);
    
    jedec[0] = tr_spi(spi, DUMMY_BYTE);
    jedec[1] = tr_spi(spi, DUMMY_BYTE);
    jedec[2] = tr_spi(spi, DUMMY_BYTE);

    CS_HIGH();

    if (st != W25QXX_OK)
        return st;

    /* jedec[2] encodes log2 of capacity in bytes */
    uint8_t densityCode = jedec[2];
    if (densityCode < 0x11U || densityCode > 0x1AU)
        return (jedec[0] << 16) | (jedec[1] << 8) | jedec[2];   /* unrecognised part */

    w25qxx.CapacityBytes = 1UL << densityCode;   /* e.g. 0x14 -> 16 MB  */

    /* Geometry */
    w25qxx.PageSize      = W25QXX_PAGE_SIZE;
    w25qxx.SectorSize    = W25QXX_SECTOR_SIZE;
    w25qxx.BlockSize32K  = W25QXX_BLOCK32_SIZE;
    w25qxx.BlockSize64K  = W25QXX_BLOCK64_SIZE;
    w25qxx.PageCount     = w25qxx.CapacityBytes / W25QXX_PAGE_SIZE;
    w25qxx.SectorCount   = w25qxx.CapacityBytes / W25QXX_SECTOR_SIZE;
    w25qxx.BlockCount32K = w25qxx.CapacityBytes / W25QXX_BLOCK32_SIZE;
    w25qxx.BlockCount64K = w25qxx.CapacityBytes / W25QXX_BLOCK64_SIZE;

    st = w25qxx_read_device_id(spi, &w25qxx.DeviceID);

    if (st != W25QXX_OK)
        return st;

    st = w25qxx_read_unique_id(spi);

    return st;
}

w25qxx_status_t w25qxx_read(SPI_HandleTypeDef* spi, uint32_t address, uint8_t *pData, uint32_t size) {

    if (pData == NULL || size == 0U)
        return W25QXX_ERR_PARAM;

    w25qxx_status_t st = w25qxx_wait_for_ready(spi, W25QXX_TIMEOUT_MS);

    if (st != W25QXX_OK) return st;

    uint8_t buf[5];
    buf[0] = CMD_READ_DATA;
    uint8_t addrLen = pack_address(&buf[1], address);

    CS_LOW();

    st = spi_transmit(spi, buf, 1U + addrLen);

    if (st == W25QXX_OK) {

        uint32_t remaining = size;
        uint32_t offset    = 0;
        while (remaining > 0U && st == W25QXX_OK) {
            uint16_t chunk = (remaining > 0xFFFFU) ? 0xFFFFU : (uint16_t)remaining;
            st = spi_receive(spi, pData + offset, chunk);
            offset    += chunk;
            remaining -= chunk;
        }
    }

    CS_HIGH();

    return st;
}

w25qxx_status_t w25qxx_power_down(SPI_HandleTypeDef* spi) {

    uint8_t cmd = CMD_POWER_DOWN;

    CS_LOW();

    w25qxx_status_t st = spi_transmit(spi, &cmd, 1);

    CS_HIGH();
    HAL_Delay(1);   

    return st;
}

w25qxx_status_t w25qxx_power_up(SPI_HandleTypeDef* spi) {

    uint8_t cmd = CMD_RELEASE_POWER_DOWN;

    CS_LOW();

    w25qxx_status_t st = spi_transmit(spi, &cmd, 1);

    CS_HIGH();
    HAL_Delay(1);

    return st;
}

