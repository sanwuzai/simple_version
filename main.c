#include <msp430.h>
#include "bsp_init.h"
#include "command_process.h"
#include "timer_A.h"
#include "Sampler.h"
#include "Working.h"

int main(void)
{
    static const uint8_t hello = '=';

    BspInit();
    for (;;) {
        switch (state) {
        case HANDSHAKE:
            if (Delay(100u, HANDSHAKE)) {
                Send(&hello, 1u, HANDSHAKE);
            }
            break;
        case IDLE:
            __disable_interrupt();
            if (state == IDLE) {
                __bis_SR_register(LPM0_bits | GIE);
            }
            __enable_interrupt();
            break;
        case WORKING:
            Working();
            break;
        }
    }
}
