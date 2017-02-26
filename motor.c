

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
bool_t firstVecX;
bool_t firstVecY;
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


////////////////  convenience functions  /////////////

void set_ustep(char axis, char ustepIdx) {
  if(axis == 0){
    MS1_X_LAT = ms1PerIdx[ustepIdx];
    MS2_X_LAT = ms2PerIdx[ustepIdx];
    MS3_X_LAT = ms3PerIdx[ustepIdx];
  }
  else {
    MS1_Y_LAT = ms1PerIdx[ustepIdx];
    MS2_Y_LAT = ms2PerIdx[ustepIdx];
    MS3_Y_LAT = ms3PerIdx[ustepIdx];
  }
}

void set_dir(char axis, char val) {
  if(axis == 0)
    DIR_X_LAT = (motorSettings.directionLevelXY >> 1) ^ val;
  else
    DIR_Y_LAT = (motorSettings.directionLevelXY &  1) ^ val;
}

void set_sleep() {
  // all motor inputs must be low when driver sleeping to prevent pin conflict
  RESET_X_LAT = 0;
  STEP_X_LAT  = 0;
  MS1_X_LAT   = 0;
  MS2_X_LAT   = 0;
  MS3_X_LAT   = 0;
  DIR_X_LAT   = 0;

  RESET_Y_LAT = 0;
  STEP_Y_LAT  = 0;
  MS1_Y_LAT   = 0;
  MS2_Y_LAT   = 0;
  MS3_Y_LAT   = 0;
  DIR_Y_LAT   = 0;
}

void set_resets(bool_t resetHigh) {
  RESET_X_LAT = resetHigh;
  RESET_Y_LAT = resetHigh; 
}

void initMotor() {
  // init dac
  DAC1CON1bits.DAC1R  = 0;  
  DAC1CON0bits.OE1    = 1;
  DAC1CON0bits.OE2    = 0;
  DAC1CON0bits.PSS    = 0;
  DAC1CON0bits.NSS    = 0;
  DAC1CON0bits.EN     = 1;

  // set default settings
  motorSettings.homeUIdx             = defHomeUIdx;
  motorSettings.homeUsecPerPulse     = defHomeUsecPerPulse;
  motorSettings.homeBkupUIdx         = defHomeBkupUIdx;
  motorSettings.homeBkupUsecPerPulse = defHomeBkupUsecPerPulse;
  motorSettings.directionLevelXY     = defDirectionLevelXY;
  
  motorCurrent(defMotorCurrent);
}

void homingSpeed() {
  motorSettings.homeUIdx         = spiBytes[2];
  motorSettings.homeUsecPerPulse = spiInts[1];
}

void homingBackupSpeed() {
  motorSettings.homeBkupUIdx         = spiBytes[2];
  motorSettings.homeBkupUsecPerPulse = spiInts[1];
}

void directionLevelXY(char val) {
  // d1 is X and d0 is Y
  motorSettings.directionLevelXY = val;
}

void motorCurrent(char val) {
  DAC1CON1bits.DAC1R = (val);
}


///////////////////////////////// homing ///////////////////////


void startHoming() {
  // also stops timers and clears motor reset pins
  setState(statusHoming);
  homingStateX = headingHome;
  homingStateY = headingHome;
  homingDistX = 0;
  homingDistY = 0;
  set_ustep(X, defHomeUIdx);
  set_dir(X, BACKWARDS);
  set_ustep(Y, defHomeUIdx);
  set_dir(Y, BACKWARDS);
  resetTimers();
  setNextTimeX(debounceAndSettlingTime, START_PULSE); 
  setNextTimeY(debounceAndSettlingTime, START_PULSE);
}

