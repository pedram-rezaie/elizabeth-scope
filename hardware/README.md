# Hardware

4-layer board designed in Altium Designer, made by JLCPCB and hand soldered. Power, analog front end, ADC, STM32 and USB are all on one board.

## Files

| File | |
|---|---|
| `altium/Elizabeth.PrjPcb` | Project file, open this one |
| `altium/STM32.SchDoc` | Top sheet: MCU, crystal, SWD header, buttons, LEDs, BNC input, and the sheet symbols for the other pages |
| `altium/Power.SchDoc` | Battery inputs, protection, LDOs, USB-C |
| `altium/FrontEnd.SchDoc` | Input buffer, DC bias, anti-aliasing filter |
| `altium/Amplifier.SchDoc` | Gain stage (resistor ladder, ADG708, OPA356) |
| `altium/ADC.SchDoc` | ADC1173 |
| `altium/PCB1.PcbDoc` | Layout |
| `altium/Elizabeth.BomDoc` | BOM |
| `altium/Elizabeth.OutJob` | Generates Gerbers, drill files and the schematic PDF |
| `altium/lib/` | My component library (`Elizabeth_Library.LibPkg` with its SchLib and PcbLib) |

If you don't have Altium, the schematic PDF and Gerbers are in the release.

## Signal path

The front end is designed for a 10x probe. ±7 V at the probe tip is ±0.7 V at the BNC.

**Input.** BNC connector, 1 MΩ to ground with 3.3 pF in parallel, so a standard 10x scope probe can be compensated against it. A TVS diode (GBLC08) and a dual Schottky (BAS70-04) clamping to VBAT+ and VBAT- protect the input.

**Buffer and bias.** The OPA1656 runs from the bipolar battery rails, so it can take negative input voltages, and its FET inputs don't load the 1 MΩ input. Its second channel buffers the DC bias from the STM32's DAC1. An OPA356 adds the signal and the bias together, which moves the signal into the 0 to 3.3 V range of everything after it.

**Anti-aliasing filter.** Two unity-gain Sallen-Key stages (U3 and U2, both OPA356) make a 4th-order Butterworth low-pass with the cutoff at about 2 MHz. The worst case I designed for is 14 MSPS. There, anything at 11 MHz folds back onto 3 MHz, so the filter is designed to give about -57 dB at 11 MHz. One LSB of an 8-bit ADC is -48 dB, so whatever aliases ends up below one LSB.

**Gain.** Eight 47 Ω resistors in series form a divider with taps at 1/8, 2/8 ... 8/8 of the signal. The ADG708 picks one tap, and an OPA356 with a gain of about 8 (1 + 1k/143) brings it back up, so the total gain is 1x to 8x. The mux only drives the high-impedance op-amp input, so its on-resistance doesn't matter. With 200 MHz GBW and a gain of 8, the OPA356 still has about 25 MHz of bandwidth, far above the filter. It runs on 3.3 V, so its output can't leave the ADC's supply range.

**ADC.** ADC1173 with its references self-biased (VRT tied to VRTS, VRB tied to VRBS). 47 Ω and 47 pF at its input isolate the op-amp from the ADC's sampling kickback. The clock comes from STM32 PA8 through a 22 Ω series resistor to damp ringing, and the 8 data lines go to PD0-PD7.

## Power

Two batteries on JST XH connectors: BAT1 makes VBAT+, BAT2 makes VBAT-, and the point between them is ground. Both rails get the same 100 µF bulk capacitor, because uneven decoupling on bipolar rails shows up as even-harmonic distortion. Each battery has a PTC fuse and a MOSFET for reverse-polarity protection (P-channel on the positive side, N-channel on the negative side).

Two AP2112K-3.3 LDOs run from VBAT+: LDO1 for the digital 3.3 V and LDO2 for the analog 3.3 V, so switching noise from the STM32 doesn't end up on the analog supply. The ADC's digital supply is taken from the analog rail through 47 Ω. A REF3030 gives the STM32 a 3.0 V VREF+.

USB does not power anything. VBUS only goes through a 22k/47k divider to PA9 so the firmware knows when a host is plugged in.

## USB

GCT USB4930 USB-C connector, USB 2.0 only. CC1 and CC2 each have their own 5.1k pulldown. A USBLC6-2SC6 protects D+ and D-, which go to the STM32's OTG_FS pins.

## Stackup and grounding

JLC7628, 1.6 mm, 1 oz outer and 0.5 oz inner copper.

| Layer | |
|---|---|
| L1 | Signals and parts |
| L2 | Solid GND, no splits |
| L3 | Power pours: analog 3.3 V with the VBAT+ and VBAT- areas on one side, digital 3.3 V on the other |
| L4 | Power electronics |

The ground plane is one continuous plane, but the board is still split into an analog and a digital side. The split is done by placement: the analog parts sit over one part of the plane and the digital parts over the other, and no fast digital trace crosses the analog area. Return currents stay under their own traces, so they don't mix, and there's no gap in the plane that a signal would have to go around.

The USB pair on L1 is 0.240 mm wide with 0.127 mm spacing, about 90 Ω on this stackup.

## Design choices

I designed v2 to be easy to hand solder rather than to be perfect on paper. The pads are IPC Level A with extended toes, and the crystal has oversized pads so it can be reflowed with an iron.

The ADC data lines go to PD0-PD7 because those pins sit next to each other on one side of the LQFP-100. On port E the same bits are spread around the package. The catch is that the order ended up reversed (PD0 = D7), which the host software fixes.
