
#include <xc.h>
#include "timer.h"
#include "pins-b.h"

// bytes are set between timer interrupts for quick use in int
char ccpXLowByte;
char ccpXHighByte;
char ccpYLowByte;
char ccpYHighByte;

// this starts rising step edges and interrupts which never end
void initTimer() {
  T1CLKbits.CS   = 1;      // Fosc/4, 8 MHz before prescale
  T1CONbits.CKPS = 3;      // 11 = 1:8 prescale value, 1 MHz
  T1GCONbits.GE = 0;       // always on, no gate
  TMR1ON = 1;              // enable timer 1
   
  // X step pin from counter compare 1
  CCP1_LAT = 1;            // start step pin high
  CCP1_TRIS = 0;           // step pin output
  CCP1_PPS = 0x09;         // CCP1 => B4
  CCP1CONbits.MODE = 0x8;  // Compare mode, set step high
//  CCPTMRS0.C1TSEL = 0;   // CCP1 matches timer 1  ???
  CCP1IF = 0;              // start with int flag off
  CCP1IE = 1;              // enable int
  CCP1CONbits.EN = 1;      // enable CCP1
 
  // Y step pin from counter compare 2
  CCP2_LAT = 1;            // start step pin high
  CCP2_TRIS = 0;           // step pin output
  CCP2_PPS = 0x0A;         // CCP2 => C2
  CCP2CONbits.MODE = 0x8; // Compare mode, set step high
//  CCPTMRS1.C2TSEL = 0;     // CCP2 matches timer 1  ???
  CCP2IF = 0;              // start with int flag off
  CCP2IE = 1;              // enable int
  CCP2CONbits.EN = 1;      // enable CCP2
}

/*
 CCPR1L, CCPR1H, CCPR2L, CCPR2H
 */
