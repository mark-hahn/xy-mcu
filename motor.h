
#ifndef MOTOR_H
#define	MOTOR_H

#include "main.h"

// in motorsettings
#define defHomingPps      1000 // 1000 => 50 mm/sec  (.05 / .001)
#define defHomingUstep    4    // 0.05 mm/pulse
#define defHomeBkupPps    1000 // 1000 => 6.25 mm/sec (0.00625 / 0.001)
#define defHomeBkupUstep  5    // 5 => 0.00625 mm/pulse
#define defHomeAccel      32   // 1000 mm/sec/sec
#define defHomeJerk       50   // speed considered zero
#define defDebounceTime  50000   // debounce and time to reverse, 50 ms

#ifdef XY
#define defMotorCurrent      20 // 20 -> 1.5 amp, 26 -> 2 amp
#define defDirectionLevels 0b11 // x dir: d1, y dir: d0
#define defHomeDistanceX    800   // (5 mm)
#define defHomeDistanceY    160  // (1 mm)
#endif

#ifdef Z2
#define defMotorCurrent      84 // 0.4V, 800 ma
#define defDirectionLevels 0b00 // x dir: d1
#define defHomeDistanceX    800 // (5 mm)
#endif

#define MOTORS_RESET                0 // motor unlocked, no current
#define MOTORS_NOT_RESET            1 // motor locked with current

typedef struct MotorSettings {
  uint8_t  current;
  uint8_t  directionLevels;  // d1 is X, d0 is Y, 1 is forward
  uint16_t debounceTime;     
  
  uint8_t  homingUstep;
  uint16_t homingPps;
  uint8_t  homeBkupUstep;
  uint16_t homeBkupPps;
  int8_t   homeAccel;
  uint8_t  homeJerk;
  uint8_t  homeDistanceX;
  uint8_t  homeDistanceY;
} MotorSettings;

typedef enum HomingState {
  notHoming = 0,
  headingHome,
  deceleratingPastSw,
  backingUpToSw,
  backingUpToHome
} HomingState;

typedef struct MoveState {
  uint8_t     dir;
  uint8_t     ustep;
  uint16_t    pps;
  uint16_t    usecsPerPulse;
  int8_t      acceleration;
  uint16_t    pulseCount;
  int8_t      accells[10];
  uint8_t     accellsIdx;
  uint16_t    delayUsecs;
  HomingState homingState;
  bool_t      done;
} MoveState;

extern MotorSettings motorSettings;
extern int32_t distanceX;
extern int32_t distanceY;

void startHoming();
void startMoving();

extern int32_t homingDistX; // how long each axis traveled to get home
void chkHomingX();
void chkMovingX();

#ifdef XY
extern int32_t homingDistY;
void chkHomingY();
void chkMovingY();
#endif

void initMotor();
void set_resets(bool_t resetHigh);

void homingSpeed();
void homingBackupSpeed();
void directionLevels(char val);
void motorCurrent(char val);

#endif	/* MOTOR_H */


/////////////////////////////////  Notes  ///////////////////////////

// CPU ...
//   for max stepper speed calculator see ...
//     http://techref.massmind.org/techref/io/stepper/estimate.htm
//   assuming 1A, 2.6mH, 12V, and 200 steps per rev; min is 433 usecs full step

// MOTOR ...
// distance per step
// 0: 0.2 mm
// 1: 0.1 mm
// 2: 0.05 mm
// 3: 0.025 mm
// 4: 0.0125 mm
// 5: 0.00625 mm

// DAC ...
// vref = (val * 3.3 / 32 - 0.65)/2; amps = 2 * V
// The following are motor current VREFs measured by dac settings
//  4 -> 0       s.b. -0.119
//  5 -> 0.009   s.b. -0.067
//  6 -> 0.041   s.b. -0.035
//  7 -> 0.083   s.b.  0.036
//  8 -> 0.128   s.b.  0.088
//  9 -> 0.175   s.b.  0.139
// 10 -> 0.222   s.b.  0.191
// 20 -> 0.710   s.b.  0.706
// 31 -> 1.275   s.b.  1.273


// MAX SPEED  X-AXIS ONLY
// ustep 0:   1200                                    240 mm/sec (no accel)
//            3000 with accell of 50 pps/sec
//            3800 with accell of 25 pps/sec 
//            4000 with accell of 20 pps/sec          800 mm/s !!!
// ustep 1:   2400                                    240 mm/sec  (no accel)
//            2700 with accell of 10 pps/sec
//            4000 with accell of 25 pps/sec          400 mm/s
// ustep 2-5: 4000 (limited by MCU counter delay)

// MAX SPEED  BOTH AXES
// ustep 1, accell 10: 1300  --  130 mm/s
// ustep 1, accell 20: 2000
// ustep 1, accell 30: 1000

