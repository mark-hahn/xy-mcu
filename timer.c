
#include <xc.h>
#include "timer.h"

// this starts rising step edges and interrupts which never end

void initTimer() {
  T1CLKbits.CS   = 1;      // Fosc/4, 8 MHz before prescale
  T1CONbits.CKPS = 3;      // 11 = 1:8 prescale value, 1 MHz
  T1GCONbits.GE = 0;       // always on, no gate
  TMR1ON = 1;              // enable timer 1
   
  // X step pin from counter compare 1
  LATB4 = 1;               // start step pin high
  TRISB4 = 0;              // step pin output
  RB4PPS = 0x09;           // CCP1 => B4
  CCP1CONbits.MODE = 0x80; // Compare mode, set step high
//  CCPTMRS0.C1TSEL = 0;     // CCP1 matches timer 1  ???
  CCP1IF = 0;              // start with int flag off
  CCP1IE = 1;              // enable int
  CCP1CONbits.EN = 1;      // enable CCP1
 
  // Y step pin from counter compare 2
  LATC2 = 1;               // start step pin high
  TRISC2 = 0;              // step pin output
  RC2PPS = 0x0A;           // CCP2 => C2
  CCP2CONbits.MODE = 0x80; // Compare mode, set step high
//  CCPTMRS1.C2TSEL = 0;     // CCP2 matches timer 1  ???
  CCP2IF = 0;              // start with int flag off
  CCP2IE = 1;              // enable int
  CCP2CONbits.EN = 1;      // enable CCP2
}

/*
 CCPR1L, CCPR1H, CCPR2L, CCPR2H
 */
