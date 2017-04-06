
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

int16_t settings[NUM_SETTINGS];
        
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

void set_dir(char axis, char val) {
  if(axis == 0)
    DIR_X_LAT = (settings[directionLevels]   >> 1) ^ val;
#ifdef XY
  else
    DIR_Y_LAT = (settings[directionLevels] & 0x01) ^ val;
#endif
}

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
  
void setMotorCurrent(int16_t val) {
#ifdef XY
  DAC1CON1bits.DAC1R = (uint8_t) val;
#endif
#ifdef Z2
  setPwmVref(val);
#endif
}

void set_resets(bool_t resetHigh) {
  RESET_X_LAT = resetHigh;
#ifdef XY
  RESET_Y_LAT = resetHigh;
#endif
}

void initMotor() {
  settings[debounceTime]    = 30000; // debounce and time to reverse, 30 ms
  settings[homingUstep]     = 3;     // 0.05 mm/pulse
  settings[homingPps]       = 2000;  // 1000 => 50 mm/sec  (.05 / .001)
  settings[homeBkupUstep]   = 5;     // 5 => 0.00625 mm/pulse
  settings[homeBkupPps]     = 1000;  // 1000 => 6.25 mm/sec (0.00625 / 0.001)
  settings[homeAccel]       = 8;    // 1000 mm/sec/sec
  settings[homeJerk]        = 10;    // speed considered zero
  settings[homeOfsX]        = 5000;   // (5 mm)
  settings[directionLevels] = 0b11;  // x dir: d1, y dir: d0
  #ifdef XY
  settings[motorCurrent]    = 20;    // 16: 1A, 20: 1.5A, 26: 2A
  settings[homeOfsY]        = 5000;   // (5 mm)
  #endif 
  #ifdef Z2
  settings[motorCurrent]    = 84;    // 0.4V, 800 ma
  settings[directionLevels] = 0b00;  // x dir: d1
  settings[homeOfsY]        = 800;   // (5 mm)
  #endif

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

  // interrupt on either fault pin lowering
//  X_FAULT_IOC_IF = 0;                      -- ONLY FOR XY Rev B --  TODO
//  X_FAULT_IOC = 1;
#ifdef XY
//  Y_FAULT_IOC_IF = 0;
//  Y_FAULT_IOC = 1;
#endif

  setMotorCurrent(settings[motorCurrent]);
  spiInt = 0;
}

uint16_t maxPps;

///////////////////////////////// homing ///////////////////////

void startHoming() {
  setState(statusHoming);
  resetTimers();
  
  for(uint8_t i = 0; i < sizeof(MoveState); i++) {
    ((uint8_t *) &moveStateX)[i] = 0;
#ifdef XY
    ((uint8_t *) &moveStateY)[i] = 0;
#endif
  }
  
  moveStateX.homingState  = headingHome;
  moveStateX.ustep        = settings[homingUstep];
  moveStateX.dir          = BACKWARDS;
  moveStateX.pulseCount   = 0xffff;
  moveStateX.targetPps    = settings[homingPps];
  moveStateX.acceleration = settings[homeAccel];
  chkMovingX();

#ifdef XY
  moveStateY.homingState  = headingHome;
  moveStateY.ustep        = settings[homingUstep];
  moveStateY.dir          = BACKWARDS;
  moveStateY.pulseCount   = 0xffff;
  moveStateY.targetPps    = settings[homingPps];
  moveStateY.acceleration = settings[homeAccel];
  chkMovingY();
#endif
}

///////////////////////////////// moving ///////////////////////

void startMoving() {
  setState(statusMoving); 
  
  for(uint8_t i = 0; i < sizeof(MoveState); i++) {
    ((uint8_t *) &moveStateX)[i] = 0;
#ifdef XY
    ((uint8_t *) &moveStateY)[i] = 0;
#endif
  }
  chkMovingX();
  
#ifdef XY
  chkMovingY();
#endif
}

