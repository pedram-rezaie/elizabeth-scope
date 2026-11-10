#include <stdint.h>
#include"regs.h"
#include "usb.h"

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;
extern void otg_fs_handler(void);
extern volatile uint8_t global_oscilloscope_mode;
extern int main(void);
extern uint32_t spare[64];
extern uint32_t adc_buf[30656];
extern volatile int dma_buffer_is_full;


void reset_handler(void);
void iwilldothislater(void);
void systick_handler(void);
void otg_fs_handler(void);
void dma2_stream2_handler(void);
__attribute__((section(".isr_vector")))
//IRQ + 16
uint32_t const vector_table[98]={
        (uint32_t)&_estack, //estack is an alias of the start address of the stack (top of the CCM here) which is patched by the linker later
        (uint32_t)&reset_handler,
        [2 ... 6] = (uint32_t)&iwilldothislater,
        [7 ... 10] = 0,
        [11 ... 12] = (uint32_t)&iwilldothislater,
        [13] = 0,
        [14] = (uint32_t)&iwilldothislater,
        [15]=(uint32_t)&systick_handler,
        [16 ... 73] = (uint32_t)&iwilldothislater,
        [74] = (uint32_t)&dma2_stream2_handler,
        [75 ... 82] = (uint32_t)&iwilldothislater,
        [83] = (uint32_t)&otg_fs_handler
     };
void reset_handler(void){

    uint32_t* src = &_sidata;
    for(uint32_t* i = &_sdata; i<&_edata;++i){ //copy from tart of initialization data from the flash into start data in ram
        *i= *src;
        ++src;
    }

    for(uint32_t* i= &_sbss; i<&_ebss; ++i){ //copy zeros for evey var that has no defined value at the start
        *i = 0x00000000;
    }

    main();
    while(1);
}
void iwilldothislater(void){
    pb0_on();
    while (1)
    {}
}

extern volatile uint32_t halt_done;
void systick_handler(void){
    halt_done = 1;
}

void dma2_stream2_handler(void){
    // global_oscilloscope_mode = 01 means burst, 10 means continues
    // we are using dma2 steam2
    global_oscilloscope_mode &= 0x03;
    if(global_oscilloscope_mode ==0 ) {pb0_on();}
    
    if(global_oscilloscope_mode == 0x01){
        // if transfer problem accrourd
        if((*DMA2).LISR & 0x00080000){
            while(1){ pb0_on();}
        }
        //transfer complete
        if((*DMA2).LISR & 0x00200000){
            // which transfer u ask?
            if((*DMA2).S[2].CR & 0x00080000){ // bit 19 is high so current memory is memory 1 meaning 0 is full move pointer to trash
                (*DMA2).S[2].M0AR = (uint32_t)&spare;
            }
            else{ // ram is now full  disable dma initiate transfer
                dma_buffer_is_full=1;
                dma2_deactivate();
            }
            // clear flag
            (*DMA2).LIFCR = 0x00200000;
        }
    }
    if(global_oscilloscope_mode == 0x02){

    }
}