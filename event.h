
#ifndef EVENT_H
#define	EVENT_H

#include "mcu-cpu.h"

// errorAxis only means anything when errorCode is set
// and means nothing if error not axis-specific
// 1 bit
extern char mcu_state;
extern char errorAxis;
extern Error errorCode;
extern uint32_t spiWord;
extern StatusRecU statusRec;

#define STATUS_REC_IDLE  -2
#define STATUS_REC_START -1
int8_t  statusRecOutIdx;

void initEvent();
void setState(char setState);
void handleError(char axis, Error code);
void eventLoop();  // called once from main.c and never returns

#endif	/* EVENT_H */

