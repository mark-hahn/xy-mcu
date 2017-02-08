
#include <xc.h>
#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "spi.h"
#include "vectors.h"
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
      spiByteToCpu = 0; // in case chkStatusWord below is too late
      // an spi data exchange just happened
      ((char *) &spiWordIn)[spiWordByteIdx] = spiByteFromCpu;
      if(spiWordByteIdx == 3) {
        // last byte of a complete 32-bit word (spiWordIn) arrived
        handleNewSpiWord();
        spiWordByteIdx == 0;
      }
      else
        spiWordByteIdx++;
      chkStatusWord(); // sets spiByteToCpu
      spiIntHappened == FALSE;
    }
    // if error, no homing or moving happens until clearError cmd
    if(errorCode) continue;

    if(isPulsingX && CCP1_PIN) { 
      // X step pin was raised by compare
      if(mcu_status == statusHoming) 
        chkHomingX();
      else 
        chkMovingX();
    }
    if(isPulsingY && CCP2_PIN) {
      // Y step pin was raised by compare
      if(mcu_status == statusHoming) 
        chkHomingY();
      else 
        chkMovingY();
    }
  }
}  

