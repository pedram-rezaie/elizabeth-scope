# Elizabeth v2


https://github.com/user-attachments/assets/4d803e4a-9425-4e94-8e22-d3d69fb301be


A USB oscilloscope I designed and built around an STM32F407 and an ADC1173. 4-layer board, hand soldered, register-level firmware and a Python app on the PC side. ** the design assumes an 10x probe**

<!-- TODO: board photo -->
<!-- ![Elizabeth v2](docs/board.jpg) -->

<!-- TODO: demo GIF or video -->

This is the second version. The first board never worked because of footprint and schematic mistakes, so I redid v2 from scratch with hand assembly in mind: bigger pads, one solid ground plane, and analog and digital 3.3 V split on the power layer with a ferrite between them. It still needed a few bodge wires after fabrication. Those are listed in [`hardware/README.md`](hardware/README.md) so the next revision can fix them.

## Specs

| | |
|---|---|
| Channels | 1 |
| ADC | ADC1173, 8 bit, parallel output |
| Sample rate | Firmware runs at 16.8 MSPS. The hardware timing is designed for 14 MSPS as the worst case |
| Bandwidth | Front end designed for 6-7 MHz. In practice it drops about 12 dB at 3 MHz, which the host corrects with FFT equalization |
| Input range | ±7 V max (14 Vpp). Smallest signal that shows up reliably is about 0.2 Vpp |
| Gain | 1x to 8x in integer steps (ADG708 mux) |
| Capture length | 122,624 samples |
| USB | Full Speed, vendor class bulk endpoints, WinUSB (no driver install on Windows) |
| Power | Battery with bipolar rails. USB only carries data and VBUS sense |
| MCU | STM32F407VGT6 |

## How it works
Input → front end + DAC1 bias → ADG708 gain → OPA1656 buffer → ADC1173 → PD0-PD7 → DMA2 (triggered by TIM1) → SRAM → USB → PC

TIM1 channel 1 generates the ADC clock on PA8. Channel 2 of the same timer fires a bit later, once the ADC outputs have settled, and triggers a DMA transfer from `GPIOD->IDR` into RAM. The CPU never touches the samples during a capture. When the buffer is full it goes out over USB.

I kept the firmware as dumb as possible. It sets the DACs, the gain mux and the clock divider, captures, and sends the data. Everything else is done in Python: triggering, converting codes to volts, calibration, the FFT, and reversing the bit order (the ADC data pins ended up connected backwards on PD0-PD7). That way I can change the signal processing without reflashing the board.

DAC1 sets the DC bias of the input and has to follow the selected gain. DAC2 works as a simple function generator, which I use for testing and calibrating the input.

## Repository

| Folder | |
|---|---|
| [`hardware/`](hardware/) | Altium project, component library, reworks |
| [`firmware/`](firmware/) | STM32 firmware, build and flashing with OpenOCD |
| [`software/`](software/) | Python host app |

Each folder has its own README. Gerbers, the schematic PDF, the compiled library and a firmware binary are in [Releases](../../releases).

## Running it

Flash the firmware over SWD (see [`firmware/README.md`](firmware/README.md)), plug in USB, then:

```bash
cd software
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

## In progress

- **Signal reconstruction**: take the DFT of the capture, correct it with the front end's measured frequency response, then inverse DFT back to the time domain. This should undo the filter roll-off and give a cleaner waveform near the bandwidth limit.
- **Dithering**: DAC1 injects a small known signal on top of the bias. Averaging over it should give more than 8 bits of effective resolution on slow signals.

## Related

The USB stack is also available on its own: <!-- TODO: link -->

## License

<!-- TODO -->
