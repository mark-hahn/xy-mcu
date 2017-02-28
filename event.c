
#include <xc.h>
#include <string.h>

#include "event.h"
#include "command.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "vector.h"
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
    STEP_X_LAT= 1; // turn off sleep by setting step pins idle
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

// d5: error flag (if error, then no high-waters sent)
// d4-d3: vector buf high-water flags for X and Y
// d2-d0: status code (mcu_state)
void sendStateByte() {
  char outbuf = 0;
  if(errorCode) {
    outbuf = (typeState | spiStateByteErrFlag | mcu_state);
  } else {
    outbuf = (typeState                    | 
             (vecBufXIsAtHighWater() << 4) |
             (vecBufYIsAtHighWater() << 3) |
              mcu_state);
  }
  if(outbuf && SPI_SS) SSP1BUF = outbuf;
}

void sendStatusRecByte() {
  char outbuf = 0;
  if (statusRecOutIdx == STATUS_REC_START) {
    // state byte before rec
    sendStateByte();
    statusRec.rec.homeDistX = homingDistX;
    statusRec.rec.homeDistY = homingDistY;
    memcpy(&statusRecOut, &statusRec, sizeof(StatusRec)); //snapshot rec
    statusRecIdx = statusRecOutIdx = 0;
  }
  else if(statusRecOutIdx == STATUS_SPI_BYTE_COUNT) {
    // state byte after rec
    sendStateByte();
    statusRecOutIdx == STATUS_REC_IDLE;
  }
  else {
    // pack every 3 8-bit statusRec bytes into 4 6-bit spi bytes 
    switch(statusRecOutIdx % 4) {
      case 0: 
        if(SPI_SS) 
          outbuf = (typeData | (statusRecOut.bytes[statusRecIdx] >> 2));
        break;
      case 1: {
        char left2 = (statusRecOut.bytes[statusRecIdx++] & 0x03) << 4;
        if(SPI_SS) 
          outbuf = typeData | left2 | 
                  ((statusRecOut.bytes[statusRecIdx]   & 0xf0) >> 4);
        break;
      }
      case 2: {
        char left4 = (statusRecOut.bytes[statusRecIdx++] & 0x0f) << 2;
        if(SPI_SS) 
          outbuf = typeData | left4 | 
                  ((statusRecOut.bytes[statusRecIdx]   & 0xc0) >> 4);
        break;
      }
      case 3:
        if(SPI_SS) 
          outbuf = typeData | (statusRecOut.bytes[statusRecIdx++] & 0x3f);
        break;
    }
    statusRecOutIdx++;
  }
  if(outbuf && SPI_SS) SSP1BUF = outbuf;
}

// called once from main.c and never returns
void eventLoop() {
  char outbuf;
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
    
    // check for SPI word input event
    if(spiInt) {
      // a little-endian 32-bit word (spiBytesIn) arrived (SS went high)

      FAN_LAT = 1;

      // copy word to buffered interrupt version (global))
      spiWord    = *((uint32_t *) &spiBytesIn);
      
      // use spiInts for structs with 2 uint16_t (global))
      spiInts[0] = *((uint16_t *) &spiBytesIn[2]);
      spiInts[1] = *((uint16_t *) &spiBytesIn[0]);
      
      // little-endian array version of spiWord (global))
      spiBytes   = ((char *) &spiWord);
      
      // return state, error, or statusRec data in SPI output buf
      // status rec is always surrounded by state bytes
      if (statusRecOutIdx != STATUS_REC_IDLE)
        sendStatusRecByte();
   
      else if (errorCode) {
        outbuf = (typeError | errorCode | errorAxis);
        if(SPI_SS) SSP1BUF = outbuf;
      }
      else 
        sendStateByte();

      // process input word
      if(spiWord != 0) handleSpiWord();
      
      // spiInt must be cleared before next 32-bit word arrives (SS high)
      spiInt = FALSE;
      
      FAN_LAT = 0;
    }

    // if error, no homing or moving happens until clearError cmd
    if(errorCode) continue;

    // check for motor pulse end events
    if(CCP1Int) { 
      CCP1Int = FALSE;
      // X step pin was raised by int routine
      if(mcu_state == statusHoming)      chkHomingX();
      else if(mcu_state == statusMoving) chkMovingX();
    } 
    if(CCP2Int) {
      CCP2Int = FALSE;
      // Y step pin was raised by int routine
      if(mcu_state == statusHoming)      chkHomingY();
      else if(mcu_state == statusMoving) chkMovingY();
    }
  }
}  

