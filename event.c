
#include <xc.h>
#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "spi.h"
#include "vector.h"
#include "motor.h"


Status mcu_status;
Error errorCode;
char errorAxis;

void initEvent() {
  errorCode = 0;
  newStatus(statusUnlocked); 
}

// this also clears time and sets or clears motor reset pins
void newStatus(char newStatus) {
  // timer counting and ints off until move or homing command
  stopTimer();
  if (errorCode) return;
  set_resets(newStatus != statusUnlocked);
  mcu_status = newStatus; 
}

// axis is zero when not specific to axis
void handleError(char axis, Error code) {
  newStatus(statusUnlocked);
  errorAxis = axis;
  errorCode = code;
//  while(1);
}


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

