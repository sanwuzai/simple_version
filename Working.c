#include "Working.h"
#include "Sampler.h"
#include "timer_A.h"

#define TEMP_TICKS 100u
#define WAVE_POINTS 8u
#define HISTORY_POINTS 64u
#define TRIGGER_POINTS (2u * HISTORY_POINTS)
#define AVCC_MV 3300UL
#define TRIGGER_LIMIT (200UL * HISTORY_POINTS * 1023UL / AVCC_MV)

static uint8_t frame[FRAME_SIZE(TRIGGER_POINTS)];
/* One wave frame goes out in ~21 ms; the next takes 40 ms to fill. */
static uint8_t wave_frame[FRAME_SIZE(WAVE_POINTS)];

static void Temp(void)
{
    uint16_t raw;

    PackHeader(frame, TEMPERATURE, 1u);
    while (Delay(TEMP_TICKS, WORKING) && Sample(&raw)) {
        Pack(frame, 0u, raw);
        if (!Send(frame, FRAME_SIZE(1u), WORKING)) return;
    }
}

static bool VoltageSample(uint16_t *raw)
{
    return Delay(1u, WORKING) && Sample(raw);
}

static void Wave(void)
{
    uint8_t *filling = frame;
    uint8_t i;
    uint16_t raw;

    for (;;) {
        PackHeader(filling, WAVE, WAVE_POINTS);
        for (i = 0; i < WAVE_POINTS; ++i) {
            if (!VoltageSample(&raw)) return;
            Pack(filling, i, raw);
        }
        if (!Send(filling, FRAME_SIZE(WAVE_POINTS), WORKING)) return;
        filling = filling == frame ? wave_frame : frame;
    }
}

static bool Triggered(uint16_t raw, uint16_t sum)
{
    uint16_t scaled = raw * HISTORY_POINTS;
    uint16_t delta = scaled > sum ? scaled - sum : sum - scaled;

    /* Compare with the mean including the latest point, without rounding it. */
    return delta > TRIGGER_LIMIT;
}

static void ReverseHistory(uint8_t first, uint8_t end)
{
    while (first < end) {
        uint16_t raw = ReadPacked(frame, first);

        --end;
        Pack(frame, first, ReadPacked(frame, end));
        Pack(frame, end, raw);
        ++first;
    }
}

static void Trigger(void)
{
    uint8_t i, oldest = 0;
    uint16_t raw, sum = 0;            /* 64 * 1023 fits in uint16_t. */

    PackHeader(frame, TRIGGER, TRIGGER_POINTS);
    for (i = 0; i < HISTORY_POINTS; ++i) {
        if (!VoltageSample(&raw)) return;
        Pack(frame, i, raw);
        sum += raw;
    }
    while (!Triggered(raw, sum)) {
        if (!VoltageSample(&raw)) return;
        sum -= ReadPacked(frame, oldest);
        sum += raw;
        Pack(frame, oldest, raw);
        oldest = (oldest + 1u) % HISTORY_POINTS;
    }
    for (i = HISTORY_POINTS; i < TRIGGER_POINTS; ++i) {
        if (!VoltageSample(&raw)) return;
        Pack(frame, i, raw);
    }
    /* Rotate just the history so the trigger point is at index 63. */
    if (oldest != 0u) {
        ReverseHistory(0u, oldest);
        ReverseHistory(oldest, HISTORY_POINTS);
        ReverseHistory(0u, HISTORY_POINTS);
    }
    Send(frame, FRAME_SIZE(TRIGGER_POINTS), WORKING);
}

void Working(void)
{
    Mode selected;

    if (!WorkBegin(&selected)) return;
    SamplerInit(selected);
    TimerInit();
    switch (selected) {
    case TEMPERATURE: Temp(); break;
    case WAVE: Wave(); break;
    case TRIGGER: Trigger(); break;
    }
    SendWait();
    WorkEnd();
}
