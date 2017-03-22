
#include <xc.h>
#include "timer.h"
#include "main.h"
#include "pins.h"

// these are loaded into CCP compare regs in interrupt
volatile time_ut timeX;
volatile time_ut timeY;

void initTimer() {
  // timer 1 -- used only by CCPs below
  T1CLKbits.CS   = 1;      // Fosc/4, 8 MHz before prescale
  T1CONbits.CKPS = 3;      // 11 = 1:8 prescale value, 1 MHz
  T1GCONbits.GE  = 0;      // always on, no gate
  TMR1ON = 1;              // start timer
  
  // CCP 1 -- controls X step interrupt from counter compare 1
  STEP_X_LAT       = 1;  // high on idle
  CCP1IE           = 0;
  CCP1CONbits.MODE = 0x8;  // set output high on compare (output not used)
  CCP1IE           = 0;
  CCP1CONbits.EN   = 1;
#ifdef XY
  // CCP 2 -- controls Y step interrupt from counter compare 2
  STEP_Y_LAT       = 1;  // high on idle
  CCP2IE           = 0;
  CCP2CONbits.MODE = 0x8;  // set output high on compare (output not used)
#endif
 
  CCP1IE           = 0;
  CCP2CONbits.EN   = 1;
}

void stopTimerX() {
  CCP1IE   = 0;
}
#ifdef XY
void stopTimerY() {
  CCP2IE   = 0;
}
#endif

void resetTimers() {
  TMR1H  = 0;
  TMR1L  = 0;
  CCP1IE = 0;
  // timeBytes are used as running 16-bit timer positions desired
  timeX.timeBytes[1] = 0;
  timeX.timeBytes[0] = 0;
#ifdef XY
  CCP2IE = 0;
  timeY.timeBytes[1] = 0;
  timeY.timeBytes[0] = 0;
#endif
}

void setNextTimeX(shortTime_t delta, bool_t startPulse) {
  CCP1IE = 0;
  if(startPulse) STEP_X_LAT = 0;
  timeX.timeShort += delta;
  CCPR1H = timeX.timeBytes[1];
  CCPR1L = timeX.timeBytes[0];
  CCP1IF = 0;
  CCP1IE = 1;
}
#ifdef XY
void setNextTimeY(shortTime_t delta, bool_t startPulse) {
  CCP2IE = 0;
  if(startPulse) STEP_Y_LAT = 0;
  timeY.timeShort += delta;
  CCPR2H = timeY.timeBytes[1];
  CCPR2L = timeY.timeBytes[0];
  CCP2IF = 0;
  CCP2IE = 1;
}
#endif

