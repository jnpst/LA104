export PATH="/Users/gabrielvalky/Downloads/gcc-arm-none-eabi-7-2018-q2-update/bin/":"$PATH"

arm-none-eabi-gcc -Os -fno-common -mcpu=cortex-m3 -mthumb -msoft-float -fno-exceptions -DSTM32F1 -DLA104 -MD -D _ARM -D STM32F10X_HD -c webusb.c
