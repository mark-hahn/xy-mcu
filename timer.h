
#ifndef TIMER_H
#define	TIMER_H

#include "main.h"

#define START_PULSE 1
#define NO_PULSE 0

typedef union time_ut {
  uint16_t time;
  char        timeBytes[2]; // remember, little-endian
} time_ut;

// CCP times are set between timer interrupts for quick use in int
void initTimer();
void resetTimers();

extern volatile time_ut timeX;
void stopTimerX();
void setNextTimeX(uint16_t delta, bool_t startPulse);
void setNextPpsX(uint16_t pps, bool_t startPulse);

#ifdef XY
extern volatile time_ut timeY;
void stopTimerY();
void setNextTimeY(uint16_t delta, bool_t startPulse);
void setNextPpsY(uint16_t pps, bool_t startPulse);
#endif

#endif	/* TIMER_H */

