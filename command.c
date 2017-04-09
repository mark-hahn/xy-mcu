
#include <xc.h>
#include "command.h"
#include "vector.h" 
#include "main.h"
#include "spi.h"
#include "pins.h"
#include "mcu-cpu.h"
#include "motor.h"
#include "event.h"
#include "parse-spi.h"

void immediateCmd(Cmd cmd);

void handleSpiWord() {
  if ((spiBytes[3] & 0xc0) == 0x80) {
    Cmd cmd = (spiBytes[3] & 0x1f);
    if(errorCode && cmd != clearErrorCmd) 
      return;
    immediateCmd(cmd);
  }
  else {
#ifdef XY
    if(axisFromSpiWord(&spiWord)) putVectorY(); 
    else                          putVectorX();
#endif
#ifdef Z2
    putVectorX();
#endif
  }
}

void immediateCmd(Cmd cmd) {
  uint8_t ofs;
  int16_t val;
  
  switch (cmd) {
    case moveCmd: 
      startMoving();
      return;
      
    case homeCmd:  
      startHoming();
      return;

    case statusCmd:
      statusRecOutIdx = STATUS_REC_START;
      return;
    
    case lockCmd: 
      // this stops timer and deactivates motor reset pins (pins high)
      setState(statusLocked); 
      return;
      
    case unlockCmd: 
      // this stops timer and activates motor reset pins (pins low)
      setState(statusUnlocked); 
      return;
      
    case idleCmd:
      // clears vectors and stops timer but doesn't change lock state
      if(RESET_X_LAT) setState(statusLocked);
      else            setState(statusUnlocked);
      initVectors();
      return;
      
    case clearDistance:
      distanceX = 0;
#ifdef XY
      distanceY = 0;
#endif
      return;
              
    case clearErrorCmd:
      errorCode = 0;
      setState(statusUnlocked);
      initVectors();
      return;
      
    case settingsCmd:
      ofs = spiBytes[2];
      val = *((int16_t *) &spiInts[1]);
      settings[ofs] = val;
      if (spiBytes[2] == motorCurrent) setMotorCurrent(val);
      return;
    
    case updateFlashCode:
      setState(statusFlashing);

      // clear beginning of app code so bootloader with load code at reset
      NVMADRH = 0x02; // erase one block at 0x200
      NVMADRL = 0x00;
      NVMCON1 = 0x94; // access FLASH memory, wren=1, FREE specifies erase 
      NVMCON2 = 0x55;
      NVMCON2 = 0xaa;
      NVMCON1bits.WR = 1;
      RESET();
  }
}

