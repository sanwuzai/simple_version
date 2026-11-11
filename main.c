#include <msp430.h>
#include "bsp_init.h"
#include "command_process.h"
#include "timer_A.h"
#include "Working.h"

int main(void)
{
    BspInit();
    for (;;) {
        switch (state) {
        case HANDSHAKE://握手状态不断发送"="
            Delay(100);
            if (state == HANDSHAKE) UCA0TXBUF = '=';
            break;
        case IDLE://空闲状态进入低功耗等待指令唤醒
            __disable_interrupt();
            if (state == IDLE) {
                __bis_SR_register(LPM0_bits | GIE);
            }
            __enable_interrupt();
            break;
        case WORKING://工作状态开始工作
            working();
            break;
        }
    }
}
