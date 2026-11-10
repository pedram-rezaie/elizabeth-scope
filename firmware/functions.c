#include "functions.h"
volatile uint8_t global_oscilloscope_mode=0;
volatile int read_command=0;
extern uint16_t dac2_table[4096];
volatile int dma_buffer_is_full=0;
// is dac table updated? start with 1 cause we doint want that interrupt
volatile int is_table_updated =1;
// should we update dac table?
volatile int table_update=0;
void init(void){
    (*RCC).AHB1ENR |= (1<<1);      //enable portb clock
    (void)(*RCC).AHB1ENR;
    (*GPIOB).ODR     = 0x00000060; //pb4,0 low pb5,6 high to select muc input 3
    (*GPIOB).MODER   = 0x00001580;  // enable pb5,4,6 as output
    (*GPIOB).OTYPER  = 0x00000001;  // pb0 as open drain, eveything else as push pull
    (*GPIOB).OSPEEDR = 0x00000000;  // all pins low speed, pb4 normally high tho
    (*GPIOB).PUPDR   = 0x00000000; // no pull down or push up resistor needed, pb3 default is on so it wastes power
    clock_set();
    dac_init();
    pb0_blinking();
    tim1_init();
    dma1_active();
}

void mux_reset(void){
    // pb4 high pb5 high ,pb6 low
    uint32_t pb4 = 0x00100000;
    uint32_t pb5 = 0x00000020;
    uint32_t pb6 = 0x00000040;
    mux_set(1,pb6,pb5,pb4);
}
void mux_set(uint8_t doWeUpdate, uint32_t pb6, uint32_t pb5,uint32_t pb4 ){
    if(!doWeUpdate) return;
    if(pb4 ==1 ) pb4 = 0x10;
    else if(pb4==0) pb4 = 0x100000;
    if(pb5==1) pb5=0x20;
    else if(pb5==0) pb5=0x200000;
    if(pb6==1) pb6=0x40;
    else if(pb6==0) pb6=0x400000;
    //pb4 =MSB pb5 =LSB
    // from least to most amp: 3= x1, 2=2x. 1=3x. 0=4x, 7=5x, 6=6x, 5=7x, 4=8x
    //BSRR bit 0-15 set to 1, bit 16-31 reset to 0
    uint32_t set_value = 0x00000000;
    set_value |= (pb4 | pb5 |pb6);
    GPIOB->BSRR = set_value;
}
// HSE/M -> PLL; PLL*N->VCO; VCO/P->SYSCLK; VCO/Q->USB
volatile int is_clock_enabled = 0;
void clock_set(void){

    // flash read at full speed has to wait at least 5 instructions but we will enable cash and data prefetch
    *FLASH_ASR = 0x00000705;

    // pllm=4 (vco input= 8/4Mhz); plln=168 (vco out= 2*168Mhz); pplp=2 (sysclk vco/2=168Mhz); pllq=7 (usbc vco/q=48mhz); pllsrc =1 (external)
    (*RCC).PLLCFGR &= 0xF03C0000;
    (*RCC).PLLCFGR |= 0x07402A04;

    // ahb bus configuration and etc
    (*RCC).CFGR &= 0x0000030C;
    (*RCC).CFGR |= 0x00089402;

    // bit 16 is HSE clock enable; bit 24 is pll enable; pll has to be confiqured before enabled
    (*RCC).CR |= 0x00010000; 
    while (1) //bit 17 is HSE ready flag
    {
        uint32_t temp = (*RCC).CR;
        temp = temp & 0x00020000;
        if(temp) break;
    }
    (*RCC).CR |= 0x01000000;
    while (1) // bit 25 is PPL ready flag
    {
        uint32_t temp = (*RCC).CR;
        temp = temp & 0x02000000;
        if(temp) break; 
    }
    while (1) //bit 3:2 System clock switch status, 10= PPL
    {
        uint32_t temp = (*RCC).CFGR;
        temp = temp & 0x00000008;
        if(temp) break;
    }
    is_clock_enabled = 1;
}
volatile uint32_t halt_done = 0;
void halt_cpu(uint32_t delay){
    // time delayed = (delay+1)*5.95 ns for 1ms 168000-1
    (*STK).LOAD = delay;
    (*STK).CTRL = 0x00000007;
    while(!halt_done){}
    (*STK).LOAD = 0x00000000;
    (*STK).CTRL = 0x00000000;
    halt_done = 0;
}

