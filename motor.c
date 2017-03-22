
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

MotorSettings motorSettings;

// used to keep total in deltas
shortTime_t  usecsPerPulseX;
#ifdef XY
shortTime_t  usecsPerStepY;
#endif
typedef enum AxisHomingState {
  headingHome,
  backingUpToHome,
  homed
} AxisHomingState;

AxisHomingState homingStateX;
pos_t  targetDistForHomeX;
pos_t  homingDistX;
char   deltaVecCountX;
uint16_t pulseCountX;
uint16_t usecsPerPulseX;
uint16_t deltaX[4];
int8_t  deltaXSign;
char     deltaXIdx;
bool_t movingDoneX;
Vector *vecX;

#ifdef XY
AxisHomingState homingStateY;
pos_t  targetDistForHomeY;
pos_t  homingDistY;
char   deltaVecCountY;
uint16_t pulseCountY;
uint16_t usecsPerPulseY;
uint16_t deltaY[4];
int8_t   deltaYSign;
char     deltaYIdx;
bool_t movingDoneY;
Vector *vecY;
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

bool_t dirIsFwdX() {
  return (DIR_X_LAT != (motorSettings.directionLevels >> 1));
}
#ifdef XY
bool_t dirIsFwdY() {
  return (DIR_Y_LAT != (motorSettings.directionLevels &  1));
}
#endif

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
  // this also stops timer and clears motor reset pins
  setState(statusMoving);   
  
  pulseCountX = 0;
  // first vec cmd can't be a delta or an eof
  deltaVecCountX = 0;
  movingDoneX = TRUE;
#ifdef XY
  pulseCountY = 0;
  deltaVecCountY = 0;
  movingDoneY = TRUE;
#endif
  if(haveVectorsX()) {
    movingDoneX = FALSE;
    vecX = getVectorX();
    // ctrlWord has five bit fields, from msb to lsb ...
    //   1 bit: axis X vector, both X and Y clr means command, not vector
    //   1 bit: axis Y vector, both X and Y set means delta, not absolute, vector
    //   1 bit: dir (0: backwards, 1: forwards)
    //   3 bits: ustep idx, 0 (full-step) to 5 (1/32 step)
    //  10 bits: pulse count
    set_dir(  X, (vecX->ctrlWord >> 13) & 1);
    set_ustep(X, (vecX->ctrlWord >> 10) & 0x0007);
    setNextTimeX(usecsPerPulseX, (vecX->ctrlWord & 0x03ff) == 0); 
  }
#ifdef XY
  if(haveVectorsY()) {
    movingDoneY = FALSE;
    vecY = getVectorY();
    set_dir(  Y, (vecY->ctrlWord >> 13) & 1);
    set_ustep(Y, (vecY->ctrlWord >> 10) & 0x0007);
    resetTimers();
    usecsPerStepY = vecY->usecsPerPulse;
    setNextTimeY(usecsPerStepY, (vecY->ctrlWord & 0x03ff) == 0);
  }
#endif
}

// 4 delta format,  7 bits each: 11s0 wwww wwwX XXXX XXyy yyyy yZZZ ZZZZ
// 3 delta format,  9 bits each: 11s1 0www wwww wwXX XXXX XXXy yyyy yyyy
// 2 delta format, 13 bits each: 11s1 10ww wwww wwww wwwX XXXX XXXX XXXX

