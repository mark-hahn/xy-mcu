

#include <xc.h>
#include "pins-b.h"
#include "spi.h"
#include "motor.h"
#include "vector.h"
#include "event.h"
#include "dac.h"
#include "mcu-cpu.h"
#include "timer.h"

MotorSettings motorSettings;

// used to keep total in deltas
shortTime_t  usecsPerStepX;
shortTime_t  usecsPerStepY;

typedef enum AxisHomingState {
  headingHome,
  backingUpToHome,
  homed
} AxisHomingState;

AxisHomingState homingStateX;
AxisHomingState homingStateY;
pos_t  homingDistX;
pos_t  homingDistY;
bool_t isMovingX = FALSE;
bool_t isMovingY = FALSE;
char   deltaVecCountX;
char   deltaVecCountY;
unsigned int pulseCountX;
unsigned int pulseCountY;
unsigned int deltaX[4];
signed char  deltaXSign;
char         deltaXIdx;
unsigned int deltaY[4];
signed char  deltaYSign;
char         deltaYIdx;
bool_t movingDoneX;
bool_t movingDoneY;


////////////  fixed constants  ///////////////

// distance per pulse by ustep idx 
// from 0.2 mm to 0.00625 mm
// used by homing to measure position before starting homing
#define distPerPulse(idx) (1 << (5-idx))

// table for microstep pins on DRV8825 stepper driver
char ms1PerIdx[6] = { 0, 1, 0, 1, 0, 1}; // mode 0
char ms2PerIdx[6] = { 0, 0, 1, 1, 0, 0}; // mode 1
char ms3PerIdx[6] = { 0, 0, 0, 0, 1, 1}; // mode 2


////////////////  convenience macros  /////////////

#define set_reset(axis, val) if (axis) RESET_Y_LAT = (val); else RESET_X_LAT = (val)
#define set_ms1(axis, val)   if (axis) MS1_Y_LAT   = (val); else MS1_X_LAT   = (val)
#define set_ms2(axis, val)   if (axis) MS2_Y_LAT   = (val); else MS2_X_LAT   = (val)
#define set_ms3(axis, val)   if (axis) MS3_Y_LAT   = (val); else MS3_X_LAT   = (val)
#define set_ustep(axis, ustepIdx)     \
  set_ms1(axis, ms1PerIdx[ustepIdx]); \
  set_ms2(axis, ms2PerIdx[ustepIdx]); \
  set_ms3(axis, ms3PerIdx[ustepIdx])

void set_dir(char axis, char val) {
  if(axis == 0)
    DIR_X_LAT = (motorSettings.directionLevelXY >> 1) ^ val;
  else
    DIR_Y_LAT = (motorSettings.directionLevelXY &  1) ^ val;
}


////////////////  public functions  /////////////

void initMotor() {
  // set default settings
  motorSettings.homeUsecPerPulse     = defHomeUsecPerPulse;
  motorSettings.homeUIdx             = defHomeUIdx;
  motorSettings.homeBkupUsecPerPulse = defHomeBkupUsecPerPulse;
  motorSettings.homeBkupUIdx         = defHomeBkupUIdx;
  motorSettings.directionLevelXY     = defDirectionLevelXY;
  set_dac(defMotorCurrent);
}

void motorReset(char axis, bool_t resetHigh) {
  set_reset(X, resetHigh);
  set_reset(Y, resetHigh);
}

