
#include <xc.h>
#include "event.h"
#include "mcu-cpu.h"
#include "pins-b.h"
#include "main.h"
#include "spi.h"
#include "vector.h"
#include "motor.h"

char spiZeroCount = 0;

// called once from main.c and never returns
void eventLoop() {
  while(1) {
    if(spiInt) {
       // an spi data exchange just happened
      spiInt = FALSE;
      LATC6 = 1;
      // four zero bytes or more in a row synchronize the word boundary
      // this can be sent any time since a word with 1st byte of zero is a nop
      if (spiByteFromCpu == 0) {
        spiZeroCount++;
//        spiByteToCpu = spiZeroCount + 0xA0;
      } 
      else {
        spiZeroCount = 0;
//        spiByteToCpu = spiZeroCount + 0xB0;
      }
      if (spiZeroCount > 3) {
        spiZeroCount = 4; // may get more than 256 zeros
        spiWordByteIdx = 0;
        LATC6 = 0;
        spiByteToCpu = (spiByteFromCpu & 0x0f) + 0xC0;
        continue;
      }
      spiByteToCpu = ~spiByteFromCpu; //spiWordByteIdx + 0xD0;

      ((char *) &spiWordIn)[3-spiWordByteIdx] = spiByteFromCpu;
      if(spiWordByteIdx == 3) {
        // last byte of a complete 32-bit word (spiWordIn) arrived
        handleSpiWordInput();
        spiWordByteIdx == 0; 
      }
      else
        spiWordByteIdx++;
//      getOutputByte(); // sets spiByteToCpu
      LATC6 = 0;
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

