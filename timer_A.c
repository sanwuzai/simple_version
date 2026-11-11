#include <msp430.h>
#include "command_process.h"
#include "timer_A.h"

static volatile int ticks_left;

void TimerInit(void)
{
    TA0CCR0 = 4999u;
    TA0CCTL0 = 0;
    TA0CTL = TASSEL_2 | MC_1 | TACLR;  /* 5000 SMCLK cycles per tick */
}

void Delay(int n)
{
    State previous;

    __disable_interrupt();
    previous = state;
    ticks_left = n;
    TA0CCTL0 = CCIE;                  /* Clear a flag from outside Delay. */
    while (ticks_left > 0 && state == previous) {
        __bis_SR_register(LPM0_bits | GIE);
        __disable_interrupt();
    }
    TA0CCTL0 = 0;
    __enable_interrupt();
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TimerAIsr(void)
{
    --ticks_left;
    __bic_SR_register_on_exit(LPM0_bits);
}
