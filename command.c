
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

void immediateCmd();

void handleSpiWord() {
  Cmd topSpiByte = spiBytes[3];
  if(errorCode) {
    if (topSpiByte == clearErrorCmd) immediateCmd();
    // else all other vectors/commands are ignored
  } 
  else if ((topSpiByte & 0x80) == 0) immediateCmd();
  
  else {
#ifdef XY
    if ((topSpiByte & 0xe0) == 0x80) {       // velocity
      if(topSpiByte & 0x10) putVectorY(); else putVectorX();
    }
    else if ((topSpiByte & 0xfe) == 0xfe) {  // acceleration
      if(topSpiByte & 0x01) putVectorY(); else putVectorX();
    }
    else { // curve
      if(axisFromSpiWord(&spiWord)) putVectorY(); else putVectorX();
    }
#endif
#ifdef Z2
  putVectorX();
#endif
  }
}

void immediateCmd() {
  // word is big-endian, mcu isn't
  char cmd = spiBytes[3];
  switch (spiBytes[3]) {
    
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
              
    case clearErrorCmd:
      errorCode = 0;
      setState(statusUnlocked);
      initVectors();
      return;
      
    case setHomingSpeed:
      homingSpeed();
      return;
    
    case setHomingBackupSpeed:
      homingBackupSpeed();
      return;
      
    case setMotorCurrent: 
      motorCurrent(spiBytes[0]);
      return;
    
    case setDirectionLevels:
      directionLevels(spiBytes[0]);
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

