#include <msp430.h>
#include "Sampler.h"

volatile uint16_t adcBUF;
volatile uint16_t PackBit;
static volatile uint8_t txBUF[4u + 2u * TRIGGER_POINTS];
static volatile uint16_t tx_pos;

void SamplerInit(void)//根据模式选择初始化adc
{
    ADC10CTL0 &= ~ENC;
    ADC10DTC1 = 0;
    ADC10CTL0 = ADC10SHT_3 | ADC10ON;
    if (mode == TEMPERATURE) {
        ADC10AE0 &= ~BIT7;
        ADC10CTL1 = INCH_10 | ADC10SSEL_3;
        ADC10CTL0 |= SREF_1 | REFON;  /* Internal 1.5 V reference */
        __delay_cycles(100);//等待参考稳定
    } else {
        P1DIR &= ~BIT7;
        P1REN &= ~BIT7;
        ADC10AE0 |= BIT7;
        ADC10CTL1 = INCH_7 | ADC10SSEL_3;  /* A7, AVCC reference */
    }
}

void Sample(void)//进行一次采样
{
    __disable_interrupt();
    if (state == WORKING) {
        ADC10CTL0 |= ADC10IE | ENC | ADC10SC;//开启一次采样
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
    case 0://第零位时填入帧头
        txBUF[PackBit++] = 0xA5;
        break;
    case 1://第一位时填入帧头
        txBUF[PackBit++] = 0x5A;
        break;
    case 2://第二位时填入数据类型
        txBUF[PackBit++] = mode == TEMPERATURE ? 0x00 : 0xFF;
        break;
    case 3://第三位时填入数量
        switch (mode) {
        case TEMPERATURE: txBUF[PackBit++] = 1u; break;
        case WAVE: txBUF[PackBit++] = WAVE_POINTS; break;
        case TRIGGER: txBUF[PackBit++] = TRIGGER_POINTS; break;
        }
        break;
    default://填入数据
        txBUF[PackBit++] = (uint8_t)(adcBUF >> 8);
        txBUF[PackBit++] = (uint8_t)adcBUF;
        return;
    }
    Pack();
}

void send(void)//发送直到发送队列空
{
    __disable_interrupt();
    if (state != WORKING) {
        __enable_interrupt();
        return;
    }
    tx_pos = 0;
    IE2 |= UCA0TXIE;//打开发送中断
    while (IE2 & UCA0TXIE) {
        __bis_SR_register(LPM0_bits | GIE);
        __disable_interrupt();
    }
    __enable_interrupt();
    /* TXBUF empty does not mean the last stop bit is on the wire yet. */
    while (UCA0STAT & UCBUSY) {}
}

#pragma vector=ADC10_VECTOR
__interrupt void AdcIsr(void)
{
    adcBUF = ADC10MEM;//取出采样数据
    ADC10CTL0 &= ~ADC10IE;//关闭采集
    __bic_SR_register_on_exit(LPM0_bits);
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void UartTxIsr(void)
{
    if (IFG2 & UCA0TXIFG) {//可以写入下一个字符时
        UCA0TXBUF = txBUF[tx_pos++];
        if (tx_pos == PackBit) {//如果发完了就停止发送
            IE2 &= ~UCA0TXIE;
            __bic_SR_register_on_exit(LPM0_bits);
        }
    }
}