void handleMotorCmd(char *word) {
  // word[0] is cmd code byte
  switch (word[0]) {
    case resetCmd: 
      // this also stops timer and sets motor reset pins
      newStatus(statusUnlocked); 
      return;
      
    case homeCmd:  
      // zero counter and time state, will start again below
      stopTimer(); 
      homingStateX = headingHome;
      homingStateY = headingHome;
      homingDistX = 0;
      homingDistY = 0;
      set_ustep(X, defHomeUIdx);
      set_dir(X, 0);
      CCP1_LAT = 0; // lower X step pin to start first pulse
      isMovingX = TRUE;
      set_ustep(Y, defHomeUIdx);
      set_dir(Y, 0);
      CCP2_LAT = 0; // lower Y step pin to start first pulse
      isMovingY = TRUE;
      // this also stops timer and clears motor reset pins
      newStatus(statusHoming);
      startTimer();
      setNextTimeX(defHomeUsecPerPulse);      
      setNextTimeY(defHomeUsecPerPulse);      
     return;
      
    case moveCmd:  
      if(mcu_status == statusUnlocked) {
        handleError(0, errorMoveWhenUnlocked);
        return;
      }
      if(currentVectorX == vecBufHeadX) {
        handleError(X, errorMoveWithNoVectors);
        return;
      }
      if(currentVectorY == vecBufHeadY) {
        handleError(Y, errorMoveWithNoVectors);
        return;
      }
      // this also stops timer and clears motor reset pins
      newStatus(statusMoving);   
      isMovingX = TRUE;
      isMovingY = TRUE;
      pulseCountX = 0;
      pulseCountY = 0;
      // first vec cmd can't be a delta one
      deltaVecCountX = 0;
      deltaVecCountY = 0;
      movingDoneX = FALSE;
      movingDoneY = FALSE;
      // currentVector is already set by vectors added before this cmd
      set_ustep(X, (currentVectorX->ctrlWord >> 10) & 0x0007);
      set_dir(X, (currentVectorX->ctrlWord >> 13) & 1);
      if(currentVectorX->ctrlWord & 0x03ff)
        // this is not just a delay command
        CCP1_LAT = 0; // lower X step pin to start next pulse
      set_ustep(Y, (currentVectorY->ctrlWord >> 10) & 0x0007);
      set_dir(Y, (currentVectorY->ctrlWord >> 13) & 1);
      if(currentVectorY->ctrlWord & 0x03ff)
        // this is not just a delay command
        CCP2_LAT = 0; // lower Y step pin to start next pulse
      timeX.timeShort = 2;      
      timeY.timeShort = 2;
      setNextTimeX(currentVectorX->usecsPerPulse);
      setNextTimeY(currentVectorY->usecsPerPulse);
      startTimer();  // this issues first pulse edge after 2 usecs
      return;
      
    case setHomingSpeed: 
      motorSettings.homeUIdx = word[1];
      motorSettings.homeUsecPerPulse = *((shortTime_t *) &word[2]);
      return;
      
    case setHomingBackupSpeed: 
      motorSettings.homeBkupUIdx = word[1];
      motorSettings.homeBkupUsecPerPulse = *((shortTime_t *) &word[2]);
      return;
      
    case setMotorCurrent: 
      // middle two bytes are empty
      set_dac(word[3]);
      return;
      
    case setDirectionLevelXY: 
      // d1 is X and d0 is Y
      motorSettings.directionLevelXY = word[3];
      return;
      
    case reqHomeDist:
      // next 2 words to CPU will be X and Y home distance from last homing
      nextRetWordType = 1;
      return;
      
    case clearErrorCmd:
      errorAxis = 0;
      errorCode = 0;
      newStatus(statusUnlocked);
      return;
  }
}

