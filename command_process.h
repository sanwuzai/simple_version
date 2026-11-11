#ifndef SIMPLE_COMMAND_PROCESS_H
#define SIMPLE_COMMAND_PROCESS_H

#include <stdint.h>

typedef enum { HANDSHAKE, IDLE, WORKING } State;
typedef enum { TEMPERATURE, WAVE, TRIGGER } Mode;

extern volatile State state;
extern volatile Mode mode;

void CommandAnalyze(uint8_t command);

#endif
