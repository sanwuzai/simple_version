#include <msp430.h>
#include "bsp_init.h"
#include "timer_A.h"

void BspInit(void)
{
    WDTCTL = WDTPW | WDTHOLD;
    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    BCSCTL2 = 0;                     /* MCLK = SMCLK = 1 MHz */

    P1SEL |= BIT1 | BIT2;             /* P1.1 RXD, P1.2 TXD */
    P1SEL2 |= BIT1 | BIT2;
    UCA0CTL1 = UCSWRST | UCSSEL_2;
    UCA0CTL0 = 0;                    /* 8N1 */
    UCA0BR0 = 104u;
    UCA0BR1 = 0;
    UCA0MCTL = UCBRS0;               /* 9600 baud */
    UCA0CTL1 &= ~UCSWRST;
    IE2 = (IE2 & ~UCA0TXIE) | UCA0RXIE;

    TA0CCR0 = 4999u;
    TA0CCTL0 = 0;
    TA0CTL = TASSEL_2 | MC_1 | TACLR;  /* 5000 SMCLK cycles per tick */

    __enable_interrupt();
}
