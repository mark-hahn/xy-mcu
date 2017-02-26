
#include <xc.h>
#include <string.h>

#include "event.h"
#include "command.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "timer.h"
#include "spi.h"
#include "timer.h"
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
  spiBytesInIdx = 0;
  statusRecOutIdx = STATUS_REC_IDLE;
}

// called once from main.c and never returns
void eventLoop() {
  while(1) {
    if(SSP1CON1bits.SSPOV) { // spi input overflow
      handleError(0, errorSpiOvflw);
      SSP1CON1bits.SSPOV = 0;
      spiInt = CCP1Int = CCP2Int = FALSE;
    }
    if(SSP1CON1bits.WCOL) { // spi write collision
      handleError(0, errorSpiWcol);
      SSP1CON1bits.WCOL = 0;
      spiInt = CCP1Int = CCP2Int = FALSE;
    }
    if(intError) {
      handleError(0, intError);
      intError = spiBytesInIdx = 0;
      spiInt = CCP1Int = CCP2Int = FALSE;
    }
    if(spiInt) {
      FAN_LAT = 1;
      // a little-endian 32-bit word (spiBytesIn) arrived (SS went high)
      // copy to buffer interrupt version
      spiWord = *((uint32_t *) &spiBytesIn);
      // use spiInts for structs with 2 uint16_t
      spiInts[0] = *((uint16_t *) &spiBytesIn[2]);
      spiInts[1] = *((uint16_t *) &spiBytesIn[0]);
      // little-endian array version of copied word
      spiBytes = ((char *) &spiWord);
      
      spiBytesInIdx = 0;   
      
      // return state, error, or statusRec data in SPI output buf
      // status rec is always surrounded by state bytes
      if (statusRecOutIdx != STATUS_REC_IDLE) {
        if (statusRecOutIdx == STATUS_REC_START) {
          // state byte before rec
          if(errorCode) SSP1BUF = (typeState | spiStateByteErrFlag | mcu_state);
          else          SSP1BUF = (typeState | mcu_state);
          statusRec.rec.homeDistX = homingDistX;
          statusRec.rec.homeDistY = homingDistY;
          memcpy(&statusRecOut, &statusRec, sizeof(StatusRec)); //snapshot rec
          statusRecIdx = statusRecOutIdx = 0;
        }
        else if(statusRecOutIdx == STATUS_SPI_BYTE_COUNT) {
          // state byte after rec
          if(errorCode) SSP1BUF = (typeState | spiStateByteErrFlag | mcu_state);
          else          SSP1BUF = (typeState | mcu_state);
          statusRecOutIdx == STATUS_REC_IDLE;
        }
        else {
          switch(statusRecOutIdx % 4) {
            case 0:
              SSP1BUF = (typeData | (statusRecOut.bytes[statusRecIdx] >> 2));
              break;
            case 1: {
              char left2 = (statusRecOut.bytes[statusRecIdx++] & 0x03) << 4;
              SSP1BUF = typeData | left2 | 
                          ((statusRecOut.bytes[statusRecIdx]   & 0xf0) >> 4);
              break;
            }
            case 2: {
              char left4 = (statusRecOut.bytes[statusRecIdx++] & 0x0f) << 2;
              SSP1BUF = typeData | left4 | 
                          ((statusRecOut.bytes[statusRecIdx]   & 0xc0) >> 4);
              break;
            }
            case 3:
              SSP1BUF = typeData | (statusRecOut.bytes[statusRecIdx++] & 0x3f);
              break;
          }
          statusRecOutIdx++;
        }
      }
      else if (errorCode) SSP1BUF = (typeError | errorCode | errorAxis);
      else SSP1BUF = (typeState | mcu_state);

      if(spiWord != 0) handleSpiWord();
      
      // spiInt must be cleared before next 32-bit word arrives (SS high)
      spiInt = FALSE;
      FAN_LAT = 0;
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