void newDeltaX() {
  uint32_t word = *((uint32_t *) &vecX);
  // see mcu-cpu.h for delta format description
  // all deltas have the same sign
  deltaXSign = ((word & 0x02000000) == 0 ? +1 : -1);
  if ((word & 0x01000000) == 0) {
    // we have a 4-delta word with 7 bits per delta
    deltaX[0] = (word & 0x0000007f);
    deltaX[1] = (word & 0x00003f80) >>  7;
    deltaX[2] = (word & 0x001fc000) >> 14;
    deltaX[3] = (word & 0x0fe00000) >> 21;
    deltaXIdx = 4;   // deltas are indexed backwards and 1-indexed
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
#ifdef XY
void newDeltaY() {
  uint32_t word = *((uint32_t *) &vecY);
  // see mcu-cpu.h for delta format description
  // all deltas have the same sign
  deltaYSign = ((word & 0x02000000) == 0 ? +1 : -1);
  if ((word & 0x01000000) == 0) {
    // we have a 4-delta word with 7 bits per delta
    deltaY[0] = (word & 0x0000007f);
    deltaY[1] = (word & 0x00003f80) >>  7;
    deltaY[2] = (word & 0x001fc000) >> 14;
    deltaY[3] = (word & 0x0fe00000) >> 21;
    deltaYIdx = 4;  // deltas are indexed backwards and 1-indexed
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
#endif

void chkMovingX() {
  // compare match and X pulse just happened
  if(!LIMIT_SW_X)  { 
    // unexpected closed limit switch
      handleError(X, errorLimit); 
      return;
    }
  if (deltaXIdx ? (--deltaXIdx == 0) : 
                  (++pulseCountX >= (vecX->ctrlWord & 0x03ff))) {
    // we are done with this vector, get a new one
    pulseCountX = 0;
    vecX = getVectorX();
    if(errorCode) return;
    
    if ((vecX->ctrlWord & 0xc000) == 0xc000) 
      newDeltaX();
    
    else if(vecX->usecsPerPulse == 1) {
      // end of vector stream
      stopTimerX();
      movingDoneX = TRUE;
#ifdef XY
      if(movingDoneY) {
        // done with all moving
        setState(statusMoved); 
        return;
      }
#endif
#ifdef Z2
      setState(statusMoved); 
#endif
    }
  } 
  if(deltaXIdx) {
    // leave ustep and dir pins still set to last word
    usecsPerPulseX += deltaXSign * ((int) deltaX[deltaXIdx-1]);
    setNextTimeX(usecsPerPulseX, START_PULSE);
  } 
  else {
    usecsPerPulseX = vecX->usecsPerPulse;
    set_dir(X, (vecX->ctrlWord >> 13) & 1);
    set_ustep(X, (vecX->ctrlWord >> 10) & 0x0007);
    if ((vecX->ctrlWord & 0x03ff) == 0) {
      // pulseCount == 0, this is just a delay
      setNextTimeX(usecsPerPulseX, NO_PULSE);
    }
    else {
      // set up absolute vector
      setNextTimeX(usecsPerPulseX, START_PULSE);
    }
  }
}

#ifdef XY

Vector *vecY;

void chkMovingY() {
  // compare match and Y pulse just happened
  if(!LIMIT_SW_Y)  { 
    // unexpected closed limit switch
    handleError(Y, errorLimit);
    return;
  }
  if (deltaYIdx ? (--deltaYIdx == 0) : 
                  (++pulseCountY >= (vecY->ctrlWord & 0x03ff))) {
    // we are done with this vector, get a new one
    pulseCountY = 0;
    vecY = getVectorY();
    if(errorCode) return;
    
    if ((vecY->ctrlWord & 0xc000) == 0xc000) newDeltaY();
    
    else if(vecY->usecsPerPulse == 1) {
      // end of vector stream
      stopTimerY();
      movingDoneY = TRUE;
      if(movingDoneX) {
        // done with all moving
        setState(statusMoved); 
        return;
      }
    }
  }
  if(deltaYIdx) {
    // leave ustep and dir pins still set to last word
    usecsPerPulseY += deltaYSign * ((int) deltaY[deltaYIdx-1]);
    setNextTimeY(usecsPerPulseY, START_PULSE);
  } 
  else {
    usecsPerPulseY = vecY->usecsPerPulse;
    set_dir(Y, (vecY->ctrlWord >> 13) & 1);
    set_ustep(Y, (vecY->ctrlWord >> 10) & 0x0007);
    if ((vecY->ctrlWord & 0x03ff) == 0) {
      // pulseCount == 0, this is just a delay
      setNextTimeY(usecsPerPulseY, NO_PULSE);
    } else {
      // set up absolute vector
      setNextTimeY(usecsPerPulseY, START_PULSE);
    }
  }
}
#endif

