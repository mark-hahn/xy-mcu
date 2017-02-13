
#include <xc.h>
#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "spi.h"
#include "vector.h"
#include "motor.h"

// called once from main.c and never returns
void eventLoop() {
  while(1) {
    if(spiInt) {
      LATC6 = 0;
      spiInt = FALSE;
      if(spiWordByteIdx == 4 && spiWordIn != 0) {
        // last byte of a complete 32-bit word (spiWordIn) arrived
        LATC6 = 1; 
        handleSpiWordInput();
        spiWordByteIdx = 0; 
        SSP1BUF =  (errorAxis << 7) | (mcu_status << 4) | errorCode;
        LATC6 = 0;
      }
//      getOutputByte(); // sets spiByteToCpu
      if(SPI_SS) spiWordByteIdx = 0;
      LATC6 = 1;
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

