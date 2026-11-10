#include "usb.h"
static void usbrst_handler(void);
static void enumdne_handler(void);
static void rfxlvl_handler(void);
static void setup_handler(void);
static void ep1_send(void);
static void data_out_handler(uint32_t epnum, uint32_t bcnt);
static void ep0_out_arm(void);
static void ep0_stall(void);
static void ep0_in_arm(uint32_t size);
static void ep1_in_arm(uint32_t packets, uint32_t bytes);
static void ep0_send(const uint8_t *p, uint32_t len);
static void ep1_abort(void);
static const uint8_t dev_desc[20] = {
    0x12, 0x01,       // bLength 18, bDescriptorType DEVICE
    0x01, 0x02,       // bcdUSB 2.01 (was 2.00): Windows asks for the BOS descriptor
    0x00, 0x00, 0x00, // class, subclass, protocol: defined per interface
    0x40,             // bMaxPacketSize0 64
    0x09, 0x12,       // idVendor 0x1209 (pid.codes)
    0x01, 0x00,       // idProduct 0x0001 (test PID)
    0x01, 0x01,       // bcdDevice 1.01 (was 1.00)
    0x00, 0x00, 0x00, // no string descriptors
    0x01,              // bNumConfigurations 1
    0x00, 0x00
};

static const uint8_t cfg_desc[32] = {
    // configuration
    0x09, 0x02,       // bLength 9, bDescriptorType CONFIGURATION
    0x20, 0x00,       // wTotalLength 32
    0x01,             // bNumInterfaces 1
    0x01,             // bConfigurationValue 1 (SET_CONFIGURATION sends this in wValue)
    0x00,             // iConfiguration, no string
    0xC0,             // bmAttributes: bit 7 reserved = 1, bit 6 self-powered = 1, bit 5 no remote wakeup
    0x00,             // bMaxPower 0 mA, nothing on the board runs from VBUS
    // interface 0
    0x09, 0x04,       // bLength 9, bDescriptorType INTERFACE
    0x00,             // bInterfaceNumber 0
    0x00,             // bAlternateSetting 0
    0x02,             // bNumEndpoints 2
    0xFF, 0x00, 0x00, // class vendor, subclass 0, protocol 0
    0x00,             // iInterface, no string
    // EP1 IN
    0x07, 0x05,       // bLength 7, bDescriptorType ENDPOINT
    0x81,             // bEndpointAddress: EP1, IN (bit 7)
    0x02,             // bmAttributes: bulk
    0x40, 0x00,       // wMaxPacketSize 64
    0x00,             // bInterval, ignored for bulk
    // EP1 OUT
    0x07, 0x05,       // bLength 7, bDescriptorType ENDPOINT
    0x01,             // bEndpointAddress: EP1, OUT
    0x02,             // bmAttributes: bulk
    0x40, 0x00,       // wMaxPacketSize 64
    0x00              // bInterval, ignored for bulk
};
static const uint8_t bos_desc[33] = {
    // BOS header
    0x05, 0x0F,             // bLength 5, bDescriptorType BOS (15)
    0x21, 0x00,             // wTotalLength 33
    0x01,                   // bNumDeviceCaps 1
    // platform capability: MS OS 2.0
    0x1C, 0x10, 0x05, 0x00, // bLength 28, DEVICE CAPABILITY (16), PLATFORM (5), reserved
    0xDF, 0x60, 0xDD, 0xD8, // MS OS 2.0 UUID {D8DD60DF-4589-4CC7-9CD2-659D9E648A9F},
    0x89, 0x45, 0xC7, 0x4C, //   first three fields little endian, last 8 bytes as written
    0x9C, 0xD2, 0x65, 0x9D,
    0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06, // dwWindowsVersion 0x06030000 = Windows 8.1 and newer
    0xA2, 0x00,             // wMSOSDescriptorSetTotalLength 162, the set we write in step 2
    0x01,                   // bMS_VendorCode 1: Windows asks for the set with bRequest = 1
    0x00                    // bAltEnumCode 0
};
//Format: Microsoft OS 2.0 Descriptors Specification”
static const uint8_t msos20_desc[162] = {
    // set header
    0x0A, 0x00,             // wLength 10
    0x00, 0x00,             // wDescriptorType 0, SET_HEADER_DESCRIPTOR
    0x00, 0x00, 0x03, 0x06, // dwWindowsVersion 0x06030000, Windows 8.1 and newer
    0xA2, 0x00,             // wTotalLength 162
    // compatible ID
    0x14, 0x00,             // wLength 20
    0x03, 0x00,             // wDescriptorType 3, FEATURE_COMPATIBLE_ID
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,           // CompatibleID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // SubCompatibleID
    // registry property
    0x84, 0x00,             // wLength 132
    0x04, 0x00,             // wDescriptorType 4, FEATURE_REG_PROPERTY
    0x07, 0x00,             // wPropertyDataType 7, REG_MULTI_SZ
    0x2A, 0x00,             // wPropertyNameLength 42
    'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,                // "DeviceInterfaceGUIDs", UTF-16LE
    'I',0,'n',0,'t',0,'e',0,'r',0,'f',0,'a',0,'c',0,'e',0,
    'G',0,'U',0,'I',0,'D',0,'s',0,
    0x00, 0x00,             // end of name
    0x50, 0x00,             // wPropertyDataLength 80
    '{',0,'9',0,'6',0,'F',0,'B',0,'8',0,'F',0,'A',0,'D',0,'-',0,
    '0',0,'1',0,'8',0,'A',0,'-',0,
    '4',0,'E',0,'A',0,'C',0,'-',0,
    '9',0,'4',0,'A',0,'0',0,'-',0,
    '8',0,'7',0,'C',0,'4',0,'3',0,'C',0,'D',0,'6',0,'7',0,'A',0,'7',0,'4',0,'}',0,
    0x00, 0x00,             // end of string
    0x00, 0x00              // end of list (MULTI_SZ)
};
// guid is 96FB8FAD-018A-4EAC-94A0-87C43CD67A74 i used it for my own project but it can be reused not a big project
extern volatile int is_clock_enabled;
extern volatile int read_command;
extern volatile int table_update;
extern uint16_t dac2_table[4096];
extern volatile int is_table_updated;
uint32_t cmd_buf[16];               // 64 bytes, one packet
volatile int  /*global_ep1_message_lenght*/ ge1ml =0;
volatile uint32_t*  /*global_ep1_message_pointer*/ ge1mp;
volatile int /*gloabal_ep1_armed_message_lenght*/ ge1aml=0;
static inline uint32_t min(uint32_t a, uint32_t b){ return a < b ? a : b; }
void usb_enable(void){
    if(!is_clock_enabled) clock_set();
    (*RCC).AHB1ENR |= 0x00000001; // port a clock enable
    (void)(*RCC).AHB1ENR;
    (*RCC).AHB2ENR |= 0x00000080; // otg_fs clock enable
    (void)((*RCC).AHB2ENR);
    //PA 11, 12 are usb dp dn using af10
    (*GPIOA).AFRH |= 0x000AA000;
    uint32_t mask = 0x03C00000;
    (*GPIOA).MODER &= ~(mask);
    (*GPIOA).MODER |=0x02800000;
    (*GPIOA).OSPEEDR |= 0x03C00000;
//reset the usb
    while (1)
    {
        // make sure the core is idle 
        uint32_t temp = (*OTG_FS).GRSTCTL;
        uint32_t mask = 0x80000000;
        temp &= mask;
        if(temp){ 
            // fifo reset
            (*OTG_FS).GRSTCTL |= 0x00000030;

            // if idle soft reset
            (*OTG_FS).GRSTCTL |= 0x00000001;
            //
            
            while (1)
        {
            // wait until reset is done
            uint32_t temp2 = (*OTG_FS).GRSTCTL;
            uint32_t mask2 = 0x00000001;
            temp2 &= mask2;
            if(!temp2) break;
        }
        break;
                }                     
    }
    // force stm into device mod and sets USB turnaround time to 6
    (*OTG_FS).GUSBCFG  = 0x00000000;
    (*OTG_FS).GUSBCFG |= 0x40000000;
    halt_cpu(4199999);
    (*OTG_FS).GUSBCFG |= 0x00001800;
    // turn on the PHY and the bus sensing as device
    (*OTG_FS).GCCFG= 0x00210000;
    // make usb full speed
    (*OTG_FS).DCFG |= 0x00000003;
    // interrupt enable USBRST | ENUMDNE
    (*OTG_FS).GINTMSK = 0x00003000;
    // global enable, TTXFELVL will be enabled only when data transfer is being done
    (*OTG_FS).GAHBCFG = 0x00000001;

    // set size of recive fifo
    (*OTG_FS).GRXFSIZ = 0x80;
    //ep0 IN size and start address
    (*OTG_FS).DIEPTXF0 = 0x00400080;
    // ep1 IN size and start address
    (*OTG_FS).DIEPTXF[0] = 0x008000C0; 
    // enable IRQ 67, this is the last gate to fire the inter
    
    (*NVIC).ISER[2] = 0x00000008;


}
                    
