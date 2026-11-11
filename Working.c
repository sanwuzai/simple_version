#include "Working.h"
#include "Sampler.h"
#include "timer_A.h"
#include <stdbool.h>

#define TEMP_TICKS 100
#define HISTORY_POINTS 64u
#define AVCC_MV 3300UL
#define TRIGGER_LIMIT (200UL * HISTORY_POINTS * 1023UL / AVCC_MV)

static uint16_t history[HISTORY_POINTS];

static void Temp(void)//温度模式
{
    Delay(TEMP_TICKS);
    if (state != WORKING) return;
    Sample();
    if (state != WORKING) return;
    Pack();
    send();
}

static bool VoltageSample(void)//采集一次电压
{
    Delay(1);
    if (state != WORKING) return false;
    Sample();
    return state == WORKING;
}

static void Wave(void)//波形模式每5ms采集一次电压，采集八次后发送
{
    uint8_t i;

    for (i = 0; i < WAVE_POINTS; ++i) {
        if (!VoltageSample()) return;
        Pack();
    }
    send();
}

static void Trigger(void)   //触发模式，详细原理见设计文档
{
    uint8_t i, oldest = 0;
    uint16_t sum = 0;   // 64 * 1023 fits in uint16_t.

    for (i = 0; i < HISTORY_POINTS; ++i) {//采集63个点并求和
        if (!VoltageSample()) return;
        history[i] = adcBUF;
        sum += history[i];
    }
    for (;;) {
        uint16_t scaled = adcBUF * HISTORY_POINTS;
        uint16_t delta = scaled > sum ? scaled - sum : sum - scaled;

        if (delta > TRIGGER_LIMIT) break;//差值过大就触发
        if (!VoltageSample()) return;//未触发继续采样新的点
        sum -= history[oldest];
        history[oldest] = adcBUF;
        sum += history[oldest];//更新sum
        oldest = (oldest + 1u) % HISTORY_POINTS;//更新oldest
    }
    //将历史数据按顺序打包
    for (i = 0; i < HISTORY_POINTS; ++i) {
        if (state != WORKING) return;
        adcBUF = history[(oldest + i) % HISTORY_POINTS];
        Pack();
    }
    //继续采集后64个点并打包
    for (i = HISTORY_POINTS; i < TRIGGER_POINTS; ++i) {
        if (!VoltageSample()) return;
        Pack();
    }
    send();
    state = IDLE;
}

void working(void)
{
    PackBit = 0;
    SamplerInit();//根据当前模式进行adc的配置
    switch (mode) {//根据模式调用不同的工作函数
    case TEMPERATURE: Temp(); break;
    case WAVE: Wave(); break;
    case TRIGGER: Trigger(); break;
    }
}
