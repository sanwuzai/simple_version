#include <msp430.h>
#include "command_process.h"

volatile State state = HANDSHAKE;
volatile Mode mode = TEMPERATURE;
/* Remember a stop even if another FF arrives while the old frame drains. */
static volatile bool stopped;

bool StateActive(State expected)
{
    return state == expected && (expected != WORKING || !stopped);
}

bool WorkBegin(Mode *selected)
{
    bool started;

    __disable_interrupt();
    started = state == WORKING;
    if (started) {
        stopped = false;
        *selected = mode;
    }
    __enable_interrupt();
    return started;
}

void WorkEnd(void)
{
    __disable_interrupt();
    if (StateActive(WORKING)) {
        state = IDLE;
    }
    __enable_interrupt();
}

void CommandAnalyze(uint8_t command)
{
    switch (command) {
    case 0xF0:
        if (state == HANDSHAKE) state = IDLE;
        break;
    case 0xFF:
        if (state == IDLE) state = WORKING;
        break;
    case 0x00:
        if (state == WORKING) {
            stopped = true;
            state = IDLE;
        }
        break;
    case 0xAA:
        if (state == IDLE) mode = TEMPERATURE;
        break;
    case 0xBB:
        if (state == IDLE) mode = WAVE;
        break;
    case 0xCC:
        if (state == IDLE) mode = TRIGGER;
        break;
    }
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void UartRxIsr(void)
{
    if (IFG2 & UCA0RXIFG) {
        uint8_t status = UCA0STAT;
        uint8_t command = UCA0RXBUF;
        State previous = state;

        if (!(status & (UCOE | UCFE | UCPE | UCBRK))) {
            CommandAnalyze(command);
        }
        if (state != previous) {
            __bic_SR_register_on_exit(LPM0_bits);
        }
    }
}
