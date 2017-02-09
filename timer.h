
#ifndef TIMER_H
#define	TIMER_H

#include "main.h"

typedef struct time_ut {
  shortTime_t timeShort;
  char        timeBytes[2]; // remember, little-endian
} time_ut;

// CCP times are set between timer interrupts for quick use in int
extern time_ut timeX;
extern time_ut timeY;
extern bool_t ccp1Matched;
extern bool_t ccp2Matched;

void initTimer();
void startTimer();
void stopTimerX();
void stopTimerY();
void stopTimer();
void setNextTimeX(shortTime_t delta);
void setNextTimeY(shortTime_t delta);

#endif	/* TIMER_H */

