
#include <xc.h>
#include "timer.h"
#include "main.h"
#include "pins-b.h"

// CCP times are set between timer interrupts for quick use in int
time_ut timeX;
time_ut timeY;

// this starts rising step edges and interrupts which never end
void initTimer() {
  T1CLKbits.CS   = 1;      // Fosc/4, 8 MHz before prescale
  T1CONbits.CKPS = 3;      // 11 = 1:8 prescale value, 1 MHz
  T1GCONbits.GE = 0;       // always on, no gate
  // timer not enabled until startTimer()
   
  // X step pin from counter compare 1
  CCP1_LAT = 1;            // start step pin high
  CCP1_TRIS = 0;           // step pin output
  CCP1_PPS = 0x09;         // CCP1 => B4
  CCP1CONbits.MODE = 0x8;  // Compare mode, set step high
//  CCPTMRS0.C1TSEL = 0;   // CCP1 matches timer 1  ???
  // CCP1 not enabled until startTimer()
  
  // Y step pin from counter compare 2
  CCP2_LAT = 1;            // see comments above
  CCP2_TRIS = 0;           
  CCP2_PPS = 0x0A;         // CCP2 => C2
  CCP2CONbits.MODE = 0x8;  
//  CCPTMRS1.C2TSEL = 0;   
 // CCP2  not enabled until startTimer()
  
  stopTimer();             // disable ints until move or homing cmd
}

void startTimer(){
  stopTimer();
  TMR1L = 0;
  TMR1H = 0;
  CCPR1H = 0;
  CCPR1L = 2;// first pulse happens 2 usecs after starting timer
  CCPR2H = 0;
  CCPR2L = 2;
  CCP1IF = 0;
  CCP2IF = 0;
  CCP1IE = 1;
  CCP2IE = 1;
  CCP1CONbits.EN = 1; // enable CCPs
  CCP2CONbits.EN = 1;      
  TMR1ON = 1;         // start timer
}

void stopTimer(){
  TMR1ON = 0;              // stop timer
}

bool_t isTimerRunning() {
  return TMR1ON;
}

void setNextTimeX(shortTime_t delta) {
  timeX.timeShort += delta;
  CCPR1L = timeX.timeBytes[0];
  CCPR1H = timeX.timeBytes[1];
}

void setNextTimeY(shortTime_t delta) {
  timeY.timeShort += delta;
  CCPR2L = timeY.timeBytes[0];
  CCPR2H = timeY.timeBytes[1];
}