// in general in f(a,b,c) a is cinsidred MSB when data is being sent
void command_read(void){
    read_command =0;
    // byte 0 is the password
    // byte 1 is the mux command
    // bit0 = pb4, bit3= doweupdate
    uint32_t mux_command = cmd_buf[1];
    uint32_t  pb4= mux_command & 0x01; uint32_t pb5= mux_command & 0x02; pb5 >>=1;  uint32_t pb6= mux_command & 0x04; pb6 >>=2;
    uint8_t doWeUpadte = mux_command>>3 & 0x01;
    mux_set(doWeUpadte, pb6, pb5, pb4);


    //DAC 1 commands are words 2:4, bit 0 dac update code, bit 1 turnOff, bit 2:17 amp, 18:31 padding, 32:63 f, 64 triangle, 65:80 dc, 81:95 padding
    uint32_t dac_command[3]; dac_command[0] = cmd_buf[2];  dac_command[1] = cmd_buf[3]; dac_command[2] = cmd_buf[4]; 
    uint8_t dac1_doWeUpdate = dac_command[0] & 0x01; uint8_t dac1_turnOff = dac_command[0]>>1 & 0x01; uint16_t amplitude = dac_command[0]>>2 & 0xffff;
    uint32_t frequency = dac_command[1];
    uint8_t triangle = dac_command[2] & 0x01; uint16_t dc_value = dac_command[2] >>1 & 0xffff;
    dac1_set(dc_value,triangle,frequency,amplitude,dac1_turnOff,dac1_doWeUpdate);


    // dac2 uses dma1
    // from lsb-> bit0 table_update, bit 1 dac2_turnOff, bit 2 dac2_update, 3:31 padding, bit 32:47 arr, bit 48:63 lenght of the table
    uint32_t dac2_command[2]; dac2_command[0]= cmd_buf[5]; dac2_command[1] = cmd_buf[6];
    
    table_update = dac2_command[0] & 0x01; uint8_t dac2_turnOff= dac2_command[0]>>1 & 0x01; uint8_t dac2_update = dac2_command[0]>>2 & 0x01;
    uint16_t arr = dac2_command[1]; uint16_t lenght = dac2_command[1]>>16;
    if(table_update==1) update_dac2_table();
    dac2_activate(dac2_table,lenght, arr,dac2_update,dac2_turnOff);


    // bit 0 is enable or disable bit 1:2 is mode 
    uint32_t capture_command = cmd_buf[7];
    uint8_t capture_enable = 0x01 & capture_command; uint8_t capture_mode = 0x02 & capture_command; capture_mode >>=1;
    if(capture_enable==0) dma2_deactivate(); 
    else if(capture_mode == 0x01) burst_capture();
    // usb_data_transfer(cmd_buf,28);
    // ADC clock: word 8 bit 0 update, bits 2:1 PA8 speed; word 9 bits 15:0 arr, bits 31:16 ccr2
    uint8_t clock_update = cmd_buf[8] & 0x01; uint8_t pa8_speed = cmd_buf[8] >> 1 & 0x03;
    uint16_t adc_arr = cmd_buf[9] & 0xFFFF; uint16_t ccr2 = cmd_buf[9] >> 16;
    adc_clock_set(ccr2, adc_arr, pa8_speed, clock_update);

}
void update_dac2_table(void){
    // we need to disable ep1 before arming it again since we already armed it once and accroding to RM0090 page 1134
    if ((*OTG_FS).OEP[1].DOEPCTL & 0x80000000){
        (*OTG_FS).DCTL |= 0x00000200;                         // SGONAK (bit 9): NAK on all OUT endpoints
        while (!((*OTG_FS).GINTSTS & 0x00000080)){}           // GONAKEFF (bit 7): global OUT NAK in effect
        (*OTG_FS).OEP[1].DOEPCTL |= 0x48000000;               // EPDIS (bit 30) | SNAK (bit 27)
        while (!((*OTG_FS).OEP[1].DOEPINT & 0x00000002)){}    // EPDISD (bit 1): disabled, core cleared EPENA
        (*OTG_FS).OEP[1].DOEPINT = 0x00000002;                // clear EPDISD (write 1)
        (*OTG_FS).DCTL |= 0x00000400;                         // CGONAK (bit 10): global OUT NAK off, clears GONAKEFF
    }
    is_table_updated=0;
    //enable ep1 for the table
    (*OTG_FS).OEP[1].DOEPTSIZ = 0x04002000;   // PKTCNT 128 (bits 28:19), XFRSIZ 8192 (bits 18:0)
    (*OTG_FS).OEP[1].DOEPCTL |= 0x84000000;   // EPENA (31) | CNAK (26)
    usb_data_transfer(0,0);
    while (1)
    {
        pb0_on();
        if(is_table_updated==1){ is_table_updated=0; break;}
    }
    pb0_blinking();
}

