# Elizabeth scope: PC software

The PC side of the Elizabeth oscilloscope, Windows only for now. It talks to the STM32F407 over native USB (vendor class, WinUSB on Windows) and gives you a GUI to set the input gain, both DACs and the ADC clock, start a capture and look at the result.


## Files

| File | What it does |
|---|---|
| `main.py` | entry point, finds the scope over USB and opens the GUI |
| `GUI.py` | main window (PySide6): one box per function on the left, preview of the last capture on the right |
| `control_functions.py` | builds the 64-byte command message, DAC2 waveform tables, USB send / receive |
| `signal_viewer.py` | separate window for the last capture, with zoom and scroll |
| `requirements.txt` | Python packages |

## Install and run

Needs Python 3.9 or newer.

In PowerShell:
```
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
python main.py
```

If the board isn't plugged in, `main.py` stops with `scope not found`.

## Drivers

Nothing to install. The firmware has MS OS 2.0 descriptors, so Windows loads WinUSB by itself, and `libusb-package` brings its own libusb DLL.

## Using the GUI

Every box has its own set button. Pressing it clears the enable bits of all other boxes, writes its own settings and sends the command right away, so one button only changes one thing. The enable field of a box has to be 1, otherwise the STM ignores that box.

- **mux:** input gain x1 to x8.
- **DAC1 (PA4):** DC level, optionally with the built-in triangle on top. Values in volts, 0 to 3 V. The amplitude is from 0 to top (peak-to-peak), the real amplitude is half of it. "dac1 off" turns the channel off.
- **DAC2 (PA5):** function generator, sine / square / triangle / sawtooth at 1 MS/s. Amplitude is the peak, in volts. Set "new table" to 1 to upload a new waveform and "enable" to 1 to play it. "dac2 off" turns the channel off.
- **ADC clock:** TIM1 drives the ADC clock on PA8. Sample rate = 168 MHz / (ARR + 1), ARR 9 gives 16.8 MHz. CCR2 (0 to ARR) is the point in the clock period where the DMA reads PD0:PD7; above ARR the DMA never fires and the capture hangs. PA8 speed sets the output edge speed of the clock pin. The ADC doesn't work below about 1 MHz.
- **capture:** burst capture only for now. The continuous modes are placeholders and show a warning.

The plots show the raw data: x is the sample index, y is the ADC number (0 to 255). "signal viewer" in the toolbar opens the full viewer window.

## USB protocol

EP1 OUT (`0x01`) takes commands, EP1 IN (`0x81`) returns data. VID:PID is `1209:0001`.

Every command is exactly 64 bytes = 16 words of 32 bits, little endian. Every message carries all sections; a section is only applied when its update / enable bit is 1.

| Word | Bits | Meaning |
|---|---|---|
| 0 | 31:0 | magic `0x53430001` (also the protocol version), anything else and the message is ignored |
| 1 | 2:0 | mux pins PB4 / PB5 / PB6 (bit 0 = PB4) |
| 1 | 3 | mux update |
| 2 | 0 | DAC1 update |
| 2 | 1 | DAC1 turn off (needs update = 1) |
| 2 | 17:2 | DAC1 triangle amplitude, 12-bit, peak-to-peak |
| 3 | 31:0 | DAC1 triangle frequency in Hz |
| 4 | 0 | DAC1 triangle on |
| 4 | 16:1 | DAC1 DC value, 12-bit |
| 5 | 0 | DAC2 table update, a new table follows |
| 5 | 1 | DAC2 turn off (needs update = 1) |
| 5 | 2 | DAC2 update / start playing |
| 6 | 15:0 | TIM7 ARR for DAC2 (83 = 1 MS/s) |
| 6 | 31:16 | DAC2 table length, up to 4096 samples |
| 7 | 0 | capture enable |
| 7 | 2:1 | capture mode, 01 = burst (others are placeholders) |
| 8 | 0 | ADC clock update |
| 8 | 2:1 | PA8 speed: 00 low, 01 medium, 10 fast, 11 high |
| 9 | 15:0 | TIM1 ARR (ADC clock) |
| 9 | 31:16 | TIM1 CCR2 (DMA read point) |
| 10 to 15 | | unused |

**DAC2 table upload:** send the command with table update = 1, wait for a zero-length packet on EP1 IN, then send the table. The table is always 8192 bytes (4096 samples, uint16 little endian, 12-bit right aligned); unused samples are padding.

**Burst capture:** after a command with capture enable = 1 and mode = burst, the scope fills its RAM and sends 122624 bytes on EP1 IN, followed by a zero-length packet. One byte is one sample, in time order. The ADC data pins are wired reversed (PD0 is the ADC MSB), so every byte arrives bit reversed; `bit_reverse()` in `control_functions.py` flips them back.

## Limits

- Burst capture only, 122624 samples per capture.
- No volts on the y axis yet, raw ADC numbers only.
- No FFT yet.
