
#ifndef EVENT_H
#define	EVENT_H

#include "mcu-cpu.h"

extern char mcu_state;
extern StatusRecU statusRec;

// errorAxis only means anything when errorCode is set
// and means nothing if error not axis-specific
extern char errorAxis;  // 1 bit
extern Error errorCode; // 5 bits

#define STATUS_REC_IDLE  -2
#define STATUS_REC_START -1
int8_t  statusRecOutIdx;

void initEvent();
void setState(char newState);
void handleError(char axis, Error code);
void eventLoop();  // called once from main.c and never returns

#endif	/* EVENT_H */