//     DOEPMSK/DIEPMSK  (which events)
//   AND DAINTMSK   (which endpoints)
//   AND GINTMSK 18/19
//   AND GAHBCFG bit 0
//   AND NVIC ISER2 bit 3
//     GINTSTS
//  ├─ OEPINT (bit 19): "some OUT endpoint has an event"
//  │    └─ DAINT bits 31:16: which OUT endpoint (bit 16 = EP0, bit 17 = EP1...)
//  │         └─ DOEPINTx: which event (STUP, XFRC, EPDISD...)
//  ├─ IEPINT (bit 18): "some IN endpoint has an event"
//  │    └─ DAINT bits 15:0: which IN endpoint (bit 0 = EP0...)
//  │         └─ DIEPINTx: which event (XFRC, TOC, TXFE...)
//  └─ RXFLVL (bit 4): "a packet is waiting in the RX FIFO"
//       └─ GRXSTSP: which endpoint (EPNUM), what kind (PKTSTS), how big (BCNT)
void otg_fs_handler(void){   
    // check for interrupts that are there and also enabled
    uint32_t staus = (*OTG_FS).GINTMSK & (*OTG_FS).GINTSTS;
    // USBRST, bit 12
    if (staus & 0x00001000){
        usbrst_handler();
    }
    //  ENUMDNE, bit 13
    if(staus & 0x00002000){
        enumdne_handler();
    } 
    //RXFLVL, bit 4, packate has arrived
    if(staus & 0x00000010){
        rfxlvl_handler();
    }
    if (staus & 0x00080000){                          // OEPINT;
    }
    if (staus & 0x00040000){                          // IEPINT
        //DAINT bit 1 (Device All endpoints INTerrupt, EP1 IN) if DAINT bit 1 is set check DIEPINT1 to see what happend
        uint32_t daint = (*OTG_FS).DAINT;
        if (daint & 0x00000002){
            // uint32_t f = (*OTG_FS).IEP[1].DIEPINT;              // which event(s)
            // TXFE handler
            if (!(ge1aml>0)){
                (*OTG_FS).GINTMSK &= 0xFFFBFFFF;
                (*OTG_FS).DIEPEMPMSK &= 0xFFFFFFFD;
            }            
            ep1_send();  
        }
    }  
}
void usbrst_handler(void){
    *DEMCR    |= 0x01000000;   // TRCENA (bit 24): DWT block on
    *DWT_CTRL |= 0x00000001;   // CYCCNTENA (bit 0): cycle counter running
    (*OTG_FS).DIEPMSK = 0x00000000;    // no in end point even allowed expect 1
    (*OTG_FS).DAINTMSK = 0x00000000;   // no  in point is allowed ti interrupt
        // all out points have to NAK
    (*OTG_FS).OEP[1].DOEPCTL |= 0x08000000;
    (*OTG_FS).OEP[2].DOEPCTL |= 0x08000000; 
    (*OTG_FS).OEP[3].DOEPCTL |= 0x08000000;  
    //DAINTMSK controlls which end points are allowed to send interupts both IN and OUT all eps
    // (*OTG_FS).DAINTMSK =0x00010001;
    // DOEPMSK  controls which out interrupts are allowed at all, XFRCM	OUT transfer complete, STUPM	SETUP phase done (control EPs only)
    (*OTG_FS).DOEPMSK =0x00000009;
    // DIEPMSK controls which in interrupts are allowed at all here XFRCM (Transfer complete) TOM (Timeout condition)
    // (*OTG_FS).DIEPMSK = 0x00000000;
    //GRXFSIZ decides the size of the sets the size of the RX FIFO each word is 4 byets and max 330 words space in ram
    (*OTG_FS).GRXFSIZ = 128;
    //Device IN EndPoint TX FIFO, endpoint 0, DIEPTXF0 controlls start and end of ep0 in
    (*OTG_FS).DIEPTXF0 = 0x00400080;
    //DOEPTSIZ0 controls describes what EP0 should expect to receive from the host STUPCNT, bits 30:29 XFRSIZ = how many bytes you expect PKTCNT = in how many packets (always 1 on EP0)
    (*OTG_FS).OEP[0].DOEPTSIZ = 0x60000000;
    // set address to 0, device full speed
    (*OTG_FS).DCFG = 0x02200003;
    (*OTG_FS).GRSTCTL = 0x00000420;                 // TXFFLSH (bit 5), TXFNUM = 0x10 (bits 10:6) = all TX FIFOs
    while ((*OTG_FS).GRSTCTL & 0x00000020){}        // wait until the flush is done
    (*OTG_FS).DIEPEMPMSK = 0x00000000;              // no TXFE requests
    // reset flag to 0 // DIEPEMPMSK only controls Transmit FIFO Empty 
    (*OTG_FS).GINTSTS = 0x00001000;
}
void enumdne_handler(void){
    // evice IN EndPoint ConTroL register, DIEPCTL0 is the control register for IN endpoint 0, 1:0	MPSIZ controls max packet size for both in and out
    (*OTG_FS).IEP->DIEPCTL = 0x00000000;
    // on GINTMSK bit 4: RXFLVL,RxFIFO not empty: the host sent something and it’s waiting;  bit 18: IEPINT	some IN endpoint has an event (check DAINT, then DIEPINTx); bit 19: OEPINT	some OUT endpoint has an event (check DAINT, then DOEPINTx)
    (*OTG_FS).GINTMSK |= 0x00000010; 
    (*OTG_FS).GINTSTS = 0x00002000; // writing 1 to GINTSTS clears that interrupt
}
uint32_t setup[2];
void rfxlvl_handler(void){
    // read the status
    uint32_t status = (*OTG_FS).GRXSTSP;
    uint32_t epnum  = (status & 0x0000000F) >> 0;    // Endpoint number
    uint32_t bcnt   = (status & 0x00007FF0) >> 4;    // size of the package 
    // uint32_t dpid   = (status & 0x00018000) >> 15;   // Data PID
    uint32_t pktsts = (status & 0x001E0000) >> 17;   // Packet status
    // uint32_t frmnum = (status & 0x01E00000) >> 21;   // Frame number
    switch (pktsts){
    // out data arrived
    case 0x2: data_out_handler(epnum,bcnt); break;
    // OUT transfer complete: packet read and handled, take the next one
    case 0x3:
        if (epnum == 1){
            // ep1_out_arm
            (*OTG_FS).OEP[1].DOEPTSIZ = 0x00080040;   // PKTCNT 1 (bit 19), XFRSIZ 64
            (*OTG_FS).OEP[1].DOEPCTL |= 0x84000000;   // EPENA (31) | CNAK (26)
        }
        break;
    // SETUP data arrived
    case 0x6:setup[0] = *(OTG_FS_FIFO); setup[1] = *(OTG_FS_FIFO);   break;
    // setup done
    case 0x4: setup_handler(); ep0_out_arm(); break;
    default: break;
    }
}
// FIFO words are little endian, so byte 0 is bits 7:0 of setup[0].
// bmRequestType	setup[0] bits 7:0
// bRequest	setup[0] bits 15:8
// wValue	setup[0] bits 31:16
// wIndex	setup[1] bits 15:0
// wLength	setup[1] bits 31:16
// GET_DESCRIPTOR (bRequest = 6)
// SET_ADDRESS (bRequest = 5)
static void setup_handler(void){
    uint8_t  bmRequestType = setup[0] & 0xFF;
    uint8_t  bRequest      = (setup[0] >> 8) & 0xFF;
    uint16_t wValue        = (setup[0] >> 16) & 0xFFFF;
    uint16_t wIndex        = setup[1] & 0xFFFF;
    uint16_t wLength       = (setup[1] >> 16) & 0xFFFF;
    switch ((bmRequestType >> 5) & 0x03){
        // standard requests (type 0)
        case 0:
        {
            switch (bRequest)
            {
            // SET_CONFIGURATION   
            case 9: 
            {  
            // se only have confiquartion 1 for this stm so only wValue 1 means succses and confiqure
                if (wValue == 1){
                    // DIEPCTL controls 
                    (*OTG_FS).IEP[1].DIEPCTL  = 0x18488040;   // EP1 IN: bulk, 64, TX FIFO 1, DATA0, NAK
                    (*OTG_FS).OEP[1].DOEPTSIZ = 0x00080040;   // arms EP1 OUT: 1 packet, 64 bytes
                    (*OTG_FS).OEP[1].DOEPCTL  = 0x94088040;   // EP1 OUT: bulk, 64, DATA0, CNAK, EPENA
                    // only ep0,1 are alowed to send interrupts
                    (*OTG_FS).DAINTMSK        = 0x00000002;   // only EP1 IN may interrupt. EP0 IN stays polled because bit 0 is 0, EP out needs no interrupt rfxlvl already handles eveything
                    ep0_send(0, 0);                           // status ZLP
                } else if (wValue == 0){
                    ep0_send(0, 0);                           // unconfigure, just ack for now
                } else {
                    ep0_stall();                              // only configuration 1 exists
                }
                break;
            }
            // GET_DESCRIPTOR
            case 6:  
            {
            // wValue high byte = descriptor type (1 device, 2 configuration, 3 string, 6 device qualifier, ...)
            // wValue low byte = index (which string, which configuration
                switch (wValue >> 8){
                    case 1: ep0_send(dev_desc,min(18, wLength)); break;
                    case 2: ep0_send(cfg_desc, min(32, wLength)); break;
                    case 15: ep0_send(bos_desc,min(33, wLength)); break;   // BOS
                    default: ep0_stall(); break;            // 3 string, 6 device qualifier
                    }
                break;
            }
            // SET_ADDRESS
            case 5:
            {  
                (*OTG_FS).DCFG &= 0xFFFFF80F;           // clear DAD (bits 10:4)
                (*OTG_FS).DCFG |= (wValue) << 4;   // new address
                ep0_send(0,0);                           // ZLP 
                break;
            }
            default: ep0_stall();
                break;
            }
            break;
        }
        // vendor request(type 2)
        case 2:
        {
            if (bRequest == 1 && wIndex == 7) ep0_send(msos20_desc, min(162, wLength));
            else ep0_stall();
            break;
        }
        // we dont support that, cause honesstly its hard to implement and we dont really need it
        default: ep0_stall(); break;
    }
}
static void ep0_out_arm(void){
    (*OTG_FS).OEP[0].DOEPTSIZ = 0x60080040;  // STUPCNT 3 (30:29), PKTCNT 1 (19), XFRSIZ 64
    (*OTG_FS).OEP[0].DOEPCTL |= 0x84000000;  // EPENA (31) | CNAK (26)
}
static void ep0_stall(void){
    // DIEPCTL0 STALL bit 21, core clears it on the next SETUP
    (*OTG_FS).IEP[0].DIEPCTL |= 0x00200000;
}
static void ep0_in_arm(uint32_t size){
    (*OTG_FS).IEP[0].DIEPTSIZ = 0x00080000 | size;   // PKTCNT 1 (19), XFRSIZ size (6:0)
    (*OTG_FS).IEP[0].DIEPCTL |= 0x84000000;          // EPENA (31) | CNAK (26)
}
static void ep0_send(const uint8_t *pointer, uint32_t len){
    (*OTG_FS).IEP[0].DIEPINT = 0x00000001;   // clear XFRC (write 1 to clear)
    ep0_in_arm(len>64 ? 64 : len);
    int i = 64;
    if (len<64) i=len;
    len = len - i;
    while (i>0)
    {
        uint32_t word = 0x00;
        word |= (uint32_t)*pointer;       ++pointer; --i; if(i<=0) goto done;
        word |= (uint32_t)*pointer << 8;  ++pointer; --i; if(i<=0) goto done;
        word |= (uint32_t)*pointer << 16; ++pointer; --i; if(i<=0) goto done;
        word |= (uint32_t)*pointer << 24; ++pointer; --i; if(i<=0) goto done;
        done: (*OTG_FS_FIFO) = word;
    }
    //// wait for XFRC (bit 0), data transfer complete bit
    uint32_t t0 = *DWT_CYCCNT;
    while(((*OTG_FS).IEP[0].DIEPINT & 0x00000001) == 0){
        // wait for 50ms
        if ((*DWT_CYCCNT - t0) > 8400000){
            // check if endpoint is enabled since after transfer core turns it off
            if ((*OTG_FS).IEP[0].DIEPCTL & 0x80000000){
                // turn of end point and answer stall
                (*OTG_FS).IEP[0].DIEPCTL |= 0x40200000;
                // wait for end point disable interrupt, cause the datasheet says so
                while (!((*OTG_FS).IEP[0].DIEPINT & 0x00000002)){pb0_on();}
                pb0_blinking();
                // clear interrupt by writing 1
                (*OTG_FS).IEP[0].DIEPINT = 0x00000002;
            } else (*OTG_FS).IEP[0].DIEPCTL |= 0x00200000;
            // flush fifo
            (*OTG_FS).GRSTCTL = 0x00000020;
            while ((*OTG_FS).GRSTCTL & 0x00000020){}
            return;
        }
    } 
    if(len>0){
        ep0_send(pointer,len);
    }
    (*OTG_FS).IEP[0].DIEPINT = 0x00000001;   // clear XFRC (write 1 to clear)
    return;
}
// to check how much of the dac table has arrived so we can end the loop
volatile int table_bytes_recieved=0;
//table_halfwords_recieved
volatile int n=0;
static void data_out_handler(uint32_t epnum, uint32_t bcnt){
    int lenght = (int)bcnt;
    if(epnum == 0){
        //ep0 is not a data end point just in case we pop it
        for (int i = 0; i < (int)bcnt; i += 4) (void)*OTG_FS_FIFO;
         return;
        } 
    if(epnum== 1){
        if(table_update==1){
            int i=bcnt;
            while(i>0){
                uint32_t temp = (*OTG_FS_FIFO1);
                if(n<4096){
                    dac2_table[n++] = temp;
                    dac2_table[n++] = temp>>16;
                    table_bytes_recieved += 4; 
                }
                i -= 4;
            }
            if(table_bytes_recieved>=8192){
                table_bytes_recieved=0;
                n=0;
                table_update =0;
                is_table_updated=1;
            }
        }
        else{
        int i=0;
        while(lenght>0){
            cmd_buf[i] = (*OTG_FS_FIFO1);
            ++i;
            lenght -= 4;
        }
        // 0x53430001 set in byte 0 is the password to use command
        if(bcnt == 64 && cmd_buf[0] == 0x53430001) read_command=1 ;
    }
    }
    else return;  
}
// // GINTSTS IEPINT (bit 18)
//   └─ DAINT bit 1 (EP1 IN)
//        └─ DIEPINT1 bit 7 (TXFE)

