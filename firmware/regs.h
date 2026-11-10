#ifndef regs_h
#define regs_h

#include <stdint.h>



#define FLASH_ASR ((volatile uint32_t *)0x40023C00)

#define GPIOB ((GPIOB_PORTB*)0x40020400)
typedef struct GPIO_PORTB
{
    volatile uint32_t MODER;   // 0x00000280;
    volatile uint32_t OTYPER;  //0x00000000;
    volatile uint32_t OSPEEDR; //0x000000C0;
    volatile uint32_t PUPDR ;  //0x00000100;
    volatile uint32_t IDR;
    volatile uint32_t ODR  ;   //0x00000000;
    volatile uint32_t BSRR ;   //0x00000000;
    volatile uint32_t LCKR ;   //0x00000000;
    volatile uint32_t AFRL ;   //0x00000000;
    volatile uint32_t AFRH ;   //0x00000000;

} GPIOB_PORTB;

#define GPIOA ((GPIOA_PORTA*)0x40020000)
typedef struct GPIO_PORTA
{
    volatile uint32_t MODER;   // 0xA8000000;
    volatile uint32_t OTYPER;  // 0x00000000;
    volatile uint32_t OSPEEDR; // 0x0C000000;
    volatile uint32_t PUPDR;   // 0x64000000;
    volatile uint32_t IDR;
    volatile uint32_t ODR;     // 0x00000000;
    volatile uint32_t BSRR;    // 0x00000000;
    volatile uint32_t LCKR;    // 0x00000000;
    volatile uint32_t AFRL;    // 0x00000000;
    volatile uint32_t AFRH;    // 0x00000000;

} GPIOA_PORTA;

#define STK ((stk_registers*)0xE000E010)
typedef struct stk_registers
{
    volatile uint32_t CTRL;
    volatile uint32_t LOAD; 
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} stk_registers;


#define RCC ((rcc_registers*)0x40023800)
typedef struct rcc_registers
{
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    volatile uint32_t skip0[1];
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR; 
    volatile uint32_t skip[2];
    volatile uint32_t AHB1ENR; 
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    volatile uint32_t skip2[1];
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t skip3[2];
    volatile uint32_t AHB1LPENR;
    volatile uint32_t AHB2LPENR;
    volatile uint32_t AHB3LPENR;
    volatile uint32_t skip4[1];
    volatile uint32_t APB1LPENR;
    volatile uint32_t APB2LPENR;
    volatile uint32_t skip5[2];
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
    volatile uint32_t skip6[2];
    volatile uint32_t SSCGR;
    volatile uint32_t PLLI2SCFGR;
    volatile uint32_t skip7[1];
    volatile uint32_t DCKCFGR;
} rcc_registers;

#define TIM3 ((tim3_registers *)0x40000400)
typedef struct tim3_registers
{
    volatile uint32_t CR1;        /* 0x00 control 1 */
    volatile uint32_t CR2;        /* 0x04 control 2 */
    volatile uint32_t SMCR;       /* 0x08 slave mode control */
    volatile uint32_t DIER;       /* 0x0C DMA/interrupt enable */
    volatile uint32_t SR;         /* 0x10 status */
    volatile uint32_t EGR;        /* 0x14 event generation */
    volatile uint32_t CCMR1;      /* 0x18 capture/compare mode, ch 1+2 */
    volatile uint32_t CCMR2;      /* 0x1C capture/compare mode, ch 3+4 */
    volatile uint32_t CCER;       /* 0x20 capture/compare enable */
    volatile uint32_t CNT;        /* 0x24 counter */
    volatile uint32_t PSC;        /* 0x28 prescaler */
    volatile uint32_t ARR;        /* 0x2C auto-reload */
    volatile uint32_t RCR;  /* 0x30 (RCR on TIM1/TIM8 only) */
    volatile uint32_t CCR1;       /* 0x34 capture/compare 1 */
    volatile uint32_t CCR2;       /* 0x38 capture/compare 2 */
    volatile uint32_t CCR3;       /* 0x3C capture/compare 3 */
    volatile uint32_t CCR4;       /* 0x40 capture/compare 4 */
    volatile uint32_t BDTR;  /* 0x44 */
    volatile uint32_t DCR;        /* 0x48 DMA control */
    volatile uint32_t DMAR;       /* 0x4C DMA address for burst */
} tim3_registers;

