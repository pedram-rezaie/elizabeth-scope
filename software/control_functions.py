import usb.core
import time
import math
import numpy as np
# if we are uploading the dac2 table we need to wait for the command transfer to be completed, device will send a zlp whnen its ready to reieve the table
# if capture enable is used we have to recieve 120KB of data
# if command read was succesful we get a zlp at the end which indicates we can start using the data
tableUploadStart=0
tableUploadEnd=1
captureStart=0
captureEnd=1
oscilloscopeMode = 00
command = bytearray(64)
dac2_table = bytearray(8192)
command[0:4] = (0x53430001).to_bytes(4, 'little')

# bit 0 = pb4, bit 3= doweupdate bit4:7 padding
# multiplication level -> pb6 pb5 pb4
# 1x: 011, 2x: 010, 3x: 001, 4x: 000, 5x: 111, 6x: 110, 7x: 101, 8x: 100
def mux_set(doWeUpdate, multiplication_level):
    if   multiplication_level == 1: level_bits = 0b011
    elif multiplication_level == 2: level_bits = 0b010
    elif multiplication_level == 3: level_bits = 0b001
    elif multiplication_level == 4: level_bits = 0b000
    elif multiplication_level == 5: level_bits = 0b111
    elif multiplication_level == 6: level_bits = 0b110
    elif multiplication_level == 7: level_bits = 0b101
    elif multiplication_level == 8: level_bits = 0b100
    else:
        raise ValueError(f"multiplication_level must be 1..8, got {multiplication_level}")

    command[4] = (command[4] & ~0x0F & 0xFF) | level_bits | (doWeUpdate & 0x01) << 3

def dac1_set(dc_value,triangle,frequency,amplitude,dac1_turnOff,dac1_doWeUpdate):
    word0 = 0x0000
    word0 = word0 | (dac1_doWeUpdate & 0x1) | (dac1_turnOff & 0x1)<<1 | (amplitude & 0xffff)<<2
    word1 = frequency
    word2 = 0x0000
    word2 = word2 | triangle | (dc_value&0xffff)<<1
    command[8:12] = (word0 & 0xFFFFFFFF).to_bytes(4, 'little')
    command[12:16] = (word1 & 0xFFFFFFFF).to_bytes(4, 'little')
    command[16:20] = (word2 & 0xFFFFFFFF).to_bytes(4, 'little')


def dac2_activate(table,lenght, arr,dac2_update,dac2_turnOff,table_update) :
    global tableUploadStart, tableUploadEnd
    if(table_update & 0x1):
        tableUploadStart=1
        tableUploadEnd=0
        dac2_table[:] = table
    else:
        tableUploadStart=0
        tableUploadEnd=1

    word0 = 0x00000000
    word0 = word0 | (table_update & 0x1) | (dac2_turnOff & 0x1)<<1 | (dac2_update & 0x1)<<2
    word1 = (arr & 0x0000ffff) | (lenght & 0x0000ffff)<<16
    command[20:24] = (word0 & 0xFFFFFFFF).to_bytes(4, 'little')
    command[24:28] = (word1 & 0xFFFFFFFF).to_bytes(4, 'little')

def capture_command(dma2_enable, mode):
    global captureStart, captureEnd,oscilloscopeMode

    if(dma2_enable & 0x1):
         captureStart=1
         captureEnd=0
    else:
        captureStart=0
        captureEnd=1
    if(mode == "burst capture"):
        mode = 0b01
        oscilloscopeMode = 0b01
    elif(mode == "continuous capture"):
        mode=0b11
        oscilloscopeMode = 0b11
    elif(mode =="true continuous"):
        mode = 0b10
        oscilloscopeMode =  0b10
    else:
        mode = 0b00
        oscilloscopeMode = 0b00

    word = 0x00000000
    word = word | (dma2_enable & 0x1) | (mode & 0x3)<<1
    command[28] = word

#V_out = 3 V × code / 4095

