
#ifndef TIMER_H
#define	TIMER_H

// bytes are set between timer interrupts for quick use in int
extern char ccpXLowByte;
extern char ccpXHighByte;
extern char ccpYLowByte;
extern char ccpYHighByte;

void initTimer();
void startTimer();
void stopTimer();

#endif	/* TIMER_H */