void dac_init(void){
    (*RCC).AHB1ENR |= 0x00000001;   // GPIOA clock
    (*RCC).APB1ENR |= 0x20000030;   // DAC CLOCK ENABLE, DACEN bit 29, TIM7EN bit 5, TIM6EN bit 4
    (void)(*RCC).APB1ENR;
    (*GPIOA).MODER |= 0x00000F00;   // PA4 = DAC1, PA5 = DAC2, analog (bits 11:8 = 1111)
    // really similar to enabling pb0 as blinking
    // TRGO (TRiGger Output) on every update event: MMS (Master Mode Selection) = 010
    (*TIM6).CR2 = 0x00000020;       // DAC1 trigger
    (*TIM7).CR2 = 0x00000020;       // DAC2 trigger
    // CEN stays 0, each set function starts its own timer

    (*DAC).CR = 0x00000000;         // both channels off, buffers on, no wave, no trigger
    (*DAC).DHR12R1 = 0x00000000;
    (*DAC).DHR12R2 = 0x00000000;
}
 
// we have an external 3v volatge reference and dac is 12bits so 4095=3v
//f comes from PSC and ARR.
// each trigger increses the dac by 1 LSB ( unless we use a value table, the in made dc and taignale generators) so actual f is realated to f and A
// ARR is 16 bits on TIM6 so with pcs 0 smallest f is 1282
// Points per period	Step	At 1 MHz trigger
// 8190 (built-in)	1 LSB	≈ 122 Hz
// 256	32 LSB	≈ 3.9 kHz
// 64	128 LSB	≈ 15.6 kHz 
void dac1_set(uint16_t dc, uint8_t triangle, uint32_t f, uint16_t a,uint8_t turnOff, uint8_t doWeUpdate){
    if(!doWeUpdate) return;
    (*TIM6).CR1 = 0x00000000;                 // stop the trigger
    (*DAC).CR &= 0xFFFF0000;                  // ch1 off (resets the triangle counter), ch2 untouched
    if(turnOff) return;

    if (!triangle){                           // DC only
        (*DAC).DHR12R1 = dc;
        (*DAC).CR |= 0x00000001;              // EN1, TEN1 = 0: DHR goes straight to the pin
        return;
    }

    uint32_t n;                               // MAMP1 code, largest allowed amplitude not above a
    if      (a >= 4095) n = 11;
    else if (a >= 2047) n = 10;
    else if (a >= 1023) n = 9;
    else if (a >= 511)  n = 8;
    else if (a >= 255)  n = 7;
    else if (a >= 127)  n = 6;
    else if (a >= 63)   n = 5;
    else if (a >= 31)   n = 4;
    else if (a >= 15)   n = 3;
    else if (a >= 7)    n = 2;
    else if (a >= 3)    n = 1;
    else                n = 0;
    uint32_t amp = (2u << n) - 1;             // peak-to-peak, counter goes 0 -> amp -> 0
    // math:
    // uint32_t f_trig = 2 * amp * f; chooses the dac steps per second, and we want the timer to count as many times right? so if we have to count 200 steps per second, meaning an update rate of 200hz, we need to being down 84MHz down to 200hz
    // 1: how many dac steps we need per second 2: in the full 84mhz which is tim6 f, how many times we count it
    // 3: f_update = f_tick / (ARR + 1) and f_tick = timer/pcs+1 so arr alone can bring it down till 1282hz
    // largest ARR is 65535 so without pcs mimum steps we can do is 1282
    uint32_t f_trig = 2 * amp * f;            // DAC steps per second
    uint32_t div = (84000000u + f_trig / 2) / f_trig;   // 84 MHz ticks per step, rounded
    uint32_t psc = (div - 1) / 65536;         // 0 unless div > 65536
    uint32_t arr = div / (psc + 1) - 1;

    (*TIM6).PSC = psc;
    (*TIM6).ARR = arr;
    (*TIM6).EGR = 0x00000001;                 // UG: load PSC now, CNT = 0

    (*DAC).DHR12R1 = dc - (amp >> 1);         // bottom of the triangle, so it's centered on dc
    (*DAC).CR |= (n << 8) | 0x00000084;       // MAMP1 = n, WAVE1 = 10, TSEL1 = 000 (TIM6), TEN1
    (*DAC).CR |= 0x00000001;                  // EN1 after MAMP1 is set
    (*TIM6).CR1 = 0x00000001;                 // CEN
}
void tim1_init(void){
    //CH1: OC1M (Output Compare 1 Mode) = 110, PWM mode 1. OC1REF (Output Compare 1 REFerence) is high while CNT < CCR1, and CC1E (Capture/Compare 1 output Enable) sends it to PA8. That level is your duty cycle.
    // CH2: OC2M = 000, frozen. OC2REF never changes, and CC2E = 0, so nothing reaches a pin. What’s left is the match event itself. At CNT = CCR2 the timer sets CC2IF (Capture/Compare 2 Interrupt Flag), and with CC2DE (Capture/Compare 2 DMA request Enable) on, that event is the DMA request.
    // set pa8 as alterante funcrion
    (*RCC).AHB1ENR |= 0x00000001;
    (void)(*RCC).AHB1ENR;
    (*GPIOA).MODER   &= ~0x00030000;
    (*GPIOA).MODER   |=  0x00020000;
    (*GPIOA).AFRH    &= ~0x0000000F;
    (*GPIOA).AFRH    |=  0x00000001;
    (*GPIOA).OSPEEDR = ((*GPIOA).OSPEEDR & ~0x00030000) | 0x00010000; // set pa8 as medium speed
    // tim 1 at at 16.8 Mhz using PSC=0 and adr=9 so 168/10= 16/8 using pmw mod at CCR1= 5  In PWM mode 1, OC1REF is high while CNT < CCR1 so we get a 50 precent duity cycle
    (*RCC).APB2ENR |= 0x00000001;
    (void)(*RCC).APB2ENR;
    // control register1 we use this so when we update mid program timers doeslt glitch
    (*TIM1).CR1   = 0x00000084;
    (*TIM1).PSC   = 0;
    (*TIM1).ARR   = 9;  
    (*TIM1).CCR1  = 5;
    // pwm 1, channel1 an output
    (*TIM1).CCMR1 = 0x00000068;
    //CC1E enable which connects to pa8
    (*TIM1).CCER  = 0x00000001;
    //(Break and Dead-Time Register just a safty switch
    (*TIM1).BDTR  = 0x00008000;
    // generate an event so eveything is reseted
    (*TIM1).EGR   = 0x00000001;
    // enable counter
    (*TIM1).CR1  |= 0x00000001;
    // channel 2 for fdma:
    // we can change the phase shift with this bad boy
    (*TIM1).CCR2   = 5;
    (*TIM1).CCMR1 |= 0x00000800;
    (*TIM1).BDTR  = 0x00008000;
    (*TIM1).EGR   = 0x00000001;
    (*TIM1).CR1  |= 0x00000001;  
    // CC2DE for later  
}
__attribute__((section(".dma_input"))) uint32_t adc_buf[30656];
__attribute__((section(".dma_input"))) uint32_t spare[64];
__attribute__((section(".dac_table"))) uint16_t dac2_table[4096];