# l = how many points we stored k= how many peroids are in the table arr = f= 84Mhz/(arr+1)
# 1- f_table = fs / L; 2- fs = 84 MHz / (ARR + 1); 3- f_sin = k · f_table all together f_sin = k · 84 MHz / ((ARR + 1) · L)
# arr can be minimum 83 cause dac has max 1Mhz clock, limits of amplitude are ignored here but each lsb takes some ns to go stm does give a slew rate
# lets brute force this thing yehhh
TIM7_CLOCK = 84000000

def dac2_find_parameters(fsin):
    prev_error= 100000000000
    prev_point_density=0
    l_k_arr = [0,0,0]
    for k in range(1,33):
        for l in range(10*k,4096):
            new_point_density = l/k
            arr = round(TIM7_CLOCK * k / (fsin * l)) - 1
            if(arr<84 or arr>65535):
                continue
            f_real  = TIM7_CLOCK * k / ((arr+1) * l)
            new_error = abs(f_real-fsin)
            if(new_error<prev_error):
                prev_point_density = new_point_density
                prev_error = new_error
                l_k_arr =[l,k,arr]
            elif(new_error==prev_error and new_point_density>prev_point_density ):
                prev_point_density = new_point_density
                prev_error = new_error
                l_k_arr =[l,k,arr]

    return l_k_arr


#code[n] = round( DC + A · sin(2π · n / N) ), n = 0 … N−1
#v[n] = dc + amp · sin(2π · f · n / fs), v[n] = dc + amp · sin(2π · k · n / l), t = n/fs
def dac2_table_generator(amp, f, dc, mode):
    table = bytearray(8192)
    l_k_arr = dac2_find_parameters(f)
    l, k, arr = l_k_arr
    if l == 0:
        raise ValueError(f"can't make {f} Hz")
    if(mode=="sine"):
        for i in range(0,l):
            value = round(dc + amp * math.sin(2 * math.pi * k * i / l))
            value = max(0, min(4095, value))
            table[2*i : 2*i+2] = value.to_bytes(2, 'little')

    elif(mode=="square"):
        high = max(0, min(4095, dc + amp))
        low  = max(0, min(4095, dc - amp))
        for n in range(l):
                phase = (k * (n ) / l) % 1        # position inside the period, 0 … <1
                value = high if phase < 0.5 else low
                table[2*n : 2*n+2] = value.to_bytes(2, 'little')

    elif(mode=="triangle"):
        for n in range(l):
            phase = (k * n / l) % 1
            if phase < 0.5:
                shape = -1 + 4 * phase          # rising
            else:
                shape = 3 - 4 * phase           # falling
            value = round(dc + amp * shape)
            value = max(0, min(4095, value))
            table[2*n : 2*n+2] = value.to_bytes(2, 'little')

    elif(mode=="sawtooth"):
        for n in range(l):
            phase = (k * n / l) % 1
            shape = -1 + 2 * phase              # rising ramp
            value = round(dc + amp * shape)
            value = max(0, min(4095, value))
            table[2*n : 2*n+2] = value.to_bytes(2, 'little')

    return table, l, arr


#------------------------------ start of send function----------------------------------#

# i will brach it this way: we either have to upload a dac table or not than it branches into 3 possible capture mods so 6 possible outcome
def send_command(dev):
    time.sleep(0.01)
