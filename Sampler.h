#ifndef SIMPLE_SAMPLER_H
#define SIMPLE_SAMPLER_H

#include "command_process.h"

#define FRAME_SIZE(count) (4u + 2u * (count))

void SamplerInit(Mode selected);
bool Sample(uint16_t *raw);
void PackHeader(uint8_t *frame, Mode selected, uint8_t count);
void Pack(uint8_t *frame, uint8_t index, uint16_t raw);
uint16_t ReadPacked(const uint8_t *frame, uint8_t index);
bool Send(const uint8_t *frame, uint16_t size, State expected);
void SendWait(void);

#endif
