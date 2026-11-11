#include <msp430.h>
#include "Sampler.h"

static volatile uint16_t adcBUF;
static volatile bool adc_ready;
static const uint8_t *volatile tx_next;
static volatile uint16_t tx_remaining;

void SamplerInit(Mode selected)
{
    ADC10CTL0 &= ~ENC;
    ADC10DTC1 = 0;
    ADC10CTL0 = ADC10SHT_3 | ADC10ON;
    if (selected == TEMPERATURE) {
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

bool Sample(uint16_t *raw)
{
    bool active;

    __disable_interrupt();
    if (StateActive(WORKING)) {
        adc_ready = false;
        ADC10CTL0 |= ADC10IE | ENC | ADC10SC;
        while (!adc_ready) {
            __bis_SR_register(LPM0_bits | GIE);
            __disable_interrupt();
        }
        *raw = adcBUF;
    }
    active = StateActive(WORKING);
    __enable_interrupt();
    return active;
}

void PackHeader(uint8_t *frame, Mode selected, uint8_t count)
{
    frame[0] = 0xA5;
    frame[1] = 0x5A;
    frame[2] = selected == TEMPERATURE ? 0x00 : 0xFF;
    frame[3] = count;
}

void Pack(uint8_t *frame, uint8_t index, uint16_t raw)
{
    uint16_t offset = FRAME_SIZE(index);

    frame[offset] = (uint8_t)(raw >> 8);
    frame[offset + 1u] = (uint8_t)raw;
}

uint16_t ReadPacked(const uint8_t *frame, uint8_t index)
{
    uint16_t offset = FRAME_SIZE(index);

    return ((uint16_t)frame[offset] << 8) | frame[offset + 1u];
}

bool Send(const uint8_t *frame, uint16_t size, State expected)
{
    bool active;

    __disable_interrupt();
    while (tx_remaining != 0u && StateActive(expected)) {
        __bis_SR_register(LPM0_bits | GIE);
        __disable_interrupt();
    }
    active = StateActive(expected);
    if (active) {
        tx_next = frame;
        tx_remaining = size;
        IE2 |= UCA0TXIE;
    }
    __enable_interrupt();
    return active;
}

void SendWait(void)
{
    __disable_interrupt();
    while (tx_remaining != 0u) {
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
    adc_ready = true;
    __bic_SR_register_on_exit(LPM0_bits);
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void UartTxIsr(void)
{
    if (IFG2 & UCA0TXIFG) {
        UCA0TXBUF = *tx_next++;
        if (--tx_remaining == 0u) {
            IE2 &= ~UCA0TXIE;
            __bic_SR_register_on_exit(LPM0_bits);
        }
    }
}