// we will use channel 6 stream 2 trigered by TIM1_CH2 
void dma2_active_burst(void){
    global_oscilloscope_mode = 01;
    (*RCC).AHB1ENR |= 0x00400008;
    (void)(*RCC).AHB1ENR;
    // turn off
    (*DMA2).S[2].CR = 0x00000000;
    // while active pin 1 is high and most things are inactive
    while ((*DMA2).S[2].CR & 0x00000001){}
    // write 1 to inter to clear them before starting
    (*DMA2).LIFCR     = 0x003D0000;
    // address ofg wherto read ( idr of the pins)
    (*DMA2).S[2].PAR  = 0x40020C10;
    // pointer 1 start address
    (*DMA2).S[2].M0AR = (uint32_t)&adc_buf[0];
    (*DMA2).S[2].M1AR = (uint32_t)&adc_buf[15328];
    //numbrt of transfers per transfer
    (*DMA2).S[2].NDTR = 61312;
    // we will turn on the fifo since each word written i 4 bytes and adc output is 8 bits
    (*DMA2).S[2].FCR  = 0x00000004;
    // control registers of stream 2: channel 6, double buffer, psize 00 means 8 bits, write 32bits to ram
    // also enabled tranfer error interrupt
    (*DMA2).S[2].CR   = 0x0C074414;
    //dma 2 stram 2 irq is 58, we will set it at the highest piority
    (*NVIC).IPR[58] = 0x00;
    // this is the usb otg, less important than dma since a dma problem will destroy data 
    (*NVIC).IPR[67] = 0x10;
    // turn on irq 58
    (*NVIC).ISER[1] = 0x04000000;
    
}
void dma1_active(void){    
    (*RCC).AHB1ENR |= 0x00200000;
    (void)(*RCC).AHB1ENR;
    //dma1 stream 6 channel 7 for DAC2
    (*DMA1).S[6].CR = 0x00000000;
    while ((*DMA1).S[6].CR & 0x00000001){}
    //clear inter
    (*DMA1).HIFCR     = 0x003D0000;
    // addres of DHR12R2, dac2 will request dma, dma writes to this address, dac upddates and asks again
    (*DMA1).S[6].PAR  = 0x40007414;
    (*DMA1).S[6].M0AR = (uint32_t)dac2_table;
    // dac is 12bits so read and write half words, but 4 bits are not written so we need padding 
    (*DMA1).S[6].CR   = 0x0E022D40;
}