void usb_data_transfer(uint32_t *pointer, uint32_t lenght){
    if (lenght == 0){ ep1_in_arm(1, 0); return; }
    ge1ml = lenght;
    ge1mp = pointer;
    // maximum packate caunt is 1023
    while(ge1ml>0){
        ge1aml = min(ge1ml,65472);
        ep1_in_arm(min((ge1ml+63)/64,1023),ge1aml);
        // transfer complete interr lets not enable it only poll
        //(*OTG_FS).DIEPMSK |= 0x00000001;  
        // clear XFRC before using it
        (*OTG_FS).IEP[1].DIEPINT = 0x00000001;
        // this only controls TXFE
        (*OTG_FS).DIEPEMPMSK |= 0x00000002;
        // enable bit 18 In end point enable to use TXFE
        (*OTG_FS).GINTMSK |= 0x00040000; 
        uint32_t t0 = *DWT_CYCCNT;
        while (!((*OTG_FS).IEP[1].DIEPINT & 0x00000001)){   // XFRC = bit 0
            if ((*DWT_CYCCNT - t0) > 168000000){            // 1 s per arm: laptop stopped reading
                ep1_abort();
                return;
            }
        }
        (*OTG_FS).GINTMSK &= 0xFFFBFFFF;
        (*OTG_FS).DIEPEMPMSK &= 0xFFFFFFFD;
    }
    // ep1_in_arm(1,0);
}
static void ep1_send(void){
    // DTXFSTS1 bit 0:15 show how much data is in there, keep writing until we have space >16words
    while (ge1aml > 0 && ((*OTG_FS).IEP[1].DTXFSTS & 0x0000FFFF) >= 16){
        int i= 64; if(ge1ml<64) i = ge1ml;
        ge1ml -= i;
        ge1aml -=i;
        while (i>0)
        {
            uint32_t word = *ge1mp; ge1mp++;
            (*OTG_FS_FIFO1) = word;
            i -=4;
        } 
    }
}
static void ep1_in_arm(uint32_t packets, uint32_t bytes){
    (*OTG_FS).IEP[1].DIEPTSIZ = ((packets & 0x3FF) << 19)    // PKTCNT, bits 28:19, max 1023
                              |  (bytes & 0x7FFFF);          // XFRSIZ, bits 18:0
    (*OTG_FS).IEP[1].DIEPCTL |= 0x84000000;                  // EPENA (31) + CNAK (26)
}

