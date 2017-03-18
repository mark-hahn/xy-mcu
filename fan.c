
#ifdef XY

#include <xc.h>
#include "fan.h"
#include "pins.h"
#include "mcu-cpu.h"
#include "timer.h"


void initFan() {
  // timer 2 -- input to pwm module
  // brushless fan needs a really slow period (20 to 160 Hz)
  T2CLKCON          = 6; // clk source MFINTOSC => 31.25 kHz
  T2CONbits.T2CKPS  = 7; // prescale 128:1 => 244 Hz
  PR2               = 8; // period count 8 =>  30 Hz (5 bit resolution)
  T2CONbits.T2OUTPS = 0; // postscaler set to 1:1
  T2HLTbits.PSYNC   = 0; // Prescaler Output is not synchronized to Fosc/4
  T2HLTbits.CKPOL   = 0; // Rising edge of input clock clocks timer/prescaler
  T2HLTbits.CKSYNC  = 1; // ON register bit is synchronized to TMR2_clk input
  T2HLTbits.T2MODE  = 0; // Free Running Period with software gate
  T2CONbits.ON      = 1; // enable timer
  TMR2IE            = 0; // no interrupts on timer 2
  
  // pwm module 3
  RC1PPS   = 11; // select pwm3 to output on pin RC1
  PWM_TRIS = 0;  // output
  PWM3DCL  = 0;  // bot 2 lsb bits are in d7-d6
  PWM3DCH  = 0;  // top 3 bits of 5-bit duty cycle is in d2-d0
  PWM3POL  = 0;  // default pin output is low-level
  PWM3EN   = 1;  // enable pwm
}

// Speed param is 0 to 32 for 0 to 100% pwm duty cycle.
// Any value over 31 gives 100%.
// A test with a 12V supply and 5V fan wouldn't reliably start 
// below a param value of 12 (38%), which is equivalent to 4.5 volts,
// but it seemed to run fine at a full 100% 12V :-)
void setFanSpeed(char speed) {
  PWM3DCL =  speed << 6;
  PWM3DCH = (speed & 0x1c) >> 2;
}

#endif
