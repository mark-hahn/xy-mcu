
#include <xc.h>
#include "command.h"
#include "vector.h"
#include "dac.h"
#include "main.h"
#include "spi.h"
#include "pins-b.h"
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
    case 0x40: putVectorY();   break;
    case 0x00: immediateCmd(); break;
  }
}

void immediateCmd() {
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
    
    case resetCmd: 
      // this stops timer and activates motor reset pins
      setState(statusUnlocked); 
      initVectors();
      return;
      
    case idleCmd:
      // this stops everything but doesn't change reset pins
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
      
    case setDirectionLevelXY:
      directionLevelXY(spiBytes[0]);
      return;
  }
}
