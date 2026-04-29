# Liquor Store Compass
## By Luis Abraham and Amy Alvarez

### About

This is a memory-constrained embedded device that will serve as a compass that will point the user to the nearest liquor store... ENJOY YOUR JOURNEY PIRATE!!!

The way this works is we have packed every liquor store coordinate in Florida into a KD-tree and put into a SPI flash. Then we will use that, the magnetometer, and the GPS module to find the nearest store and its relation to the user's current orientation. Then we display to an LCD.

### Components

Core processing: `STM32L412KB`

I2C Components: `GY-271M Magnetometer`

SPI Components: `W25W64 SPI Flash Memory`, `LCD Screen`

UART Components: `NEO-6M GPS Satellite Positioning Module`


