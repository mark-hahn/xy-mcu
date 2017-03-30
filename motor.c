
#include <stdint.h>
#include <xc.h>
#include "pins.h"
#include "spi.h"
#include "motor.h"
#include "vector.h"
#include "event.h"
#ifdef Z2
#include "pwm-vref.h"
#endif
#include "mcu-cpu.h"
#include "timer.h"
#include "parse-spi.h"

MotorSettings motorSettings;

typedef enum AxisHomingState {
  headingHome,
  backingUpToHome,
  homed
} AxisHomingState;

AxisHomingState homingStateX;
uint16_t        targetDistForHomeX;
uint16_t        homingDistX;
MoveState       moveStateX;

#ifdef XY
AxisHomingState homingStateY;
uint16_t        targetDistForHomeY;
uint16_t        homingDistY;
MoveState       moveStateY;
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
  // set default settings
  motorSettings.homeUIdx             = defHomeUIdx;
  motorSettings.homeUsecPerPulse     = defHomeUsecPerPulse;
  motorSettings.homeBkupUIdx         = defHomeBkupUIdx;
  motorSettings.homeBkupUsecPerPulse = defHomeBkupUsecPerPulse;
  motorSettings.directionLevels      = defDirectionLevels;
  
  motorCurrent(defMotorCurrent);
  
  // interrupt on either fault pin lowering
  X_FAULT_IOC_IF = 0;
  X_FAULT_IOC = 1;
#ifdef XY
  Y_FAULT_IOC_IF = 0;
  Y_FAULT_IOC = 1;
#endif
  spiInt = 0;
}

void homingSpeed() {
  motorSettings.homeUIdx         = spiBytes[2];
  motorSettings.homeUsecPerPulse = spiInts[1];
}

void homingBackupSpeed() {
  motorSettings.homeBkupUIdx         = spiBytes[2];
  motorSettings.homeBkupUsecPerPulse = spiInts[1];
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
  // also stops timers and clears motor reset pins
  homingDistX = 0;
  homingStateX = headingHome;
#ifdef XY
  if((spiBytes[0] & 0b10) == 0) homingStateX = homed;
  if((spiBytes[0] & 0b11) == 0) {
    setState(statusLocked);
    return;
  }
#endif
  setState(statusHoming);
  targetDistForHomeX = 0;
  set_ustep(X, defHomeUIdx);
  set_dir(X, BACKWARDS);
  resetTimers();
  if(homingStateX == headingHome)
    setNextTimeX(debounceAndSettlingTime, START_PULSE);
#ifdef XY
  homingDistY = 0;
  if((spiBytes[0] & 0b01) == 0) homingStateY = homed;
  else homingStateY = headingHome;
  targetDistForHomeY = 0;
  set_ustep(Y, defHomeUIdx);
  set_dir(Y, BACKWARDS);
  if( homingStateY == headingHome)
    setNextTimeY(debounceAndSettlingTime, START_PULSE);
#endif
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
    else if(targetDistForHomeX == 0) {
     // have gotten back to limit switch, set further distance to home
      targetDistForHomeX = homingDistX - homeDistFromLimitSwX;
      setNextTimeX(defHomeBkupUsecPerPulse, START_PULSE);
    }
    else if (homingDistX > targetDistForHomeX) {
      setNextTimeX(defHomeBkupUsecPerPulse, START_PULSE);
    }
    else {
      // we are done homing X
      homingStateX = homed;
#ifdef XY
      // if Y is done then all of homing is done
      if(homingStateY == homed){
        setState(statusLocked);
      }
#endif
#ifdef Z2
      setState(statusLocked);
#endif
    }
  }
}
#ifdef XY
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
    else if(targetDistForHomeY == 0) {
     // have gotten back to limit switch, set further distance to home
      targetDistForHomeY = homingDistY - homeDistFromLimitSwY;
      setNextTimeY(defHomeBkupUsecPerPulse, START_PULSE);
    }
    else if (homingDistY > targetDistForHomeY) {
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
#endif

///////////////////////////////// moving ///////////////////////

void startMoving() {
  uint32_t *vec;
  // this also stops timer and clears motor reset pins
  setState(statusMoving); 
  
  if(vec = getVectorX()) {
    // first vector is always a velocity vector
    if(moveStateX.pulseCount == 0)
      // this is just a delay with pps in usecs
      
      setNextTimeX((((uint16_t) moveStateX.dir)          << 15) |
                   (((uint16_t) moveStateX.ustep & 0x07) << 12) |
                                moveStateX.pps, FALSE);
    else {
      set_dir(  X, moveStateX.dir);
      set_ustep(X, moveStateX.ustep);
      setNextPpsX(moveStateX.pps, TRUE);
    }
  }
  
#ifdef XY
  if(vec = getVectorY()) {
    // first vector is always a velocity vector
    if(moveStateY.pulseCount == 0)
      // this is just a delay with pps in usecs
      setNextTimeY((((uint16_t) moveStateY.dir)          << 15) |
                   (((uint16_t) moveStateY.ustep & 0x07) << 12) |
                                moveStateY.pps, FALSE);
    else {
      set_dir(  Y, moveStateY.dir);
      set_ustep(Y, moveStateY.ustep);
      setNextPpsY(moveStateY.pps, TRUE);
    }
  }
#endif
}

void chkMoving(char axis) {
  uint32_t *vec;
  int8_t accel = 0;
  bool_t haveMove = FALSE;
  
#ifdef XY
  MoveState *moveState = (axis? &moveStateY : &moveStateX);
  
  if(!axis && !LIMIT_SW_X || axis && !LIMIT_SW_Y)  { 
    // unexpected closed limit switch
      handleError(axis, errorLimit); 
      return;
  }
#endif
#ifdef Z2
  MoveState *moveState = &moveStateX;
  
  if(!LIMIT_SW_X)  { 
    // unexpected closed limit switch
      handleError(X, errorLimit); 
      return;
  }
#endif
  
  if(moveState->accellsIdx) {
    accel = moveState->accells[--moveState->accellsIdx];
    haveMove = TRUE;
  }
  else if(moveState->pulseCount) {
    accel = moveState->acceleration;
    moveState->pulseCount--;
    haveMove = TRUE;
  }
  if(accel) moveState->pps += accel;
#ifdef XY
  if(!axis && haveMove) setNextPpsX(moveState->pps, START_PULSE);
  if( axis && haveMove) setNextPpsY(moveState->pps, START_PULSE);
#endif
#ifdef Z2
  if(haveMove) setNextPpsX(moveState->pps, START_PULSE);
#endif
  
  if(moveState->accellsIdx == 0 && moveState->pulseCount == 0) {

#ifdef XY
    if(!axis && (vec = getVectorX())) {
      if(parseVector(vec, &moveStateX)) {
        // only marker is EOF for now
        stopTimerX();
        moveStateX.done = TRUE;
        // done with all moving?
        if(moveStateY.done) setState(statusMoved); 
      }
    }
    else if(axis && (vec = getVectorY())) {
      if(parseVector(vec, &moveStateY)) {
        // only marker is EOF for now
        stopTimerY();
        moveStateY.done = TRUE;
        // done with all moving?
        if(moveStateX.done) setState(statusMoved); 
      }
    }
#endif
#ifdef Z2
    if(vec = getVectorX()) {
      if(parseVector(vec, &moveStateX)) {
        // only marker is EOF for now
        stopTimerX();
        setState(statusMoved); 
      }
    }
#endif
  }
}