void burst_capture(void){
    global_oscilloscope_mode = 0x01;       // burst
    dma2_active_burst();                       // full setup every time, dma2_deactivate resets DMA2
    // enable
    (*DMA2).S[2].CR |= 0x00000001;         
    // tim1 channel 2 pls send dma request plsss
    (*TIM1).DIER    |= 0x00000400;         
    while (dma_buffer_is_full != 1){}      // about 7.3 ms, the ISR stops DMA2 and sets the flag
    dma_buffer_is_full = 0;                // set it back for the next capture

    usb_data_transfer(adc_buf, 122624);
    usb_data_transfer(0, 0);
}

void dma2_deactivate(void){
    (*DMA2).S[2].CR &= ~0x00000001;
    while ((*DMA2).S[2].CR & 0x00000001){}
    // turn of tim1 channel 2 dma request
    (*TIM1).DIER &= ~0x00000400;
    //clear irq 58
    (*NVIC).ICER[1] = 0x04000000;
    // make sure there is no pending interrupt latched biting us on the next run
    (*NVIC).ICPR[1] = 0x04000000;
    // hold and reset bit 22, dma2 only
    (*RCC).AHB1RSTR |=  0x00400000;
    (*RCC).AHB1RSTR &= ~0x00400000;
}

void dma1_deactivate(void){
    //disable stream 6
    (*DMA1).S[6].CR &= ~0x00000001;
    while ((*DMA1).S[6].CR & 0x00000001){}
    // stop dac from generating dma requests
    (*DAC).CR &= ~0x10000000;
    // disabling sets TCIF6, clear stream 6 flags so the next EN starts clean
    (*DMA1).HIFCR = 0x003D0000;
}
void dac2_activate(uint16_t *table, uint16_t l, uint16_t arr, uint8_t dac2_update, uint8_t turn_off){
    if(!dac2_update) return;
    // stop the triggers first so nothing moves while we change things
    (*TIM7).CR1 &= ~0x00000001;          // CEN (Counter ENable) off
    dma1_deactivate();                   // stream 6 off, DMAEN2 off, flags cleared
    // channel 2 off and all its bits (29:16) cleared, channel 1 (15:0) untouched
    (*DAC).CR &= 0xC000FFFF;
    if(turn_off) return;

    // DMA1 stream 6: CR and PAR already set by dma1_activate, only where and how long
    (*DMA1).S[6].M0AR = (uint32_t)table;
    (*DMA1).S[6].NDTR = l;
    (*DMA1).S[6].CR  |= 0x00000001;      // EN: armed, waits for DAC requests

    // first trigger outputs whatever sits in DHR12R2, preload the last sample:
    // output goes table[l-1], table[0], table[1] ... no jump at the start
    (*DAC).DHR12R2 = table[l - 1];

    // TIM7 at 84 MHz
    (*TIM7).PSC = 0;
    (*TIM7).ARR = arr;
    (*TIM7).EGR = 0x00000001;            // UG (Update Generation): loads PSC, clears CNT; its TRGO is ignored, DAC trigger still off

    // BOFF2 bit 17 (buffer off), TEN2 bit 18 (trigger enable), TSEL2 = 010 bits 21:19 (TIM7 TRGO), DMAEN2 bit 28
    (*DAC).CR |= 0x10160000;
    // EN2 bit 16 in its own write, TSEL2 can't change while EN2 = 1
    (*DAC).CR |= 0x00010000;

    (*TIM7).CR1 |= 0x00000001;           // CEN: go
}