void chkHomingX() {
  // compare match and X pulse just happened
  if(homingStateX == headingHome) {
    // add distance for pulse that just finished
    homingDistX += distPerPulse(defHomeUIdx);
    if(LIMIT_SW_X)
      // have not gotten to limit switch yet, keep heading home
      setNextTimeX(defHomeUsecPerPulse, START_PULSE);
    else {
      // set backup dir and step size
      set_ustep(X, defHomeBkupUIdx);
      set_dir(X, FORWARD);
      // wait a long time for reversing and for switch bouncing to stop
      setNextTimeX(debounceAndSettlingTime, START_PULSE);
      // we reached the switch, turn around
      homingStateX = backingUpToHome;
    }
  }  
  else if(homingStateX == backingUpToHome) {
    // subtract distance for pulse that just finished
    homingDistX -= distPerPulse(defHomeBkupUIdx);
    if(!LIMIT_SW_X) {
      // have not gotten back to limit switch yet, keep heading back
      setNextTimeX(defHomeBkupUsecPerPulse, START_PULSE);
    } 
    else {
      // we are done homing X
      homingStateX = homed;
      // if Y is done then all of homing is done
      if(homingStateY == homed){
        setState(statusLocked);
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
    if(LIMIT_SW_Y)
      // have not gotten to limit switch yet, keep heading home
      setNextTimeY(defHomeUsecPerPulse, START_PULSE);
    else {
      // set backup dir and step size
      set_ustep(Y, defHomeBkupUIdx);
      set_dir(Y, FORWARD);
      // wait a long time for reversing and for switch bouncing to stop
      setNextTimeY(debounceAndSettlingTime, START_PULSE);
      // we reached the switch, turn around
      homingStateY = backingUpToHome;
    }
  } 
  else if(homingStateY == backingUpToHome) {
    // subtract distance for pulse that just finished
    homingDistY -= distPerPulse(defHomeBkupUIdx);
    if(!LIMIT_SW_Y) { 
      // have not gotten back to limit switch yet, keep heading back
      setNextTimeY(defHomeBkupUsecPerPulse, START_PULSE);
    } 
    else {
      // we are done homing Y
      homingStateY = homed;
      // if X is done then all of homing is done
      if(homingStateX == homed){
        setState(statusLocked); 
      }
    }
  }
}

///////////////////////////////// moving ///////////////////////

void startMoving() {
  if(mcu_state == statusUnlocked) {
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
  setState(statusMoving);   
  pulseCountX = 0;
  pulseCountY = 0;
  // first vec cmd can't be a delta one
  deltaVecCountX = 0;
  deltaVecCountY = 0;
  movingDoneX = FALSE;
  movingDoneY = FALSE;
  firstVecX = TRUE;
  firstVecY = TRUE;
  // currentVector is already set by vectors added before this cmd
  // delta vector not allowed as first vector (duh))
  set_ustep(X, (currentVectorX->ctrlWord >> 10) & 0x0007);
  set_dir(X, (currentVectorX->ctrlWord >> 13) & 1);
  set_ustep(Y, (currentVectorY->ctrlWord >> 10) & 0x0007);
  set_dir(Y, (currentVectorY->ctrlWord >> 13) & 1);
  resetTimers();
  usecsPerStepX = currentVectorX->usecsPerPulse;
  setNextTimeX(usecsPerStepX, (currentVectorX->ctrlWord & 0x03ff) == 0); 
  usecsPerStepY = currentVectorY->usecsPerPulse;
  setNextTimeY(usecsPerStepY, (currentVectorY->ctrlWord & 0x03ff) == 0);
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
  if(!firstVecX && !LIMIT_SW_X)  { 
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
    // we now have a new word
    unsigned long word = *((unsigned long *)&currentVectorX);
    if (*((char *)&word) & 0xc0 == 0xc0)
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
        setState(statusLocked); 
        return;
      }
    }
  } 
  if(deltaXIdx) {
    // leave ustep and dir pins still set to last word
    usecsPerStepX += deltaXSign * ((int) deltaX[deltaXIdx-1]);
    setNextTimeX(usecsPerStepX, START_PULSE);
  } 
  else {
    if ((currentVectorX->ctrlWord & 0x03ff) == 0) {
      // this is just a delay
      usecsPerStepX = currentVectorX->usecsPerPulse;
      setNextTimeX(usecsPerStepX, NO_PULSE);
    }
    else
      firstVecX = FALSE;
    // set up absolute vector
    set_ustep(X, (currentVectorX->ctrlWord >> 10) & 0x0007);
    set_dir(X, (currentVectorX->ctrlWord >> 13) & 1);
    usecsPerStepX = currentVectorX->usecsPerPulse;
    setNextTimeX(usecsPerStepX, START_PULSE);
  }
}

void chkMovingY() {
  // compare match and Y pulse just happened
  if(!firstVecY && !LIMIT_SW_Y)  { 
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
    // we now have a new word
    unsigned long word = *((unsigned long *)&currentVectorY);
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
        setState(statusLocked); 
        return;
      }
    }
  }
  if(deltaYIdx) {
    // leave ustep and dir pins still set to last word
    usecsPerStepY += deltaYSign * ((int) deltaY[deltaYIdx-1]);
    setNextTimeY(usecsPerStepY, START_PULSE);
  } 
  else {
    if ((currentVectorY->ctrlWord & 0x03ff) == 0) {
      // this is just a delay
      usecsPerStepY = currentVectorY->usecsPerPulse;
      setNextTimeY(usecsPerStepY, NO_PULSE);
    }
    else
      firstVecY = FALSE;
    // set up absolute vector
    set_ustep(Y, (currentVectorY->ctrlWord >> 10) & 0x0007);
    set_dir(Y, (currentVectorY->ctrlWord >> 13) & 1);
    usecsPerStepY = currentVectorY->usecsPerPulse;
    setNextTimeY(usecsPerStepY, START_PULSE);
  }
}

