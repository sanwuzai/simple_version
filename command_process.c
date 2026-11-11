#include <msp430.h>
#include "command_process.h"

volatile State state = HANDSHAKE;
volatile Mode mode = TEMPERATURE;

static void Handshake(void)
{
    if (state == HANDSHAKE) state = IDLE;
}

static void Start(void)
{
    if (state == IDLE) state = WORKING;
}

static void Stop(void)
{
    if (state == WORKING) state = IDLE;
}

void CommandAnalyze(uint8_t command)
{
    switch (command) {
    case 0xF0:
        Handshake();
        break;
    case 0xFF:
        Start();
        break;
    case 0x00:
        Stop();
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
        CommandAnalyze(UCA0RXBUF);
        __bic_SR_register_on_exit(LPM0_bits);
    }
}
