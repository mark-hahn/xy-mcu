
#ifndef TIMER_H
#define	TIMER_H

#include "main.h"

#define START_PULSE 1
#define NO_PULSE 0

typedef union time_ut {
  shortTime_t timeShort;
  char        timeBytes[2]; // remember, little-endian
} time_ut;

// CCP times are set between timer interrupts for quick use in int
extern volatile time_ut timeX;
extern volatile time_ut timeY;

void initTimer();
void resetTimers();
void stopTimerX();
void stopTimerY();
void setNextTimeX(shortTime_t delta, bool_t startPulse);
void setNextTimeY(shortTime_t delta, bool_t startPulse);

#endif	/* TIMER_H */

