#include <msp430.h>
#include "command_process.h"

volatile State state = HANDSHAKE;
volatile Mode mode = TEMPERATURE;

static void Handshake(void)//收到握手指令
{
    if (state == HANDSHAKE) state = IDLE;
}

static void Start(void)//收到开始指令
{
    if (state == IDLE) state = WORKING;
}

static void Stop(void)//收到停止指令
{
    if (state == WORKING) state = IDLE;
}

void CommandAnalyze(uint8_t command)//分析指令调用对应函数
{
    switch (command) {
    case 0xF0://握手
        Handshake();
        break;
    case 0xFF://启动
        Start();
        break;
    case 0x00://停止
        Stop();
        break;
    case 0xAA://温度模式
        if (state == IDLE) mode = TEMPERATURE;
        break;
    case 0xBB://波形模式
        if (state == IDLE) mode = WAVE;
        break;
    case 0xCC://触发模式
        if (state == IDLE) mode = TRIGGER;
        break;
    }
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void UartRxIsr(void)
{
    if (IFG2 & UCA0RXIFG) {//收到指令时
        CommandAnalyze(UCA0RXBUF);
        __bic_SR_register_on_exit(LPM0_bits);//关闭低功耗
    }
}