void chkHomingX() {
  // compare match and X pulse just happened
  if(homingStateX == headingHome) {
    // add distance for pulse that just finished
    homingDistX += distPerPulse(defHomeUIdx);
    CCP1_LAT = 0; // lower X step pin to start next pulse
    if(LIMIT_SW_X)
      // have not gotten to limit switch yet, keep heading home
      setNextTimeX(defHomeUsecPerPulse);
    else {
      // set backup dir and step size
      set_ustep(X, defHomeBkupUIdx);
      set_dir(X, 1);
      // wait a long time for reversing and for switch bouncing to stop
      setNextTimeX(debounceAndSettlingTime);
      // we reached the switch, turn around
      homingStateX = backingUpToHome;
    }
  } 
  else if(homingStateX == backingUpToHome) {
    // subtract distance for pulse that just finished
    homingDistX -= distPerPulse(defHomeBkupUIdx);
    if(!LIMIT_SW_X) {
      // have not gotten back to limit switch yet, keep heading back
      setNextTimeX(defHomeBkupUsecPerPulse);
      CCP1_LAT = 0; // lower X step pin to start next pulse
    } 
    else {
      // we are done homing X
      homingStateX == homed;
      isMovingX = FALSE;
      // if Y is done then all of homing is done
      if(homingStateY == homed){
        newStatus(statusLocked);
      }
    }
  }
}
// duplicate chkHoming code for speed, indexing into X and X variables takes time
void chkHomingY() {
  // compare match and Y pulse just happened
  if(homingStateY == headingHome) {
    // add distance for pulse that just finished
    homingDistY += distPerPulse(defHomeUIdx);
    CCP2_LAT = 0; // lower Y step pin to start next pulse
    if(LIMIT_SW_Y)
      // have not gotten to limit switch yet, keep heading home
      setNextTimeY(defHomeUsecPerPulse);
    else {
      // set backup dir and step size
      set_ustep(Y, defHomeBkupUIdx);
      set_dir(Y, 1);
      // wait a long time for reversing and for switch bouncing to stop
      setNextTimeY(debounceAndSettlingTime);
      // we reached the switch, turn around
      homingStateY = backingUpToHome;
    }
  } 
  else if(homingStateY == backingUpToHome) {
    // subtract distance for pulse that just finished
    homingDistY -= distPerPulse(defHomeBkupUIdx);
    if(!LIMIT_SW_Y) { 
      // have not gotten back to limit switch yet, keep heading back
      setNextTimeY(defHomeBkupUsecPerPulse);
      CCP2_LAT = 0; // lower Y step pin to start next pulse
    } 
    else {
      // we are done homing Y
      homingStateY == homed;
      isMovingY = FALSE;
      // if X is done then all of homing is done
      if(homingStateX == homed){
        newStatus(statusLocked); 
        stopTimer();
      }
    }
  }
}

void newDeltaX(unsigned long word) {
  char topByte = *((char *) &word);
  // see mcu-cpu.h for delta format description
  // all deltas have the same sign
  deltaXSign = ((topByte & 0x2) == 0 ? +1 : -1);
  if ((topByte & 0x01) == 0) {
    // we have a 4-delta word with 7 bits per delta
    deltaX[0] = (word & 0x0000007f);
    deltaX[1] = (word & 0x00003f80) >>  7;
    deltaX[2] = (word & 0x001fc000) >> 14;
    deltaX[3] = (word & 0x0fe00000) >> 21;
    deltaXIdx = 4;  // deltas are indexed backwards and +1
  }
  if ((word & 0x01800000) == 0x01000000) {
    // we have a 3-delta word with 9 bits per delta
    deltaX[0] = (word & 0x000001ff);
    deltaX[1] = (word & 0x0003fe00) >>  9;
    deltaX[2] = (word & 0x07fc0000) >> 18;
    deltaXIdx = 3;
  }
  if ((word & 0x01c00000) == 0x01800000) {
    // we have a 2-delta word with 13 and 14 bits per delta
    deltaX[0] = (word & 0x00003fff);
    deltaX[1] = (word & 0x07ffc000) >> 14;
    deltaXIdx = 2;
  }
}

void newDeltaY(unsigned long word) {
  char topByte = *((char *) &word);
  // see mcu-cpu.h for delta format description
  // all deltas have the same sign
  deltaYSign = ((topByte & 0x2) == 0 ? +1 : -1);
  if ((topByte & 0x01) == 0) {
    // we have a 4-delta word with 7 bits per delta
    deltaY[0] = (word & 0x0000007f);
    deltaY[1] = (word & 0x00003f80) >>  7;
    deltaY[2] = (word & 0x001fc000) >> 14;
    deltaY[3] = (word & 0x0fe00000) >> 21;
    deltaYIdx = 4;  // deltas are indexed backwards and +1
  }
  if ((word & 0x01800000) == 0x01000000) {
    // we have a 3-delta word with 9 bits per delta
    deltaY[0] = (word & 0x000001ff);
    deltaY[1] = (word & 0x0003fe00) >>  9;
    deltaY[2] = (word & 0x07fc0000) >> 18;
    deltaYIdx = 3;
  }
  if ((word & 0x01c00000) == 0x01800000) {
    // we have a 2-delta word with 13 and 14 bits per delta
    deltaY[0] = (word & 0x00003fff);
    deltaY[1] = (word & 0x07ffc000) >> 14;
    deltaYIdx = 2;
  }
}

