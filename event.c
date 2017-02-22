
#include <xc.h>
#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "timer.h"
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

uint32_t spiWord;

// called once from main.c and never returns
void eventLoop() {
  while(1) {
    if(spiInt) {
      // last byte of a complete 32-bit word (spiWordIn) arrived
      LATC6 = 0;
      spiInt = FALSE;
      spiWord = spiWordIn;
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
           LATC6 = !LATC6;
        }
      }
      // this must be finished when the next 32-bit word arrives
      if(spiWord != 0) {
        LATC6 = 1;
        LATC6 = 0; 
        LATC6 = 1; 
        handleSpiWordInput();
        LATC6 = 0;         
        LATC6 = 1; 
        LATC6 = 0;
      }
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