void chkMovingX() {
  uint32_t *vec;
  uint8_t accel = 0;
  bool_t haveMove = FALSE;
  bool_t homingDone = FALSE;
  
  if(moveStateX.pulsed) {
    distanceX += ((moveStateX.dir == FORWARD) ? 
                   distPerPulse(moveStateX.ustep) :
                  -distPerPulse(moveStateX.ustep) );
    moveStateX.pulsed = FALSE;
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
        moveStateX.accelSign    = 1;
        moveStateX.targetPps    = settings[homeJerk];
      }
      break;
    case deceleratingPastSw:
      if(moveStateX.currentPps <= settings[homeJerk]) {
        moveStateX.homingState  = backingUpToSw;
        moveStateX.dir          = FORWARD;
        moveStateX.ustep        = settings[homeBkupUstep];
        moveStateX.currentPps = 
        moveStateX.targetPps    = settings[homeBkupPps];
        moveStateX.acceleration = 0;
      }   
      break;
    case backingUpToSw:
      if(LIMIT_SW_X) {
        moveStateX.homingState = backingUpToHome;
        targetDistForHomeX = distanceX + settings[homeOfsX];
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
    /////////////// TODO
  }
  else if(moveStateX.pulseCount) {
    accel = moveStateX.acceleration;
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
  if(accel) {
    if(!moveStateX.usecsPerPulse)
        moveStateX.usecsPerPulse = pps2usecs(moveStateX.currentPps);

    uint16_t a = (uint16_t) 4 * accel;                    // 10 bits
    unsigned short long b = 
        (unsigned short long) moveStateX.usecsPerPulse;   // 14 bits
    unsigned short long c = a*b;                          // 24 bits
    unsigned short long d = c >> (15 - moveStateX.ustep); // -10 bits
    uint16_t ppsChange = (uint16_t) d;                    // 14 bits
    if(moveStateX.accelSign) {
      if(ppsChange > moveStateX.currentPps ||
          (moveStateX.currentPps - ppsChange) <= moveStateX.targetPps)
        moveStateX.currentPps = moveStateX.targetPps;
      else
        moveStateX.currentPps -= ppsChange;
    }
    else {
      if((moveStateX.currentPps + ppsChange) >= moveStateX.targetPps)
        moveStateX.currentPps = moveStateX.targetPps;
      else
        moveStateX.currentPps += ppsChange;
    }
  }
  if(haveMove) {
    set_ustep(X, moveStateX.ustep);
    set_dir(X, moveStateX.dir);
    moveStateX.pulsed = TRUE;
    moveStateX.pulseCount--;
    setNextPpsX(moveStateX.currentPps, START_PULSE);
  }
}
#ifdef XY

void chkMovingY() {
  uint32_t *vec;
  uint8_t accel = 0;
  bool_t haveMove = FALSE;
  bool_t homingDone = FALSE;
  
  if(moveStateY.pulsed) {
    distanceY += ((moveStateY.dir == FORWARD) ? 
                   distPerPulse(moveStateY.ustep) :
                  -distPerPulse(moveStateY.ustep) );
    moveStateY.pulsed = FALSE;
  }
  switch(moveStateY.homingState) {
    case notHoming: 
      if(!LIMIT_SW_Y) {
        // unexpected closed limit switch
        handleError(Y, errorLimit); 
        return;
      }      
      break;
    case headingHome:
      if(!LIMIT_SW_Y) {
        moveStateY.homingState  = deceleratingPastSw;
        moveStateY.accelSign    = 1;
        moveStateY.targetPps    = settings[homeJerk];
      }
      break;
    case deceleratingPastSw:
      if(moveStateY.currentPps <= settings[homeJerk]) {
        moveStateY.homingState  = backingUpToSw;
        moveStateY.dir          = FORWARD;
        moveStateY.ustep        = settings[homeBkupUstep];
        moveStateY.currentPps = 
        moveStateY.targetPps    = settings[homeBkupPps];
        moveStateY.acceleration = 0;
      }   
      break;
    case backingUpToSw:
      if(LIMIT_SW_Y) {
        moveStateY.homingState = backingUpToHome;
        targetDistForHomeY = distanceY + settings[homeOfsY];
      }
      break;
    case backingUpToHome:
      if(distanceY >= targetDistForHomeY) {
        homingDone = TRUE;
        moveStateY.homingState = notHoming;
        moveStateY.pulseCount = 0;
      }
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
    /////////////// TODO
  }
  else if(moveStateY.pulseCount) {
    accel = moveStateY.acceleration;
    haveMove = TRUE;
  }
  else {
    if(homingDone || (vec = getVectorY())) {
      // if marker, it is returned from parseVector
      if(homingDone || parseVector(vec, &moveStateY)) {
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
        moveStateY.usecsPerPulse = pps2usecs(moveStateY.currentPps);

    uint16_t a = (uint16_t) 4 * accel;                    // 10 bits
    unsigned short long b = 
        (unsigned short long) moveStateY.usecsPerPulse;   // 14 bits
    unsigned short long c = a*b;                          // 24 bits
    unsigned short long d = c >> (15 - moveStateY.ustep); // -10 bits
    uint16_t ppsChange = (uint16_t) d;                    // 14 bits
    if(moveStateY.accelSign) {
      if(ppsChange > moveStateY.currentPps ||
          (moveStateY.currentPps - ppsChange) <= moveStateY.targetPps)
        moveStateY.currentPps = moveStateY.targetPps;
      else
        moveStateY.currentPps -= ppsChange;
    }
    else {
      if((moveStateY.currentPps + ppsChange) >= moveStateY.targetPps)
        moveStateY.currentPps = moveStateY.targetPps;
      else
        moveStateY.currentPps += ppsChange;
    }
  }
  if(haveMove) {
    set_ustep(Y, moveStateY.ustep);
    set_dir(Y, moveStateY.dir);
    moveStateY.pulsed = TRUE;
    moveStateY.pulseCount--;
    setNextPpsY(moveStateY.currentPps, START_PULSE);
  }
}
#endif
