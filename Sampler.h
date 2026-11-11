#ifndef SIMPLE_SAMPLER_H
#define SIMPLE_SAMPLER_H

#include "command_process.h"

#define WAVE_POINTS 8u
#define TRIGGER_POINTS 128u

extern volatile uint16_t adcBUF;
extern volatile uint16_t PackBit;

void SamplerInit(void);
void Sample(void);
void Pack(void);
void send(void);

#endif
