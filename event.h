
#ifndef EVENT_H
#define	EVENT_H

#include "mcu-cpu.h"

extern Status mcu_status;
// errorAxis only means anything when errorCode is set
// and means nothing if error not axis-specific
// 1 bit
extern char errorAxis;
extern Error errorCode;

void initEvent();
void newStatus(char newStatus);
void handleError(char axis, Error code);
void eventLoop();  // called once from main.c and never returns

#endif	/* EVENT_H */

