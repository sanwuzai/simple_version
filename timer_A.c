#include <msp430.h>
#include "command_process.h"
#include "timer_A.h"

static volatile int ticks_left;

void Delay(int n)//延时n个5ms
{
    State previous;

    __disable_interrupt();
    previous = state;//保存先前状态
    ticks_left = n;
    TA0CCTL0 = CCIE;                  
    while (ticks_left > 0/*等待足够拍数后退出*/ && state == previous/*状态不一样也会退出*/) {
        __bis_SR_register(LPM0_bits | GIE);//休眠等待Timer中断
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
