#!/bin/sh
#make clean
make all
st-flash write build/freertos-l412kb.bin 0x08000000

