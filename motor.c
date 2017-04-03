
#include <stdint.h>
#include <xc.h>
#include "pins.h"
#include "spi.h"
#include "motor.h"
#include "vector.h"
#include "event.h"
#include "mcu-cpu.h"
#include "timer.h"
#include "parse-spi.h"
#include "invtable.h"

#ifdef Z2
#include "pwm-vref.h"
#endif

MotorSettings motorSettings;
MoveState moveStateX;
int32_t distanceX;
int32_t targetDistForHomeX;

#ifdef XY
MoveState moveStateY;
int32_t distanceY;
int32_t targetDistForHomeY;
#endif

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
#ifdef XY
  else {
    MS1_Y_LAT = ms1PerIdx[ustepIdx];
    MS2_Y_LAT = ms2PerIdx[ustepIdx];
    MS3_Y_LAT = ms3PerIdx[ustepIdx];
  }
#endif
}

void set_dir(char axis, char val) {
  if(axis == 0)
    DIR_X_LAT = (motorSettings.directionLevels >> 1) ^ val;
#ifdef XY
  else
    DIR_Y_LAT = (motorSettings.directionLevels &  1) ^ val;
#endif
}

void set_resets(bool_t resetHigh) {
  RESET_X_LAT = resetHigh;
#ifdef XY
  RESET_Y_LAT = resetHigh; 
#endif
}

void initMotor() {
  // init dac
#ifdef XY
  DAC1CON1bits.DAC1R  = 0;  
  DAC1CON0bits.OE1    = 1;
  DAC1CON0bits.OE2    = 0;
  DAC1CON0bits.PSS    = 0;
  DAC1CON0bits.NSS    = 0;
  DAC1CON0bits.EN     = 1;
#endif
#ifdef Z2
  initPwmVref();
#endif
  
  motorSettings.directionLevels = defDirectionLevels;
  motorSettings.debounceTime    = defDebounceTime;
 
  motorSettings.homingUstep     = defHomingUstep;
  motorSettings.homingPps       = defHomingPps;
  motorSettings.homeBkupUstep   = defHomeBkupUstep;
  motorSettings.homeBkupPps     = defHomeBkupPps
  motorSettings.homeAccel       = defHomeAccel;
  motorSettings.homeJerk        = defHomeJerk;
  motorSettings.homeDistanceX   = defHomeDistanceX;
#ifdef XY
  motorSettings.homeDistanceY   = defHomeDistanceY;
#endif
  
  motorCurrent(defMotorCurrent);

  // interrupt on either fault pin lowering
//  X_FAULT_IOC_IF = 0;                      -- ONLY FOR XY Rev B --  TODO
//  X_FAULT_IOC = 1;
#ifdef XY
//  Y_FAULT_IOC_IF = 0;
//  Y_FAULT_IOC = 1;
#endif
  spiInt = 0;
}

// === need new motorsettings commands ===  TODO
void homingSpeed() {
  motorSettings.homingUstep         = spiBytes[2];
  motorSettings.homingPps = spiInts[1];
}
void homingBackupSpeed() {
  motorSettings.homeBkupUstep         = spiBytes[2];
  motorSettings.homeBkupPps = spiInts[1];
}
void directionLevels(char val) {
  // d1 is X and d0 is Y
  motorSettings.directionLevels = val;
}

void motorCurrent(char val) {
#ifdef XY
  DAC1CON1bits.DAC1R = (val);
#endif
#ifdef Z2
  setPwmVref(val);
#endif
}
 
///////////////////////////////// homing ///////////////////////

void startHoming() {
  setState(statusHoming);
  resetTimers();
  
  for(uint8_t i = 0; i < sizeof(MoveState); i++) {
    ((uint8_t *) moveStateX)[i] = 0;
#ifdef XY
    ((uint8_t *) moveStateY)[i] = 0;
#endif
  }

  moveStateX.homingState  = headingHome;
  moveStateX.ustep        = motorSettings.homingUstep;
  moveStateX.dir          = BACKWARDS;
  moveStateX.pulseCount   = 0xffff;
  moveStateX.pps          = motorSettings.homeJerk;
  moveStateX.acceleration = motorSettings.homeAccel;
  chkMovingX();

#ifdef XY
  moveStateY.homingState  = headingHome;
  moveStateY.ustep        = motorSettings.homingUstep;
  moveStateY.dir          = BACKWARDS;
  moveStateY.pulseCount   = 0xffff;
  moveStateY.pps          = motorSettings.homeJerk;
  moveStateY.acceleration = motorSettings.homeAccel;
  chkMovingY();
#endif
}

///////////////////////////////// moving ///////////////////////

void startMoving() {
  setState(statusMoving); 
  
  for(uint8_t i = 0; i < sizeof(MoveState); i++) {
    ((uint8_t *) moveStateX)[i] = 0;
#ifdef XY
    ((uint8_t *) moveStateY)[i] = 0;
#endif
  }
  chkMovingX();
  
#ifdef XY
  chkMovingY();
#endif
}

