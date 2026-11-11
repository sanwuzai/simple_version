#include <msp430.h>
#include "timer_A.h"

static volatile uint16_t ticks_left;

void TimerInit(void)
{
    TA0CCR0 = 4999u;
    TA0CCTL0 = 0;
    TA0CTL = TASSEL_2 | MC_1 | TACLR;  /* 5000 SMCLK cycles per tick */
}

bool Delay(uint16_t n, State expected)
{
    bool active;

    __disable_interrupt();
    ticks_left = n;
    TA0CCTL0 = CCIE;                  /* Clear a flag from outside Delay. */
    while (ticks_left != 0u && StateActive(expected)) {
        __bis_SR_register(LPM0_bits | GIE);
        __disable_interrupt();
    }
    TA0CCTL0 = 0;
    active = StateActive(expected);
    __enable_interrupt();
    return active;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TimerAIsr(void)
{
    if (ticks_left != 0u && --ticks_left == 0u) {
        __bic_SR_register_on_exit(LPM0_bits);
    }
}
