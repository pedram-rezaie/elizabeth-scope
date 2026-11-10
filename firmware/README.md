# Elizabeth scope: firmware

Bare-metal firmware for the STM32F407VGT6 on the Elizabeth board. No HAL, no CMSIS, no libc: every peripheral is set up by direct register writes, with the register maps in `regs.h` written from RM0090. It drives an external 8-bit ADC (TI ADC1173), captures its data with DMA, runs two DACs as signal outputs and talks to the PC over its own USB device stack.

## Files

| File | What it does |
|---|---|
| `build.sh` | compiles and links everything into `firmware.elf` |
| `linker.ld` | memory map and sections |
| `startup.c` | vector table (98 entries), reset handler (copies `.data`, zeroes `.bss`, calls `main`) |
| `regs.h` | register structs and base addresses for the peripherals used |
| `main.c` | init, then a loop that runs `command_read()` whenever a command has arrived |
| `functions.c` / `functions.h` | clocks, GPIO, mux, timers, DACs, DMA, burst capture, command handling |
| `usb.c` / `usb.h` | USB device stack on OTG_FS: enumeration, EP0 control, EP1 bulk in and out, WinUSB descriptors |
| `flash_commands.txt` | the commands to build, connect over SWD and flash |

## Build

Needs the xPack `arm-none-eabi-gcc` toolchain on the PATH. From Git Bash:
```
cd firmware
bash build.sh
```
The output is `firmware.elf`. Flashing with OpenOCD over SWD is in `flash_commands.txt`.

## Clocks

8 MHz HSE crystal into the PLL (M = 4, N = 168, P = 2, Q = 7):

| Clock | Frequency |
|---|---|
| SYSCLK / AHB | 168 MHz |
| APB1 | 42 MHz (timers 84 MHz) |
| APB2 | 84 MHz (timers 168 MHz) |
| USB | 48 MHz |

Flash runs with 5 wait states, prefetch and both caches on.

## Pins

| Pin | Use |
|---|---|
| PA8 | ADC clock, TIM1_CH1 (AF1), 50% PWM |
| PD0:PD7 | ADC data in. Wired reversed: PD0 is the ADC MSB, PD7 the LSB |
| PA4 | DAC1: DC level with optional triangle |
| PA5 | DAC2: waveform generator |
| PB4, PB5, PB6 | input mux select (gain x1 to x8) |
| PB0 | status LED, open drain, active low. Blinks when idle, solid while waiting for a DAC2 table |
| PA11, PA12 | USB D- / D+ (OTG_FS, USB-C, no bridge chip) |
| PA13, PA14 | SWD |

## Memory

| Region | Address | Size | Holds |
|---|---|---|---|
| FLASH | 0x08000000 | 1024K | code, constants, `.data` load image |
| CCM | 0x10000000 | 64K | stack (top 0x10010000), `.data`, `.bss` |
| RAM | 0x20000000 | 120K | capture buffer (`.dma_input`) |
| DACRAM | 0x2001E000 | 8K | DAC2 table (`.dac_table`) |

The DMA can't reach CCM, so everything the DMA touches lives in RAM or DACRAM, and everything else lives in CCM.

## How it works

**Commands:** the PC sends a 64-byte command on EP1 OUT. The USB interrupt copies it into `cmd_buf` and sets a flag; the main loop then runs `command_read()`, which checks the magic word and applies each section whose update / enable bit is set: mux, DAC1, DAC2, ADC clock, capture. The full message layout is in `software/README.md`.

**ADC clock and capture:** TIM1 runs at 168 MHz. CH1 outputs the ADC clock on PA8, ARR sets the sample rate (ARR 9 = 16.8 MHz). CH2 has no pin; its compare event is the DMA request, so CCR2 sets where in the clock period the data pins are read.

**Burst capture:** DMA2 Stream 2 Channel 6 (TIM1_CH2) reads the `GPIOD` input register one byte per request, packs 4 samples per word through the FIFO and writes to RAM in double buffer mode, 2 x 61312 bytes. When the second half is full, the DMA interrupt stops the stream and sets a flag. `burst_capture()` waits for that flag, then sends all 122624 bytes on EP1 IN followed by a zero-length packet.

**DAC1:** TIM6 triggers the DAC's built-in triangle generator, centered on the DC value.

**DAC2:** DMA1 Stream 6 Channel 7 plays a table of up to 4096 samples from DACRAM into the DAC in circular mode, triggered by TIM7 (ARR 83 = 1 MS/s). The table is built on the PC and uploaded after a zero-length packet handshake.

**Interrupt priorities:** DMA2 Stream 2 (capture) has priority 0, OTG_FS has priority 1, so USB traffic can never delay the end of a capture.

## USB

- VID:PID `1209:0001`
- Full speed, vendor class: EP0 control, EP1 OUT `0x01` (bulk, 64 bytes) for commands, EP1 IN `0x81` (bulk) for data
- MS OS 2.0 descriptors make Windows load WinUSB on its own, so no driver install is needed
- The board is self-powered and VBUS sensing is off
