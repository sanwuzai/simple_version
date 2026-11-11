#include <msp430.h>
#include "Sampler.h"

volatile uint16_t adcBUF;
volatile uint16_t PackBit;
static volatile uint8_t txBUF[4u + 2u * TRIGGER_POINTS];
static volatile uint16_t tx_pos;

void SamplerInit(void)
{
    ADC10CTL0 &= ~ENC;
    ADC10DTC1 = 0;
    ADC10CTL0 = ADC10SHT_3 | ADC10ON;
    if (mode == TEMPERATURE) {
        ADC10AE0 &= ~BIT7;
        ADC10CTL1 = INCH_10 | ADC10SSEL_3;
        ADC10CTL0 |= SREF_1 | REFON;  /* Internal 1.5 V reference */
        __delay_cycles(100);
    } else {
        P1DIR &= ~BIT7;
        P1REN &= ~BIT7;
        ADC10AE0 |= BIT7;
        ADC10CTL1 = INCH_7 | ADC10SSEL_3;  /* A7, AVCC reference */
    }
}

void Sample(void)
{
    __disable_interrupt();
    if (state == WORKING) {
        ADC10CTL0 |= ADC10IE | ENC | ADC10SC;
        while (ADC10CTL0 & ADC10IE) {
            __bis_SR_register(LPM0_bits | GIE);
            __disable_interrupt();
        }
    }
    __enable_interrupt();
}

void Pack(void)
{
    switch (PackBit) {
    case 0:
        txBUF[PackBit++] = 0xA5;
        break;
    case 1:
        txBUF[PackBit++] = 0x5A;
        break;
    case 2:
        txBUF[PackBit++] = mode == TEMPERATURE ? 0x00 : 0xFF;
        break;
    case 3:
        switch (mode) {
        case TEMPERATURE: txBUF[PackBit++] = 1u; break;
        case WAVE: txBUF[PackBit++] = WAVE_POINTS; break;
        case TRIGGER: txBUF[PackBit++] = TRIGGER_POINTS; break;
        }
        break;
    default:
        txBUF[PackBit++] = (uint8_t)(adcBUF >> 8);
        txBUF[PackBit++] = (uint8_t)adcBUF;
        return;
    }
    Pack();
}

void send(void)
{
    __disable_interrupt();
    if (state != WORKING) {
        __enable_interrupt();
        return;
    }
    tx_pos = 0;
    IE2 |= UCA0TXIE;
    while (IE2 & UCA0TXIE) {
        __bis_SR_register(LPM0_bits | GIE);
        __disable_interrupt();
    }
    __enable_interrupt();
    /* TXBUF empty does not mean the last stop bit is on the wire yet. */
    while (UCA0STAT & UCBUSY) {
    }
}

#pragma vector=ADC10_VECTOR
__interrupt void AdcIsr(void)
{
    adcBUF = ADC10MEM;
    ADC10CTL0 &= ~ADC10IE;
    __bic_SR_register_on_exit(LPM0_bits);
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void UartTxIsr(void)
{
    if (IFG2 & UCA0TXIFG) {
        UCA0TXBUF = txBUF[tx_pos++];
        if (tx_pos == PackBit) {
            IE2 &= ~UCA0TXIE;
            __bic_SR_register_on_exit(LPM0_bits);
        }
    }
}