void chkMovingX(bool_t fromInterrupt) {
  uint32_t *vec;
  int8_t accel = 0;
  bool_t haveMove = FALSE;
  bool_t homingDone = FALSE;
  
  if(fromInterrupt) {
    distanceX += dist   TODO
  }
  
  switch(moveStateX.homingState) {
    case notHoming: 
      if(!LIMIT_SW_X) {
        // unexpected closed limit switch
        handleError(X, errorLimit); 
        return;
      }      
      break;
    case headingHome:
      if(!LIMIT_SW_X) {
        moveStateX.homingState  = deceleratingPastSw;
        moveStateX.acceleration = -motorSettings.homeAccel;
      }
      break;
    case deceleratingPastSw:
      if(moveStateX.pps <= motorSettings.homeJerk) {
        moveStateX.homingState  = backingUpToSw;
        moveStateX.dir          = FORWARD;
        moveStateX.ustep        = motorSettings.homeBkupUstep;;
        moveStateX.pps          = motorSettings.homeBkupPps
        moveStateX.acceleration = 0;
      }   
      break;
    case backingUpToSw:
      if(LIMIT_SW_X) {
        moveStateX.homingState = backingUpToHome;
        targetDistForHomeX = distanceX + motorSettings.homeDistanceX;
      }
      break;
    case backingUpToHome:
      if(distanceX >= targetDistForHomeX) {
        homingDone = TRUE;
        moveStateX.homingState = notHoming;
        moveStateX.pulseCount = 0;
      }
  }
  
doOneVecX:
  if(moveStateX.delayUsecs)  {
    setNextTimeX(moveStateX.delayUsecs, NO_PULSE);
    moveStateX.delayUsecs = 0;
    return;
  }
  else if(moveStateX.accellsIdx) {
    accel = moveStateX.accells[--moveStateX.accellsIdx];
    haveMove = TRUE;
  }
  else if(moveStateX.pulseCount) {
    accel = moveStateX.acceleration;
    moveStateX.pulseCount--;
    haveMove = TRUE;
  }
  else {
    if(homingDone || (vec = getVectorX())) {
      // if marker, it is returned from parseVector
      if(homingDone || parseVector(vec, &moveStateX)) {
        // only marker is EOF for now
        stopTimerX();
#ifdef XY
        moveStateX.done = TRUE;
        // done with all moving?
        if(moveStateY.done) setState(statusMoved); 
#endif
#ifdef Z2
        setState(statusMoved); 
#endif
        return;
      }
      else
        goto doOneVecX;
    }
    handleError(X, errorVecBufUnderflow);
    return;
  }
  if(accel && !(moveStateX.homingState && 
                moveStateX.pps >= (moveStateX.homingState == headingHome ? 
                                            defHomingPps : defHomeBkupPps))) {
    if(!moveStateX.usecsPerPulse)
        moveStateX.usecsPerPulse = pps2usecs(moveStateX.pps);
    moveStateX.pps += (int16_t)
      ((((int16_t) 5 * accel) * (short long) (moveStateX.usecsPerPulse >> 1)) 
                                                  >> (14 - moveStateX.ustep));
  }
  if(haveMove) {
    set_ustep(X, moveStateX.ustep);
    set_dir(X, moveStateX.dir);
    setNextPpsX(moveStateX.pps, START_PULSE);
  }
}

#ifdef XY
void chkMovingY() {
  uint32_t *vec;
  int8_t accel = 0;
  bool_t haveMove = FALSE;

  if(!LIMIT_SW_Y)  { 
    // unexpected closed limit switch
    handleError(Y, errorLimit); 
    return;
  }
  
doOneVecY:
  if(moveStateY.delayUsecs)  {
    setNextTimeY(moveStateY.delayUsecs, NO_PULSE);
    moveStateY.delayUsecs = 0;
    return;
  }
  else if(moveStateY.accellsIdx) {
    accel = moveStateY.accells[--moveStateY.accellsIdx];
    haveMove = TRUE;
  }
  else if(moveStateY.pulseCount) {
    accel = moveStateY.acceleration;
    moveStateY.pulseCount--;
    haveMove = TRUE;
  }
  else {
    if(vec = getVectorY()) {
      // if marker, it is returned from parseVector
      if(parseVector(vec, &moveStateY)) {
        // only marker is EOF for now
        stopTimerY();
        moveStateY.done = TRUE;
        // done with all moving?
        if(moveStateX.done) setState(statusMoved); 
        return;
      }
      else
        goto doOneVecY;
    }
    handleError(Y, errorVecBufUnderflow);
    return;
  }
  if(accel) {
    if(!moveStateY.usecsPerPulse)
        moveStateY.usecsPerPulse = pps2usecs(moveStateY.pps);
    moveStateY.pps += (int16_t)
      ((((int16_t) 5 * accel) * (short long) (moveStateY.usecsPerPulse >> 1)) 
                                                  >> (14 - moveStateY.ustep));
  }
  if(haveMove) {
    set_ustep(Y, moveStateY.ustep);
    set_dir(Y, moveStateY.dir);
    setNextPpsY(moveStateY.pps, START_PULSE);
  }
}
#endif
