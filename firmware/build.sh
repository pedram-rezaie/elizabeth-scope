#this is a script to compile things fast
# we are using a cortex m4 that runs only tumb

set -e
CFLAGS="-mcpu=cortex-m4 -mthumb -Wall"
echo $CFLAGS

echo "compiling startup"
arm-none-eabi-gcc $CFLAGS -c startup.c -o startup.o
echo "compiling main"
arm-none-eabi-gcc $CFLAGS -c main.c -o main.o
echo "compiling usb"
arm-none-eabi-gcc $CFLAGS -c usb.c -o usb.o
echo "compiling functions"
arm-none-eabi-gcc $CFLAGS -c functions.c -o functions.o
echo "linking"
arm-none-eabi-gcc $CFLAGS -nostdlib -T linker.ld functions.o usb.o startup.o main.o -o firmware.elf
echo "making bin"
arm-none-eabi-objcopy -O binary firmware.elf firmware.bin
arm-none-eabi-size firmware.elf
