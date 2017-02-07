

#include <xc.h>
#include "pins-b.h"
#include "motor.h"
#include "dac.h"
#include "cpu.h"

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
#define set_step(axis, val)  if (axis) STEP_Y_LAT  = (val); else STEP_X_LAT  = (val)
#define set_dir(axis, val)   if (axis) DIR_Y_LAT   = (val); else DIR_X_LAT   = (val)

#define limit_switch_open(axis) (axis ? LIMIT_SW_Y : LIMIT_SW_X)

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

void handleMotorCmd(char volatile *word) {
  // word[0] is cmd code byte
  switch (word[0]) {
    case resetCmd: 
      // this also stops timer and sets motor reset pins
      newStatus(statusUnlocked); 
      return;
      
    case homeCmd:  
      homingStateX = headingHome;
      homingStateY = headingHome;
      homingDistX = 0;
      homingDistY = 0;
      // this also stops timer and clears motor reset pins
      newStatus(statusHoming);
      return;
      
    case moveCmd:  
      if(status == statusUnlocked) 
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
  }
}

void chkHoming() {
  
  // handle X and Y separately,   TODO   !!!
  
  // spiWordInByteIdx just used as flag
//  if(!isTimerRunning() || spiWordInByteIdx > 0) {
//    stopTimer();  
//    spiWordInByteIdx = 0; 
//    if(homingStateX.headingHome) {
//      ccpXLowByte  = motorSettings.homeUsecPerPulse & 0xff;
//      ccpXHighByte = motorSettings.homeUsecPerPulse >> 8;
//      startTimer(); 
//    } 
//    else if(homingStateX.backingUpToHome) {
//      ccpXLowByte  = motorSettings.homeUsecPerPulse & 0xff;
//      ccpXHighByte = motorSettings.homeUsecPerPulse >> 8;
//      startTimer(); 
//    }
    // leave timer stopped is both done homing
//  }
}