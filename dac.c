

#include <xc.h>
#include "dac.h"
#include "motor.h"


void initDac() {
  DAC1CON1bits.DAC1R  = 0;  
  DAC1CON0bits.OE1    = 1;
  DAC1CON0bits.OE2    = 0;
  DAC1CON0bits.PSS    = 0;
  DAC1CON0bits.NSS    = 0;
  DAC1CON0bits.EN     = 1;
  set_dac(defMotorCurrent);
}

