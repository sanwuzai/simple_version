#include "Working.h"
#include "Sampler.h"
#include "timer_A.h"
#include <stdbool.h>

#define TEMP_TICKS 100
#define HISTORY_POINTS 64u
#define AVCC_MV 3300UL
#define TRIGGER_LIMIT (200UL * HISTORY_POINTS * 1023UL / AVCC_MV)

static uint16_t history[HISTORY_POINTS];

static void Temp(void)
{
    Delay(TEMP_TICKS);
    if (state != WORKING) return;
    Sample();
    if (state != WORKING) return;
    Pack();
    send();
}

static bool VoltageSample(void)
{
    Delay(1);
    if (state != WORKING) return false;
    Sample();
    return state == WORKING;
}

static void Wave(void)
{
    uint8_t i;

    for (i = 0; i < WAVE_POINTS; ++i) {
        if (!VoltageSample()) return;
        Pack();
    }
    send();
}

static void Trigger(void)
{
    uint8_t i, oldest = 0;
    uint16_t sum = 0;                /* 64 * 1023 fits in uint16_t. */

    for (i = 0; i < HISTORY_POINTS; ++i) {
        if (!VoltageSample()) return;
        history[i] = adcBUF;
        sum += history[i];
    }
    for (;;) {
        uint16_t scaled = adcBUF * HISTORY_POINTS;
        uint16_t delta = scaled > sum ? scaled - sum : sum - scaled;

        if (delta > TRIGGER_LIMIT) break;
        if (!VoltageSample()) return;
        sum -= history[oldest];
        history[oldest] = adcBUF;
        sum += history[oldest];
        oldest = (oldest + 1u) % HISTORY_POINTS;
    }
    /* Pack the history in order, with the trigger at sample 64. */
    for (i = 0; i < HISTORY_POINTS; ++i) {
        if (state != WORKING) return;
        adcBUF = history[(oldest + i) % HISTORY_POINTS];
        Pack();
    }
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
    SamplerInit();
    switch (mode) {
    case TEMPERATURE: Temp(); break;
    case WAVE: Wave(); break;
    case TRIGGER: Trigger(); break;
    }
}
