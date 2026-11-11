# Elizabeth v2

https://github.com/user-attachments/assets/4d803e4a-9425-4e94-8e22-d3d69fb301be

A USB oscilloscope with 2 MHz bandwidth at 16.8 MSPS (overclock 20 MSPS), built around an STM32F407 and an ADC1173. 4-layer board, hand soldered, register-level firmware and a Python app on the PC side. The second DAC doubles as a small function generator.

This is the second version. The first board never worked because of footprint and schematic mistakes, so I redid v2 from scratch with hand soldering in mind: bigger pads, one solid ground plane, and separate LDOs for the analog and digital side. It still needed a couple of bodge wires after fabrication. Those mistakes are fixed in the design files now.

## Specs

| | |
|---|---|
| Channels | 1 |
| ADC | ADC1173, 8 bit, parallel output |
| Sample rate | 16.8 MSPS normally, up to 20 MSPS. The hardware timing is designed for 14 MSPS as the worst case |
| Bandwidth | 2 MHz at 16.8 MSPS, up to 3 MHz at 20 MSPS |
| Input range | ±7 V with a 10x probe (14 Vpp). Smallest signal that shows up reliably is about 0.2 Vpp |
| Gain | 1x to 8x in integer steps |
| Capture length | 122,624 samples |
| Function generator | Sine, square, triangle, sawtooth from a 4096-sample table at 1 MS/s |
| USB | Full Speed, WinUSB, no driver install on Windows |
| Power | Two batteries for ±VBAT. USB only carries data |
| MCU | STM32F407VGT6 |

## How it works

```
BNC -> OPA1656 buffer + DAC1 bias -> 4th order low-pass -> PGA (ADG708 + OPA356) -> ADC1173
    -> PD0-PD7 -> DMA2 -> SRAM -> USB -> PC
```

The input gets buffered, shifted up by a DC bias from DAC1 so it fits into 0 to 3.3 V, filtered, and then scaled by the gain stage before it reaches the ADC. The bias goes through the gain stage too, so it has to be changed whenever the gain changes. The details are in [`hardware/README.md`](hardware/README.md).

On the STM32 side, TIM1 channel 1 makes the ADC clock on PA8. Channel 2 of the same timer fires a bit later, once the ADC outputs have settled, and that triggers a DMA transfer from `GPIOD->IDR` into RAM. The CPU doesn't touch the samples at all during a capture. When the buffer is full it goes out over USB.

I kept the firmware as dumb as I could. It sets the DACs, the gain and the sample rate, captures, and sends the data. The PC builds the commands, generates the function generator waveforms, and flips the bit order of every sample (the ADC data pins ended up wired backwards on PD0-PD7). That way most changes don't need a reflash.

## Repository

| Folder | |
|---|---|
| [`hardware/`](hardware/) | Altium project, component library, schematic PDF |
| [`firmware/`](firmware/) | STM32 firmware, building and flashing with OpenOCD |
| [`software/`](software/) | Python app and the USB protocol |

Each folder has its own README. Gerbers go in [Releases](../../releases).

## Running it

Flash the firmware over SWD (see [`firmware/README.md`](firmware/README.md)), plug in USB, then in PowerShell:

```powershell
cd software
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

## What I'm working on

Signal reconstruction: take the DFT of a capture, correct it with the measured frequency response of the front end, and go back with an inverse DFT. That should undo the filter roll-off near the top of the bandwidth.

Dithering: DAC1 adds a small known triangle on top of the bias. Averaging over it should get more than 8 bits out of slow signals.
