
#include <xc.h>
#include "command.h"
#include "vector.h"
#include "main.h"
#include "spi.h"
#include "pins.h"
#include "mcu-cpu.h"
#include "motor.h"
#include "event.h"

void immediateCmd();

void handleSpiWord() {
  Cmd topSpiByte = spiBytes[3];
  if(errorCode) {
    if (topSpiByte == clearErrorCmd) immediateCmd();
    // else all other vectors/commands not allowed
  } 
  else switch(topSpiByte & 0xc0) {
    case 0x80: putVectorX();   break;
#ifdef XY
    case 0x40: putVectorY();   break;
#endif
    case 0x00: immediateCmd(); break;
  }
}

void immediateCmd() {
//   for(char i=0; i < spiBytes[3]*2; i++) PWM_LAT = !PWM_LAT;

  // word is big-endian, mcu isn't
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
//      for(char i=0; i < 10; i++) PWM_LAT = !PWM_LAT;

      // clear beginning of app code so bootloader with load code at reset
      NVMADRH = NEW_RESET_VECTOR >> 8; // erase one block at 0x200
      NVMADRL = NEW_RESET_VECTOR & 0xff;
      NVMCON1 = 0x94; // access FLASH memory, wren=1, FREE specifies erase 
      NVMCON2 = 0x55;
      NVMCON2 = 0xaa;
      NVMCON1bits.WR = 1;
      RESET();
  }
}

