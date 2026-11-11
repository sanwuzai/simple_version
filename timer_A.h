#ifndef SIMPLE_TIMER_A_H
#define SIMPLE_TIMER_A_H

#include "command_process.h"

void TimerInit(void);
/* Wait n ticks, or return false when the expected state ends. */
bool Delay(uint16_t n, State expected);

#endif
