#ifndef USB_h
#define USB_h
#include <stdint.h>
#include "regs.h"
#include "functions.h"
void usb_enable(void);
void otg_fs_handler(void);
void usb_data_transfer(uint32_t *pointer, uint32_t lenght);
void adc_clock_set(uint16_t ccr2, uint16_t arr, uint8_t speed, uint8_t doWeUpdate);
#define DEMCR      ((volatile uint32_t *)0xE000EDFC)   // Debug Exception and Monitor Control Register
#define DWT_CTRL   ((volatile uint32_t *)0xE0001000)   // Data Watchpoint and Trace ConTRoL
#define DWT_CYCCNT ((volatile uint32_t *)0xE0001004)   // CYCle CouNT, +1 per CPU clock
#define OTG_FS_FIFO1 ((volatile uint32_t*)(0x50002000))   // EP1 TX FIFO

// RM0090, USB OTG_FS register map
typedef struct
{
    volatile uint32_t HCCHAR;     // +0x00
    volatile uint32_t RES0;       // +0x04
    volatile uint32_t HCINT;      // +0x08
    volatile uint32_t HCINTMSK;   // +0x0C
    volatile uint32_t HCTSIZ;     // +0x10
    volatile uint32_t RES1[3];    // +0x14
} OTG_FS_HC;                      // 0x20 per host channel

typedef struct
{
    volatile uint32_t DIEPCTL;    // +0x00
    volatile uint32_t RES0;       // +0x04
    volatile uint32_t DIEPINT;    // +0x08
    volatile uint32_t RES1;       // +0x0C
    volatile uint32_t DIEPTSIZ;   // +0x10
    volatile uint32_t RES2;       // +0x14
    volatile uint32_t DTXFSTS;    // +0x18
    volatile uint32_t RES3;       // +0x1C
} OTG_FS_INEP;                    // 0x20 per IN endpoint


typedef struct
{
    volatile uint32_t DOEPCTL;    // +0x00
    volatile uint32_t RES0;       // +0x04
    volatile uint32_t DOEPINT;    // +0x08
    volatile uint32_t RES1;       // +0x0C
    volatile uint32_t DOEPTSIZ;   // +0x10
    volatile uint32_t RES2[3];    // +0x14
} OTG_FS_OUTEP;                   // 0x20 per OUT endpoint

typedef struct OTG_FS_REGS
{
    /* Core global, 0x000 */
    volatile uint32_t GOTGCTL;     // 0x000
    volatile uint32_t GOTGINT;     // 0x004
    volatile uint32_t GAHBCFG;     // 0x008
    volatile uint32_t GUSBCFG;     // 0x00C
    volatile uint32_t GRSTCTL;     // 0x010
    volatile uint32_t GINTSTS;     // 0x014
    volatile uint32_t GINTMSK;     // 0x018
    volatile uint32_t GRXSTSR;     // 0x01C  read only
    volatile uint32_t GRXSTSP;     // 0x020  read + pop
    volatile uint32_t GRXFSIZ;     // 0x024
    volatile uint32_t DIEPTXF0;    // 0x028  HNPTXFSIZ in host mode
    volatile uint32_t HNPTXSTS;    // 0x02C
    volatile uint32_t RES0[2];     // 0x030
    volatile uint32_t GCCFG;       // 0x038
    volatile uint32_t CID;         // 0x03C
    volatile uint32_t RES1[48];    // 0x040
    volatile uint32_t HPTXFSIZ;    // 0x100
    volatile uint32_t DIEPTXF[3];  // 0x104  DIEPTXF1..3
    volatile uint32_t RES2[188];   // 0x110

    /* Host, 0x400 */
    volatile uint32_t HCFG;        // 0x400
    volatile uint32_t HFIR;        // 0x404
    volatile uint32_t HFNUM;       // 0x408
    volatile uint32_t RES3;        // 0x40C
    volatile uint32_t HPTXSTS;     // 0x410
    volatile uint32_t HAINT;       // 0x414
    volatile uint32_t HAINTMSK;    // 0x418
    volatile uint32_t RES4[9];     // 0x41C
    volatile uint32_t HPRT;        // 0x440
    volatile uint32_t RES5[47];    // 0x444
    OTG_FS_HC         HC[8];       // 0x500  ch n at 0x500 + 0x20*n
    volatile uint32_t RES6[128];   // 0x600

    /* Device, 0x800 */
    volatile uint32_t DCFG;        // 0x800
    volatile uint32_t DCTL;        // 0x804
    volatile uint32_t DSTS;        // 0x808
    volatile uint32_t RES7;        // 0x80C
    volatile uint32_t DIEPMSK;     // 0x810
    volatile uint32_t DOEPMSK;     // 0x814
    volatile uint32_t DAINT;       // 0x818
    volatile uint32_t DAINTMSK;    // 0x81C
    volatile uint32_t RES8[2];     // 0x820
    volatile uint32_t DVBUSDIS;    // 0x828
    volatile uint32_t DVBUSPULSE;  // 0x82C
    volatile uint32_t RES9;        // 0x830
    volatile uint32_t DIEPEMPMSK;  // 0x834
    volatile uint32_t RES10[50];   // 0x838
    OTG_FS_INEP       IEP[4];      // 0x900  IN ep n at 0x900 + 0x20*n
    volatile uint32_t RES11[96];   // 0x980
    OTG_FS_OUTEP      OEP[4];      // 0xB00  OUT ep n at 0xB00 + 0x20*n
    volatile uint32_t RES12[160];  // 0xB80

    /* Power and clock gating, 0xE00 */
    volatile uint32_t PCGCCTL;     // 0xE00

} OTG_FS_REGS;

#define OTG_FS          ((OTG_FS_REGS*)0x50000000)
#define OTG_FS_FIFO  (volatile uint32_t*)(0x50001000) //  FIFO start

#endif 