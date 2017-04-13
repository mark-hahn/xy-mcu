
#include <xc.h>
#include "pins.h"
#include "mcu-api.h"
#include "process.h"
#include "int.h"
#include "util.h"

typedef struct CmdState {
  uint8_t dir;
  uint8_t ustep;
  uint8_t pulseCount;
  bool_t  repeatPulseCount;

  bool_t  setTimeNoPulse;
  bool_t  gettingUsecBytes;
  bool_t  haveByte1;
  bool_t  haveUsecBytes;
  bool_t  waitLimitValue;
  uint8_t byte1;
  bool_t  usecByte2;
} CmdState;

CmdState stateX;

#define startPulseFromUsecsAndReturn()                        \
  { uint16_t newLow = CCPR1L + stateX.byte1;                  \
    CCPR1L = (uint8_t) (newLow & 0xff);                       \
    CCPR1H += stateX.usecByte2 + (uint8_t) (newLow >> 8);     \
    SPI_SS_IOC_IF = 0;                                        \
    return; }

void processCmdX () {
  if(stateX.waitLimitValue) {
    if(LIMIT_SW_X == stateX.waitLimitValue-1) {
      stateX.waitLimitValue   = 0;
      stateX.gettingUsecBytes = FALSE;
    }
    else if(stateX.haveUsecBytes)
      startPulseFromUsecsAndReturn();
  }
  if(stateX.repeatPulseCount && stateX.haveUsecBytes) {
    stateX.repeatPulseCount--;
    startPulseFromUsecsAndReturn();
  }    

getTimeBufByteX:
  if(timBufWrX == timBufRdX){
    // nothing in buffer, check again in 256 to 512 usecs
    CCPR1H = TMR1H + 2; 
    return;
  }
  uint8_t timeBufByte = timBufX[timBufRdX];
  timBufRdX = ((timBufRdX + 1) & 0x7f);

  if(stateX.gettingUsecBytes) {
    if(!stateX.haveByte1) {
      stateX.byte1 = timeBufByte;
      stateX.haveByte1 = TRUE;
      goto getTimeBufByteX;
    }
    stateX.usecByte2 = timeBufByte;
    stateX.haveUsecBytes = TRUE;
    stateX.gettingUsecBytes = FALSE;
    // repeat pulse?
    if(stateX.repeatPulseCount) {
      stateX.repeatPulseCount--;
      startPulseFromUsecsAndReturn();
    }
    // else stateX.waitLimitValue
    if(LIMIT_SW_X == stateX.waitLimitValue-1) {
      stateX.waitLimitValue = 0;
      goto getTimeBufByteX;
    }
    startPulseFromUsecsAndReturn(); // only happens on waitLimitValue
  }
  
  if(stateX.pulseCount) {
    // process stream of timed pulses
    if(!stateX.haveByte1) {
      stateX.byte1 = timeBufByte;
      stateX.haveByte1 = TRUE;
      goto getTimeBufByteX;
    }
    SPI_SS_IOC_IF = 0;    // start pulse                                     
    CCPR1H = timeBufByte;
    CCPR2L = stateX.byte1;
    stateX.haveByte1 = FALSE;
    stateX.pulseCount--;
    return;
  }
  
  if(stateX.setTimeNoPulse) {
    if(!stateX.haveByte1) {
      stateX.byte1 = timeBufByte;
      stateX.haveByte1 = TRUE;
      goto getTimeBufByteX;
    }
    CCPR1L = stateX.byte1;
    CCPR1H = timeBufByte;
    stateX.setTimeNoPulse = FALSE;
    return;
  }  

  stateX.haveByte1 = FALSE;
  
  switch(timeBufByte & 0xe0) {
    case 0xc0:
      // 110A DUUU settings
      stateX.dir   = (timeBufByte & 0x08) >> 3;
      stateX.ustep = (timeBufByte & 0x07);
      goto getTimeBufByteX;

    case 0x80:
    case 0xa0:
      // 10CC CCCC pulse stream: C timed pulses (2 bytes each) follow, C > 0
      stateX.pulseCount = (timeBufByte & 0x3f);
      if(!stateX.pulseCount) 
        // 1000 0000 delay: 2 byte time pair follows
        stateX.setTimeNoPulse = TRUE;
      goto getTimeBufByteX;

    case 0x00:
      // 0001 CCCC set motor current, C: 0-15
      uint8_t current = (timeBufByte & 0x0f);
#ifdef XY
      DAC1CON1bits.DAC1R = current;
#endif
#ifdef Z2
      setPwmVref(current);
#endif
      goto getTimeBufByteX;

    case 0xe0:
      if((timeBufByte & 0x10) == 0) {
        // 1110 CCCC repeat pulse, 2 byte usec pair that follows, C times
        stateX.repeatPulseCount = (timeBufByte & 0x0f);
        stateX.gettingUsecBytes = TRUE;
        goto getTimeBufByteX;
      }
      else if((timeBufByte & 0xfc) == 0xf8) {
        // 1111 10SS set status bits for CPU (e.g. homing done)
        cmdStateBitsX |= (timeBufByte & 0x03);
        goto getTimeBufByteX;
      }
      else {
        // 1111 110L repeat pulse, 2 byte usec pair that follows, until limit sw == L
        stateX.waitLimitValue = (timeBufByte & 0x01) + 1;
        stateX.gettingUsecBytes = TRUE;
        goto getTimeBufByteX;
      }

    case 0x00:
      if(timeBufByte == 0x0c) {
        // 0000 1100 clear distance counters              TODO
      }
      else {
        // 0000 1101 save distance counters, set flag     TODO
      }
  }
}

