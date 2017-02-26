
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
  char topSpiByte = spiBytes[3];
  if(errorCode) {
    if (topSpiByte == clearErrorCmd) immediateCmd();
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
    
    case sleepCmd:
      // this stops timer and sets all motor reset pins low
      // issue resetCmd to stop sleeping
      setState(statusSleeping); 
      return;
      
    case resetCmd: 
      // this stops timer and activates motor reset pins
      setState(statusUnlocked); 
      return;
      
    case idleCmd:
      // this stops timer but avoids changing reset pins
      if(RESET_X_LAT) setState(statusLocked);
      else            setState(statusUnlocked);
      initVectors();
      return;
              
    case clearErrorCmd:
      errorAxis = 0;
      errorCode = 0;
      setState(statusUnlocked);
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
