#!/bin/sh
#make clean
make all
st-flash write build/liquor-store-compass.bin 0x08000000