void chkMovingX() {
  // compare match and Y pulse just happened
  if(!LIMIT_SW_X)  { 
    // unexpected closed limit switch
    handleError(X, errorLimit);
    return;
  }
  if (deltaXIdx ? (--deltaXIdx == 0) : 
                  (++pulseCountX >= (currentVectorX->ctrlWord & 0x03ff))) {
    // we are done with this vector, get a new one
    pulseCountX = 0;
    if(++currentVectorX == vecBufX + VEC_BUF_SIZE) 
         currentVectorX = vecBufX;
    if(currentVectorX == vecBufHeadX) { 
      // vector buf is empty
      handleError(X, errorVecBufUnderflow);
      return;
    }
    unsigned long word = *((unsigned long *)&currentVectorX);
    // we now have a new word
    if (currentVectorX->ctrlWord & 0xc000 == 0xc000)
      newDeltaX(word);
    
    else if(word == 0xffffffff) {
      stopTimerX();
      movingDoneX = TRUE;
      // skip past end vector marker
      // there may be vectors left over for next move command
      if(++currentVectorX == vecBufX + VEC_BUF_SIZE) 
         currentVectorX = vecBufX;      
      if(movingDoneY) {
        // done with all moving
        newStatus(statusLocked); 
        return;
      }
    }
  }
  if(deltaXIdx) {
    CCP1_LAT = 0;  // deltas always pulse
    // leave ustep and dir still set to last word
    // apply delta
    usecsPerStepX += deltaXSign * ((int) deltaX[deltaXIdx-1]);
  } else {
    if (currentVectorX->ctrlWord & 0x03ff)
      // this is not just a delay
      CCP1_LAT = 0;
    // set up absolute vector
    set_ustep(X, (currentVectorX->ctrlWord >> 10) & 0x0007);
    set_dir(X, (currentVectorX->ctrlWord >> 13) & 1);
    usecsPerStepX = currentVectorX->usecsPerPulse;
  }
  setNextTimeX(usecsPerStepX);
}

void chkMovingY() {
  // compare match and Y pulse just happened
  if(!LIMIT_SW_Y)  { 
    // unexpected closed limit switch
    handleError(Y, errorLimit);
    return;
  }
  if (deltaYIdx ? (--deltaYIdx == 0) : 
                  (++pulseCountY >= (currentVectorY->ctrlWord & 0x03ff))) {
    // we are done with this vector, get a new one
    pulseCountY = 0;
    if(++currentVectorY == vecBufY + VEC_BUF_SIZE) 
         currentVectorY = vecBufY;
    if(currentVectorY == vecBufHeadY) { 
      // vector buf is empty
      handleError(Y, errorVecBufUnderflow);
      return;
    }
    unsigned long word = *((unsigned long *)&currentVectorY);
    // we now have a new word
    if (currentVectorY->ctrlWord & 0xc000 == 0xc000)
      newDeltaY(word);
    
    else if(word == 0xffffffff) {
      stopTimerY();
      movingDoneY = TRUE;
      // skip past end vector marker
      // there may be vectors left over for next move command
      if(++currentVectorY == vecBufY + VEC_BUF_SIZE) 
         currentVectorY = vecBufY;      
      if(movingDoneY) {
        // done with all moving
        newStatus(statusLocked); 
        return;
      }
    }
  }
  if(deltaYIdx) {
    CCP2_LAT = 0;  // deltas always pulse
    // leave ustep and dir still set to last word
    // apply delta
    usecsPerStepY += deltaYSign * ((int) deltaY[deltaYIdx-1]);
  } else {
    if (currentVectorY->ctrlWord & 0x03ff)
      // this is not just a delay
      CCP2_LAT = 0;
    // set up absolute vector
    set_ustep(Y, (currentVectorY->ctrlWord >> 10) & 0x0007);
    set_dir(Y, (currentVectorY->ctrlWord >> 13) & 1);
    usecsPerStepY = currentVectorY->usecsPerPulse;
  }
  setNextTimeY(usecsPerStepY);
}

