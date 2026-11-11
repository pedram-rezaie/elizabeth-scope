<img width="2560" height="1439" alt="pcb" src="https://github.com/user-attachments/assets/ada599f1-2d1b-4ad8-84e9-a025ba8ee9dd" />
# Hardware

4-layer board, designed in Altium, made by JLCPCB and soldered by hand. Everything is on the one board: power, front end, ADC, STM32 and USB.

## Files

| File | |
|---|---|
| `altium/Elizabeth.PrjPcb` | Project file, open this one |
| `altium/STM32.SchDoc` | Top sheet. MCU, crystal, SWD header, buttons, LEDs, BNC input, plus the sheet symbols for the other pages |
| `altium/Power.SchDoc` | Batteries, protection, LDOs, USB-C |
| `altium/FrontEnd.SchDoc` | Input buffer, DC bias, anti-aliasing filter |
| `altium/Amplifier.SchDoc` | Gain stage |
| `altium/ADC.SchDoc` | ADC1173 |
| `altium/PCB1.PcbDoc` | Layout |
| `altium/Elizabeth.BomDoc` | BOM |
| `altium/Elizabeth.OutJob` | Gerbers, drill files and schematic PDF in one click |
| `altium/lib/` | My component library |

No Altium? The schematic PDF and Gerbers are in the release.

## Signal path

```
BNC -> input buffer -> + DC bias -> 4th order low-pass -> PGA (1x to 8x) -> ADC1173 -> STM32
```

### Input

The front end assumes a 10x probe, so ±7 V at the probe tip is ±0.7 V at the BNC. The input itself is 1 MΩ in parallel with 3.3 pF, like a normal scope, so the probe can be compensated. A GBLC08 TVS and a BAS70-04 clamping to the battery rails protect it.

### Buffer and DC bias

The ADC only sees 0 to 3.3 V, so a negative input has to be lifted up first.

The input goes into an OPA1656. It runs from the bipolar battery rails so it doesn't care about negative voltages, and its FET inputs don't load the 1 MΩ. The bias comes from DAC1 on the STM32, through an RC low-pass, and into the OPA1656's second channel.

The two buffered signals meet in a 1k/1k divider. That adds them, but also cuts both in half, so the OPA356 after it has a gain of 2 to undo that. What comes out is the input sitting on top of the bias.

### Anti-aliasing filter

Two Sallen-Key stages, which together make a 4th order Butterworth low-pass at about 2 MHz.

I designed the hardware for 14 MSPS as the worst case. At that rate anything at 11 MHz lands on 3 MHz after sampling, so the filter has to be well down by 11 MHz. It's designed for about -57 dB there. One LSB of an 8-bit ADC is -48 dB, so anything that still aliases is smaller than one LSB.

### PGA

Eight 47 Ω resistors in a row make a divider with taps at 1/8, 2/8 and so on up to 8/8. The ADG708 mux picks a tap, and an OPA356 with a gain of 8 (1 + 1k/143) brings it back up. That gives 1x to 8x.

The mux only feeds an op-amp input, so its on-resistance doesn't matter. At a gain of 8 the OPA356 still has around 25 MHz of bandwidth, way above the filter. And since it runs on 3.3 V, it physically can't push the ADC input outside its supply.

### ADC

ADC1173, references self-biased (VRT to VRTS, VRB to VRBS). There's 47 Ω and 47 pF at its input to keep the op-amp away from the sampling kickback. The clock comes from PA8 through 22 Ω to damp ringing. The data lines go to PD0-PD7.

## Power

Two batteries on JST XH connectors. BAT1 makes VBAT+, BAT2 makes VBAT-, and ground is the point between them. Each one has a PTC fuse and a MOSFET against reverse polarity. Both rails get the same 100 µF bulk cap. If the decoupling on a bipolar supply is uneven, it shows up as even harmonics in the signal.

There are two AP2112K-3.3 LDOs on VBAT+. LDO1 feeds the digital side, LDO2 the analog side, so the STM32's switching noise stays off the analog supply. The ADC runs completely from the analog LDO. Its digital supply pins get their own branch, split off through a small resistor and a lot of capacitors. A REF3030 gives the STM32 a 3.0 V VREF+.

USB doesn't power anything. VBUS only goes through a 22k/47k divider to PA9 so the firmware can see when a PC is plugged in.

## USB

USB-C connector (GCT USB4930), USB 2.0 only. CC1 and CC2 each get their own 5.1k pulldown. A USBLC6-2SC6 does ESD protection on D+ and D-.

The data pair is routed as a 90 Ω differential pair. I got the width and spacing (0.240 mm / 0.127 mm) from Altium's impedance calculator for this stackup.

## Stackup and grounding

JLC7628, 1.6 mm, 1 oz outside, 0.5 oz inside.

| Layer | |
|---|---|
| L1 | Signals and parts |
| L2 | Solid ground |
| L3 | Power, cut into four regions: analog 3.3 V, digital 3.3 V, VBAT+ and VBAT- (those two only feed the input buffer) |
| L4 | Power electronics |

The ground plane isn't cut anywhere. The analog/digital split is only in the placement: the ADC sits in the middle, analog parts on one side, digital on the other, and no fast digital trace runs through the analog area. Return currents follow their own traces and stay on their own side, and nothing has to detour around a slot in the plane.

## Design choices

v2 was designed to be easy to solder by hand. The pads are IPC Level A with longer toes, and the crystal has oversized pads so I could reflow it with an iron.

The ADC goes to PD0-PD7 because those pins are next to each other on one side of the LQFP-100. Port E would have been spread around the chip. The bit order ended up reversed though (PD0 = D7), so the PC software flips it.