# ----------------------------- table update start --------------------------------#
    global tableUploadStart, tableUploadEnd, captureStart, captureEnd, oscilloscopeMode
    if(tableUploadStart):

        #ep1 out to call address is 0x01, not the actaull address but its just for calling according to the usb specification
        dev.write(0x01, command)
        # wait until device has read the command and sent the zlp
        while True:
            # ep1 In is 0x81
            data = dev.read(0x81, 64)
            if len(data) == 0:           # ZLP
                break

        dev.write(0x01, dac2_table)
        time.sleep(0.1)
        command[20] &= 0xFE # table update bit
        tableUploadEnd=1
        tableUploadStart=0
        # see if we requested capture and save the adc data
        if(captureStart):
            # burst capture
            if(oscilloscopeMode == 0b01):
                try:
                    data = dev.read(0x81, 122624 + 64, timeout=2000)   # ends at the ZLP
                except usb.core.USBTimeoutError:
                    raise RuntimeError("capture: no data within 2 s")
                if len(data) != 122624:
                    raise RuntimeError(f"got {len(data)} bytes")

                captureStart=0
                captureEnd=1
                command[28] &= 0xFE          # clear capture enable, keep mode bits
                return data

            if(oscilloscopeMode == 0b10): return -1
            if(oscilloscopeMode == 0b11): return -1
            return 0

        else: return 0

 # ----------------------------- table update end --------------------------------#


 # ----------------------------- table no update  start --------------------------------#

    elif(tableUploadStart == 0):
        dev.write(0x01, command)
        time.sleep(0.1)
        if(captureStart):
            # burst capture
            if(oscilloscopeMode == 0b01):
                try:
                    data = dev.read(0x81, 122624 + 64, timeout=2000)   # ends at the ZLP
                except usb.core.USBTimeoutError:
                    raise RuntimeError("capture: no data within 2 s")
                if len(data) != 122624:
                    raise RuntimeError(f"got {len(data)} bytes")

                captureStart=0
                captureEnd=1
                command[28] &= 0xFE          # clear capture enable, keep mode bits
                return data

            elif(oscilloscopeMode == 0b10): return -1
            elif(oscilloscopeMode == 0b11): return -1
            else: return 0

        else: return 0

    else: return -1

 # ----------------------------- table no update  end --------------------------------#
 #------------------------------ end of send function----------------------------------#

def volts_to_code(v):
    code = round(v / 3 * 4095)
    return max(0, min(4095, code))

def code_to_volts(code):
    return code * 3 / 4095

# every enable bit to 0, so a command only does what we turn on after this
def clear_enables():
    global tableUploadStart, tableUploadEnd, captureStart, captureEnd
    command[4]  &= 0xF7      # word 1 bit 3: mux update
    command[8]  &= 0xFC      # word 2 bit 0: dac1 update, bit 1: dac1 turn off
    command[20] &= 0xF8      # word 5 bit 0: table_update, bit 1: dac2 turn off, bit 2: dac2 update
    command[28] &= 0xFE      # word 7 bit 0: capture enable
    command[32] &= 0xFE      # word 8 bit 0: adc clock update
    tableUploadStart = 0
    tableUploadEnd = 1
    captureStart = 0
    captureEnd = 1

# firmware only reads turn off when update is 1, so set both
def dac1_turn_off():
    command[8] |= 0x03       # bit 0 update, bit 1 turn off

def dac2_turn_off():
    command[20] |= 0x06      # bit 1 turn off, bit 2 dac2 update

# ADC clock: word 8 bit 0 = update, bits 2:1 = PA8 speed; word 9 bits 15:0 = arr, bits 31:16 = ccr2
# PA8 speed (OSPEEDR8): 00 low, 01 medium, 10 fast, 11 high
# sample rate = 168 MHz / (arr + 1)
def adc_clock_set(ccr2, arr, speed, doWeUpdate):
    word0 = 0x00000000
    word0 = word0 | (doWeUpdate & 0x1)
    if(speed == "low (2 MHz)"):
        word0 = word0 | 0b00<<1
    elif(speed == "medium (25 MHz)"):
        word0 = word0 | 0b01<<1
    elif(speed == "fast (50 MHz)"):
        word0 = word0 | 0b10<<1
    elif(speed == "high (100 MHz)"):
        word0 = word0 | 0b11<<1
    else:
        raise ValueError(f"unknown PA8 speed {speed}")
    word1 = (arr & 0xffff) | (ccr2 & 0xffff) << 16
    command[32:36] = (word0 & 0xFFFFFFFF).to_bytes(4, 'little')
    command[36:40] = (word1 & 0xFFFFFFFF).to_bytes(4, 'little')

# PD0 carries the ADC MSB and PD7 the LSB, so every sample byte is bit reversed
# table: BIT_REVERSE[x] = x with bit 0..7 flipped, built once
BIT_REVERSE = np.array([int(f'{i:08b}'[::-1], 2) for i in range(256)], dtype=np.uint8)

def bit_reverse(data):
    return BIT_REVERSE[np.frombuffer(data, dtype=np.uint8)]
