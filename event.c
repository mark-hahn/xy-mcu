
#include <xc.h>
#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "timer.h"
#include "spi.h"
#include "timer.h"
#include "vector.h"
#include "motor.h"

Error errorCode;
char errorAxis;
char stateRecIdx = 0;
StatusRecU statusRec;

void initEvent() {
  errorCode = 0;
  setState(statusUnlocked); 
}

// this also clears time and sets or clears motor reset pins
void setState(char newState) {
  // timer counting and ints off until move or homing command
  stopTimerX();
  stopTimerY();
  if (errorCode) {
    set_resets(MOTORS_RESET);
    return;
  }
  if(newState == statusSleeping) set_sleep();
  else {
    STEP_X_LAT= 1; // turn off sleep by setting steps idle
    STEP_Y_LAT= 1;
    if(newState == statusUnlocked) set_resets(MOTORS_RESET);
    else set_resets(MOTORS_NOT_RESET);
  }
  statusRec.rec.state = newState;
}

// axis is zero when not specific to axis
void handleError(char axis, Error code) {
  setState(statusUnlocked);
  errorAxis = axis;
  errorCode = code;
  // wait for SPI idle to repair byte sync and abort word
  while (!SPI_SS); 
  spiWordByteIdx = 0;
}

uint32_t spiWord;

// called once from main.c and never returns
void eventLoop() {
  while(1) {
    if(intError) 
      handleError(0,intError);
      intError = 0;
      spiInt = CCP1Int = CCP2Int = FALSE;
    }
    if(spiInt) {
      // last byte of a complete 32-bit word (spiWordIn) arrived
      spiWord = spiWordIn;
      spiInt = FALSE;
      spiWordByteIdx = 0;  
        
      // this is really slow (10us) ==   TODO
      // always return status in first byte
      SSP1BUF = (errorAxis << 7) | (mcu_status << 4) | errorCode;
      // we should be between words with SS high (off)
      if(!SPI_SS) {
        handleError(0,errorSpiByteSync);
        // stall to get back in sync
        // keep resetting int data so zeros are received
        while(!SPI_SS) {
          spiWordIn = 0;
          spiWordByteIdx = 0; 
        }
      } 
      // this must be finished when the next 32-bit word arrives
      if(spiWord != 0) {
        handleSpiWordInput();
      }
    }
    // if error, no homing or moving happens until clearError cmd
    if(errorCode) continue;

    if(CCP1Int) { 
      CCP1Int = FALSE;
      // X step pin was raised by compare
      if(mcu_status == statusHoming)      chkHomingX();
      else if(mcu_status == statusMoving) chkMovingX();
    } 
    if(CCP2Int) {
      CCP2Int = FALSE;
      // Y step pin was raised by compare
      if(mcu_status == statusHoming)      chkHomingY();
      else if(mcu_status == statusMoving) chkMovingY();
    }
  }
}  

