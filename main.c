#include <msp430.h>
#include "bsp_init.h"
#include "command_process.h"
#include "timer_A.h"
#include "Working.h"

int main(void)
{
    BspInit();
    for (;;) {
        switch (state) {
        case HANDSHAKE:
            Delay(100);
            if (state == HANDSHAKE) UCA0TXBUF = '=';
            break;
        case IDLE:
            __disable_interrupt();
            if (state == IDLE) {
                __bis_SR_register(LPM0_bits | GIE);
            }
            __enable_interrupt();
            break;
        case WORKING:
            working();
            break;
        }
    }
}
