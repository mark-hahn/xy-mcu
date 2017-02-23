
#include <xc.h>
#include "timer.h"
#include "main.h"
#include "pins-b.h"

// these are loaded into CCP compare regs in interrupt
time_ut timeX;
time_ut timeY;

void initTimer() {
  LATC6 = !LATC6;
  
  // timer 1 -- used only by CCPs below
  T1CLKbits.CS   = 1;      // Fosc/4, 8 MHz before prescale
  T1CONbits.CKPS = 3;      // 11 = 1:8 prescale value, 1 MHz
  T1GCONbits.GE = 0;       // always on, no gate
  TMR1ON = 1;              // start timer
   
  // CCP 1 -- X step pin from counter compare 1
  CCP1_LAT  = 1;  // always one and only used when CCP disabled
  CCP1_TRIS = 0;          
  CCP1_PPS  = 0x09;         // CCP1 => B4
//  CCP1CONbits.MODE = 0xa;  // pulse DEBUG
  CCP1CONbits.MODE = 0x8;  // Compare mode, set step high
  CCP1IE = 0;

  // CCP 2 -- Y step pin from counter compare 2
  CCP2_LAT  = 1;  // always one and only used when CCP disabled
  CCP2_TRIS = 0;           
  CCP2_PPS  = 0x0A;         // CCP2 => C2
//  CCP2CONbits.MODE = 0xa;                   // DEBUG
  CCP2CONbits.MODE = 0x8;  
  CCP2IE = 0;

// wait for both outputs to go high, may take 65 ms
  CCP1CONbits.EN = 1;
  CCP2CONbits.EN = 1;
  while(!CCP1CONbits.OUT || !CCP2CONbits.OUT);
  CCP1CONbits.EN = 0;
  CCP2CONbits.EN = 0;
  
//  while(1){LATC6 = !LATC6;};
 }

void stopTimerX() {
  CCP1CONbits.EN = 0;
}
void stopTimerY() {
  CCP2CONbits.EN = 0;
}
 
void setNextTimeX(shortTime_t delta, bool_t startPulse) {
  timeX.timeShort += delta;
  CCP1IE = 0;
  CCP1CONbits.EN = 1;
  if(startPulse) {
    char hi, lo, newLo;
    do {
      hi = TMR1H;
      lo = TMR1L;  
    } while (hi != TMR1H);
    char newLo = lo + 20;// short 20 us delay to lower cp out
    if(newLo < lo) hi++;
    CCP1CONbits.MODE = 0x09; // set step low on compare match
    CCPR1L = newLo;
    CCPR1H = hi;
    char latc6 = LATC6;
    // wait for cp to go low
    while(CCP1CONbits.OUT){
      LATC6 = !LATC6;
    };
    LATC6 = latc6;
    CCPR1H = timeX.timeBytes[1]; // accidental match will just keep cp low
    CCPR1L = timeX.timeBytes[0];
    CCP1CONbits.MODE = 0x8;  // set step high on compare match 
  }
  else {
    CCPR1H = timeX.timeBytes[1];// accidental match will just keep cp high
    CCPR1L = timeX.timeBytes[0];
  }
  CCP1IF = 0;
  CCP1IE = 1;
}

void setNextTimeY(shortTime_t delta, bool_t startPulse) {
  timeY.timeShort += delta;
  CCP2IE = 0;
  CCP2CONbits.EN = 1;
  if(startPulse) {
    char hi, lo, newLo;
    do {
      hi = TMR1H;
      lo = TMR1L;  
    } while (hi != TMR1H);
    char newLo = lo + 20;// short 20 us delay to lower cp out
    if(newLo < lo) hi++;
    CCP2CONbits.MODE = 0x09; // set step low on compare match
    CCPR2L = newLo;
    CCPR2H = hi;
    while(CCP2CONbits.OUT); // wait for cp to go low
    CCPR2H = timeY.timeBytes[1]; // accidental firing will just keep cp low
    CCPR2L = timeY.timeBytes[0];
    CCP2CONbits.MODE = 0x8;  // set step high on compare match 
  }
  else {
    CCPR2H = timeY.timeBytes[1];// accidental firing will just keep cp high
    CCPR2L = timeY.timeBytes[0];
  }
  CCP2IF = 0;
  CCP2IE = 1;
}
