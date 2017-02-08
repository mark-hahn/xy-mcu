
#include <xc.h>
#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "spi.h"
#include "vector.h"
#include "motor.h"

// called once from main.c and never returns
// events are dealt with in order of most urgent first
// SPI is highest priority in event loop and lowest in interrupt routine
void eventLoop() {
  while(1) {  
    if(SPI_SS && spiWordByteIdx > 0 && spiWordByteIdx < 4) {
      // SS is idle (high) between bytes of word, fix byte idx
      handleError(0, errorSpiSync);
      spiWordByteIdx == 0; 
      spiIntHappened = FALSE;  // don't use bad data
      return;  
    }
    if(spiIntHappened) {
      // an spi data exchange just happened
      spiByteToCpu = 0; // in case chkStatusWord below is too late
      ((char *) &spiWordIn)[spiWordByteIdx] = spiByteFromCpu;
      if(spiWordByteIdx == 3) {
        // last byte of a complete 32-bit word (spiWordIn) arrived
        handleSpiWordInput();
        spiWordByteIdx == 0;
      }
      else
        spiWordByteIdx++;
      getOutputByte(); // sets spiByteToCpu
      spiIntHappened == FALSE;
    }
    // if error, no homing or moving happens until clearError cmd
    if(errorCode) continue;

    if(isMovingX && CCP1_PIN) { 
      // X step pin was raised by compare
      if(mcu_status == statusHoming) 
        chkHomingX();
      else if(mcu_status == statusMoving) 
        chkMovingX();
    }
    if(isMovingY && CCP2_PIN) {
      // Y step pin was raised by compare
      if(mcu_status == statusHoming) 
        chkHomingY();
      else if(mcu_status == statusMoving) 
        chkMovingY();
    }
  }
}  