// sequence according to the datasheet
static void ep1_abort(void){
    // turn off IN end point interr
    (*OTG_FS).GINTMSK    &= 0xFFFBFFFF;
    // turn off TXFE
    (*OTG_FS).DIEPEMPMSK &= 0xFFFFFFFD;
    ge1ml = 0; ge1aml = 0;
    // if end point is enabled
    if ((*OTG_FS).IEP[1].DIEPCTL & 0x80000000){
        // nak
        (*OTG_FS).IEP[1].DIEPCTL |= 0x08000000;
        // bit 6 of DIEPINT checks if NAK is set
        while (!((*OTG_FS).IEP[1].DIEPINT & 0x00000040)){ pb0_on();}
        pb0_blinking();
        // disable endpoint
        (*OTG_FS).IEP[1].DIEPCTL |= 0x48000000;
        while (!((*OTG_FS).IEP[1].DIEPINT & 0x00000002)){pb0_on();}
        pb0_blinking();
        // clear end point disable interr
        (*OTG_FS).IEP[1].DIEPINT = 0x00000002;
    }
    // write to flush
    (*OTG_FS).GRSTCTL = 0x00000060;
    while ((*OTG_FS).GRSTCTL & 0x00000020){pb0_on();}
    pb0_blinking();
}
// usb setup packet: 8 bytes, little endian (low byte first)
// byte 0    bmRequestType  bit 7    direction (0 host->dev, 1 dev->host)
//                          bit 6:5  type (0 standard, 1 class, 2 vendor)
//                          bit 4:0  recipient (0 device, 1 interface, 2 endpoint)
// byte 1    bRequest       which request
// byte 3:2  wValue         parameter, depends on request
// byte 5:4  wIndex         parameter, often interface/endpoint number
// byte 7:6  wLength        max bytes in data stage (0 = no data stage)
//
// standard requests:
// 6 GET_DESCRIPTOR     wValue hi = descriptor type, lo = index
// 5 SET_ADDRESS        wValue = new address
// 9 SET_CONFIGURATION  wValue = config to activate (1)
//
// stages: setup (8 bytes, always DATA0)
//      -> data (direction from bit 7, skipped if wLength = 0)
//      -> status (opposite direction, zero length packet)
// sequence is enumeration then communication then data
// TX FIFO write the bytes you want to send; RX FIFO 	read what arrived