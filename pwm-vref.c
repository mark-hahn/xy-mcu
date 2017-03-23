
#ifdef Z2

#include <xc.h>
#include "main.h"
#include "pins.h"

void initPwmVref() {
  // timer 2 -- input to pwm module
  T2CLKCON          = 1; // clk source FOSC/4 => 8 MHz (MUST BE FOSC/4)
  T2CONbits.T2CKPS  = 0; // prescale 1:1
  PR2               = 0x3f; // period count 64 =>  125KHz (8 bit resolution)
  T2CONbits.T2OUTPS = 0; // postscaler set to 1:1
  T2HLTbits.PSYNC   = 1; // Prescaler Output is not synchronized to Fosc/4
  T2HLTbits.CKPOL   = 0; // Rising edge of input clock clocks timer/prescaler
  T2HLTbits.CKSYNC  = 1; // ON register bit is synchronized to TMR2_clk input
  T2HLTbits.T2MODE  = 0; // Free Running Period with software gate
  TMR2IE            = 0; // no interrupts on timer 2
  T2CONbits.ON      = 1; // enable timer

  // pwm module 3
  PWM_VREF_PPS  = PWM3OUT_PPS; // select pwm3 to output on pin RA2
  PWM_VREF_TRIS = 0;  // output
  PWM3POL       = 0;  // default pin output is low-level
  PWM3DCL       = 0; // bot 2 lsb bits are in d7-d6
  PWM3DCH       = 0; // top 6 bits of 8-bit duty cycle are in d5-d0
  PWM3EN        = 1;  // enable pwm
}

// V = vref / 255;  A = 2 * V;
// measured 20:0.1v, 25:0.12v, 128:0.55v, 255:1.07
void setPwmVref(char vref) {
  PWM3DCL =  vref << 6;
  PWM3DCH = (vref & 0xfc) >> 2;
}

#endif