#define NVIC ((nvic_registers*)0xE000E100)
typedef struct nvic_registers
{
    volatile uint32_t ISER[8];        // 0x000 set-enable
    uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];        // 0x080 clear-enable
    uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];        // 0x100 set-pending
    uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];        // 0x180 clear-pending
    uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];        // 0x200 active bit
    uint32_t RESERVED4[56];
    volatile uint8_t  IPR[240];       // 0x300 priority, one byte per IRQ
    uint32_t RESERVED5[644];
    volatile uint32_t STIR;           // 0xE00 software trigger
} nvic_registers;
#define DAC ((dac_registers*)0x40007400)
typedef struct dac_registers
{
    volatile uint32_t CR;          // 0x00 control
    volatile uint32_t SWTRIGR;     // 0x04 software trigger
    volatile uint32_t DHR12R1;     // 0x08 ch1 data holding, 12 bit right aligned
    volatile uint32_t DHR12L1;     // 0x0C ch1 data holding, 12 bit left aligned
    volatile uint32_t DHR8R1;      // 0x10 ch1 data holding, 8 bit right aligned
    volatile uint32_t DHR12R2;     // 0x14 ch2 data holding, 12 bit right aligned
    volatile uint32_t DHR12L2;     // 0x18 ch2 data holding, 12 bit left aligned
    volatile uint32_t DHR8R2;      // 0x1C ch2 data holding, 8 bit right aligned
    volatile uint32_t DHR12RD;     // 0x20 dual data holding, 12 bit right aligned
    volatile uint32_t DHR12LD;     // 0x24 dual data holding, 12 bit left aligned
    volatile uint32_t DHR8RD;      // 0x28 dual data holding, 8 bit right aligned
    volatile uint32_t DOR1;        // 0x2C ch1 data output, read only
    volatile uint32_t DOR2;        // 0x30 ch2 data output, read only
    volatile uint32_t SR;          // 0x34 status (DMA underrun flags)
} dac_registers;
#define DMA1 ((dma_registers*)0x40026000)
#define DMA2 ((dma_registers*)0x40026400)
typedef struct dma_stream
{
    volatile uint32_t CR;      // +0x00 Configuration Register
    volatile uint32_t NDTR;    // +0x04 Number of Data items To transfer Register
    volatile uint32_t PAR;     // +0x08 Peripheral Address Register
    volatile uint32_t M0AR;    // +0x0C Memory 0 Address Register
    volatile uint32_t M1AR;    // +0x10 Memory 1 Address Register (double buffer only)
    volatile uint32_t FCR;     // +0x14 FIFO Control Register
} dma_stream;                  // 0x18 per stream
typedef struct dma_registers
{
    volatile uint32_t LISR;    // 0x00 Low Interrupt Status Register, streams 0-3
    volatile uint32_t HISR;    // 0x04 High Interrupt Status Register, streams 4-7
    volatile uint32_t LIFCR;   // 0x08 Low Interrupt Flag Clear Register
    volatile uint32_t HIFCR;   // 0x0C High Interrupt Flag Clear Register
    dma_stream S[8];           // 0x10 stream n at 0x10 + 0x18*n
} dma_registers;

// TIM1 has the same offsets as TIM3: RESERVED0 (0x30) is RCR, RESERVED1 (0x44) is BDTR
#define TIM1 ((tim3_registers*)0x40010000)
#define TIM6 ((basic_tim_registers*)0x40001000)
#define TIM7 ((basic_tim_registers*)0x40001400)
typedef struct basic_tim_registers
{
    volatile uint32_t CR1;         // 0x00 control 1
    volatile uint32_t CR2;         // 0x04 control 2
    uint32_t RESERVED0;            // 0x08
    volatile uint32_t DIER;        // 0x0C DMA/interrupt enable
    volatile uint32_t SR;          // 0x10 status
    volatile uint32_t EGR;         // 0x14 event generation
    uint32_t RESERVED1[3];         // 0x18 - 0x20
    volatile uint32_t CNT;         // 0x24 counter
    volatile uint32_t PSC;         // 0x28 prescaler
    volatile uint32_t ARR;         // 0x2C auto-reload
} basic_tim_registers;
#endif