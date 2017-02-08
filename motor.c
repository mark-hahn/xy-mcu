

#include <xc.h>
#include "pins-b.h"
#include "spi.h"
#include "motor.h"
#include "event.h"
#include "dac.h"
#include "mcu-cpu.h"
#include "timer.h"

MotorSettings motorSettings;

typedef enum AxisHomingState {
  headingHome,
  backingUpToHome,
  homed
} AxisHomingState;

AxisHomingState homingStateX;
AxisHomingState homingStateY;
pos_t  homingDistX;
pos_t  homingDistY;
bool_t isPulsingX = FALSE;
bool_t isPulsingY = FALSE;


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

#define set_ms1(axis, val)   if (axis) MS1_Y_LAT   = (val); else MS1_X_LAT   = (val)
#define set_ms2(axis, val)   if (axis) MS2_Y_LAT   = (val); else MS2_X_LAT   = (val)
#define set_ms3(axis, val)   if (axis) MS3_Y_LAT   = (val); else MS3_X_LAT   = (val)
#define set_reset(axis, val) if (axis) RESET_Y_LAT = (val); else RESET_X_LAT = (val)
#define set_dir(axis, val)   if (axis) DIR_Y_LAT   = (val); else DIR_X_LAT   = (val)

#define set_ustep(axis, ustepIdx)     \
  set_ms1(axis, ms1PerIdx[ustepIdx]); \
  set_ms2(axis, ms2PerIdx[ustepIdx]); \
  set_ms3(axis, ms3PerIdx[ustepIdx])


////////////////  public functions  /////////////

void initMotor() {
  // set default settings
  motorSettings.homeUsecPerPulse     = defHomeUsecPerPulse;
  motorSettings.homeUIdx             = defHomeUIdx;
  motorSettings.homeBkupUsecPerPulse = defHomeBkupUsecPerPulse;
  motorSettings.homeBkupUIdx         = defHomeBkupUIdx;
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
      setNextTimeX(defHomeUsecPerPulse);      
      set_ustep(X, defHomeUIdx);
      set_dir(X, 0);
      CCP1_LAT = 0; // lower X step pin to start first pulse
      isPulsingX = TRUE;
      setNextTimeY(defHomeUsecPerPulse);      
      set_ustep(Y, defHomeUIdx);
      set_dir(Y, 0);
      CCP2_LAT = 0; // lower Y step pin to start first pulse
      isPulsingY = TRUE;
      // this also stops timer and clears motor reset pins
      newStatus(statusHoming);
      startTimer();
      return;
      
    case moveCmd:  
      if(mcu_status == statusUnlocked) 
        handleError(0, errorMoveWhenUnlocked);
      else
      // this also stops timer and clears motor reset pins
        newStatus(statusMoving);   
      return;
      
    case setHomingSpeedX: 
      motorSettings.homeUIdx = word[1];
      motorSettings.homeUsecPerPulse = *((shortTime_t *) &word[2]);
      return;
      
    case setHomingSpeedY: 
      motorSettings.homeBkupUIdx = word[1];
      motorSettings.homeBkupUsecPerPulse = *((shortTime_t *) &word[2]);
      return;
      
    case setMotorCurrent: 
      // middle two bytes are empty
      set_dac(word[3]);
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
      // we are done homing in X
      homingStateX == homed;
      isPulsingX = FALSE;
      // if Y is done then all of homing is done
      if(homingStateY == homed){
        newStatus(statusLocked);
        stopTimer();
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
      // we are done homing in Y
      homingStateY == homed;
      isPulsingY = FALSE;
      // if X is done then all of homing is done
      if(homingStateX == homed){
        newStatus(statusLocked);
        stopTimer();
      }
    }
  }
}

void chkMovingX() {
}

void chkMovingY() {
}
