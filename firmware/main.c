#include <stdint.h>
#include "regs.h"
#include "usb.h"
#include "functions.h"

extern volatile int read_command;

int main(){
    pb0_on();
    init();
    usb_enable();
    while (1) {
        if(read_command ==1){ 
            command_read();
        } 
    }
}