void pb0_on(void){
    uint32_t mask = 0x00000003;
    (*GPIOB).MODER &= ~(mask);
    (*GPIOB).MODER |= 0x00000001;
    (*TIM3).CR1 = 0x00000000;
}
void pb0_blinking(void){
    // tim3 -> PSC = prescaler; ARR= Auto-Reload Register, how maany steps before it restarts back to 0
    // CCR3: Capture/Compare Register 3: CNT = x, then produces a signal called OC3REF (Output Compare channel 3 REFerence)
    // CCMR2: Capture/Compare Mode Register 2 : what does OC3REF after a match oc3m=011=toggle
    // CCER: Capture/Compare Enable Register decieds if the OC3REF goes anywhere
    //CR1: Control Register 1 : turn on  the timer
    //psc = 8399 so tim f = 84M/8400= 10000hz; psc = 0x000020CF; arr=4999 so after it goes back to 0, arr=0x00001387; ccr3=0x0
    //ccmr2 bit 1:0 control if its an output bit 6:4 chooses what does the OC3REF
    // CCER bit 8:11 is for channel 3
    // pb0 is connected to tim3 throgh channel 3 with af2
    uint32_t mask = 0x00000003;
    (*GPIOB).MODER &= ~(mask);
    (*GPIOB).AFRL  |= 0x00000002;
    (*GPIOB).MODER |= 0x00000002;
    (*RCC).APB1ENR |= 0x00000002;
    (void)(*RCC).APB1ENR; //wait after clock is set
    (*TIM3).PSC = 0x000020CF;
    (*TIM3).ARR = 0x00001387;
    (*TIM3).CCMR2 = 0x00000030;
    (*TIM3).CCER= 0x00000100;
    (*TIM3).CR1 = 0x00000001;
}

// ADC clock, DMA read moment and PA8 edge speed
// f_adc = 168 MHz / (arr + 1); CCR1 (Capture/Compare Register 1) = (arr + 1) / 2 keeps the clock near 50% duty
// ccr2 = where in the period the DMA reads PD0:PD7, 0..arr (0 = rising edge, CCR1 = falling edge)
// ccr2 > arr never matches: no DMA request, burst_capture waits forever
// arr >= 9 (16.8 MHz), DMA2 can't keep up faster; arr 65535 = 2.56 kHz slowest without PSC
// speed: 00 low 2 MHz, 01 medium 25 MHz, 10 fast 50 MHz, 11 high 100 MHz; low is too slow for a 16.8 MHz clock
void adc_clock_set(uint16_t ccr2, uint16_t arr, uint8_t speed, uint8_t doWeUpdate){
    if(!doWeUpdate) return;
    (*TIM1).ARR  = arr;                              // ARR (Auto-Reload Register)
    (*TIM1).CCR1 = (arr + 1) / 2;
    (*TIM1).CCR2 = ccr2;
    (*TIM1).EGR  = 0x00000001;                       // UG (Update Generation): ARPE and OC1PE/OC2PE are on, load the new values now, CNT = 0
    (*GPIOA).OSPEEDR &= ~0x00030000;                 // OSPEEDR8 (bits 17:16) cleared
    (*GPIOA).OSPEEDR |= (uint32_t)(speed & 0x03) << 16;
}
