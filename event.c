
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
  stopTimerX();
  stopTimerY();
  if (errorCode) return;
  set_resets(newStatus != statusUnlocked);
  mcu_status = newStatus; 
}

// axis is zero when not specific to axis
void handleError(char axis, Error code) {
  newStatus(statusUnlocked);
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

