
#include <xc.h>
#include "timer.h"
#include "main.h"
#include "pins-b.h"

// these are loaded into CCP compare regs in interrupt
time_ut timeX;
time_ut timeY;

bool_t firstPulseX = TRUE;
 
void initTimer() {
  // timer 1 -- used only by CCPs below
  T1CLKbits.CS   = 1;      // Fosc/4, 8 MHz before prescale
  T1CONbits.CKPS = 3;      // 11 = 1:8 prescale value, 1 MHz
  T1GCONbits.GE  = 0;      // always on, no gate
  TMR1ON = 1;              // start timer
  
  // CCP 1 -- controls X step interrupt from counter compare 1
  CCP1_LAT  = 1;  // high on idle
  CCP1_TRIS = 0;          
  CCP1IE    = 0;
  CCP1CONbits.MODE = 0x8;  // set output high on compare (output not used)
  CCP1CONbits.EN = 1;
  
  // CCP 2 -- controls Y step interrupt from counter compare 2
  CCP2_LAT  = 1;  // high on idle
  CCP2_TRIS = 0;           
  CCP2IE    = 0;
  CCP2CONbits.MODE = 0x8;  // set output high on compare (output not used)
  CCP2CONbits.EN = 1;
}

void stopTimerX() {
  CCP1IE   = 0;
  CCP1_LAT = 1;
}
void stopTimerY() {
  CCP2IE   = 0;
  CCP2_LAT = 1;
}

void resetTimers() {
  stopTimerX();
  stopTimerY();
  TMR1H = 0;
  TMR1L = 0;
  timeX.timeBytes[1] = 0;
  timeX.timeBytes[0] = 0;
  timeY.timeBytes[1] = 0;
  timeY.timeBytes[0] = 0;
}

void setNextTimeX(shortTime_t delta, bool_t startPulse) {
  LATC6 = 0;
  CCP1IE = 0;
  if(startPulse) CCP1_LAT = 0;
  timeX.timeShort += delta;
  CCPR1H = timeX.timeBytes[1];
  CCPR1L = timeX.timeBytes[0];
  CCP1IF = 0;
  CCP1IE = 1;
  LATC6 = 1;
}

void setNextTimeY(shortTime_t delta, bool_t startPulse) {
  CCP2IE = 0;
  if(startPulse) CCP2_LAT = 0;
  timeY.timeShort += delta;
  CCPR2H = timeY.timeBytes[1];
  CCPR2L = timeY.timeBytes[0];
  CCP2IF = 0;
  CCP2IE = 1;
}
