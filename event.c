
#include <xc.h>
 #include <string.h>

#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "timer.h"
#include "spi.h"
#include "timer.h"
#include "vector.h"
#include "motor.h"

char   mcu_state;
Error  errorCode;
char   errorAxis;
StatusRecU statusRec;
char statusRecIdx = 0;
StatusRecU statusRecOut;
int8_t statusRecOutIdx = STATUS_REC_IDLE;

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
  mcu_state = newState;
}

// axis is zero when not specific to axis
void handleError(char axis, Error code) {
  setState(statusUnlocked);
  errorAxis = axis;
  errorCode = code;
  // wait for SPI idle to repair byte sync and abort word
  while (!SPI_SS); 
  spiWordByteIdx = 0;
  statusRecOutIdx = STATUS_REC_IDLE;
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
      
      // return state, error, or statusRec data in SPI output buf
      // status rec is always surrounded by state bytes
      if (statusRecOutIdx != STATUS_REC_IDLE) {
        if (statusRecOutIdx == STATUS_REC_START) {
          SSP1BUF = (typeState | mcu_state); // state byte before rec
          statusRec.rec.homeDistX = homingDistX;
          statusRec.rec.homeDistY = homingDistY;
          memcpy(&statusRecOut, &statusRec, sizeof(StatusRec)); //snapshot rec
          statusRecIdx = statusRecOutIdx = 0;
        }
        else if(statusRecOutIdx == STATUS_SPI_BYTE_COUNT) {
          SSP1BUF = (typeState | mcu_state); // state byte after rec
          statusRecOutIdx == STATUS_REC_IDLE;
        }
        else {
          switch(statusRecOutIdx % 4) {
            case 0:
              SSP1BUF = typeData | (statusRecOut[statusRecIdx] >> 2);
              break;
            case 1: {
              char left2 = (statusRecOut[statusRecIdx++] & 0x03) << 4;
              SSP1BUF = typeData | left2 | 
                          ((statusRecOut[statusRecIdx]   & 0xf0) >> 4);
              break;
            }
            case 2: {
              char left4 = (statusRecOut[statusRecIdx++] & 0x0f) << 2;
              SSP1BUF = typeData | left4 | 
                          ((statusRecOut[statusRecIdx]   & 0xc0) >> 4);
              break;
            }
            case 3:
              SSP1BUF = typeData | (statusRecOut[statusRecIdx++] & 0x3f);
              break;
          }
          statusRecOutIdx++;
        }
      }
      else if (errorCode) SSP1BUF = (typeError | errorCode | errorAxis);
      else SSP1BUF = (typeState | mcu_state);

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
      if(mcu_state == statusHoming)      chkHomingX();
      else if(mcu_state == statusMoving) chkMovingX();
    } 
    if(CCP2Int) {
      CCP2Int = FALSE;
      // Y step pin was raised by compare
      if(mcu_state == statusHoming)      chkHomingY();
      else if(mcu_state == statusMoving) chkMovingY();
    }
  }
}  

