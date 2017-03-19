
#ifdef Z2

#include <xc.h>
#include "pins.h"

void initPwmVref() {
  // timer 2 -- input to pwm module
  T2CLKCON          = 2; // clk source FOSC => 32 MHz
  T2CONbits.T2CKPS  = 0; // prescale 1:1
  PR2               = 0x40; // period count 64 =>  500KHz (6 bit resolution)
  T2CONbits.T2OUTPS = 0; // postscaler set to 1:1
  T2HLTbits.PSYNC   = 0; // Prescaler Output is not synchronized to Fosc/4
  T2HLTbits.CKPOL   = 0; // Rising edge of input clock clocks timer/prescaler
  T2HLTbits.CKSYNC  = 1; // ON register bit is synchronized to TMR2_clk input
  T2HLTbits.T2MODE  = 0; // Free Running Period with software gate
  T2CONbits.ON      = 1; // enable timer
  TMR2IE            = 0; // no interrupts on timer 2
  
  // pwm module 3
  PWM_VREF_PPS  = 0x0b; // select pwm3 to output on pin RA2
  PWM_VREF_TRIS = 0;  // output
  PWM3DCL  = 0;  // bot 2 lsb bits are in d7-d6
  PWM3DCH  = 0;  // top 4 bits of 5-bit duty cycle is in d3-d0
  PWM3POL  = 0;  // default pin output is low-level
  PWM3EN   = 1;  // enable pwm
}

void setPwmVref(char vref) {
  PWM3DCL =  vref << 6;
  PWM3DCH = (vref & 0x3c) >> 2;
}

#endif
