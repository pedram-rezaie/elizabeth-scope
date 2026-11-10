#ifndef functions_h
#define functions_h
#include "usb.h"

#include <stdint.h>
#include "regs.h"



// sets the mux value
extern uint32_t cmd_buf[16];
void init(void);
void  mux_set(uint8_t doWeUpdate, uint32_t pb6, uint32_t pb5,uint32_t pb4 );
void mux_reset(void);
void clock_set(void);
void pb0_on(void);
void pb0_blinking(void);
void halt_cpu(uint32_t delay);
void command_read(void);
void dac_init(void);
void dac1_set(uint16_t dc, uint8_t triangle, uint32_t f, uint16_t a, uint8_t turnOff, uint8_t doWeUpdate);
void tim1_init(void);
void dma2_active_burst(void);
void dma1_active(void);
void dma1_deactivate(void);
void dma2_deactivate(void);
void burst_capture(void);
void update_dac2_table(void);
void dac2_activate(uint16_t *table, uint16_t l, uint16_t arr, uint8_t dac2_update, uint8_t turn_off);

#endif 