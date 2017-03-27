
#ifndef MOTOR_H
#define	MOTOR_H

#include "main.h"

#define defHomeUsecPerPulse      1000 // 1000 => 50 mm/sec  (.05 / .001)
#define defHomeUIdx                 2 // 0.05 mm/pulse
#define defHomeBkupUsecPerPulse  1000 // 1000 => 6.25 mm/sec (0.00625 / 0.001)
#define defHomeBkupUIdx             5 // 5 => 0.00625 mm/pulse

#ifdef XY
#define defMotorCurrent            20 // 20 -> 1.5 amp, 26 -> 2 amp
#define defDirectionLevels      0b11 // x dir: d1, y dir: d0
#endif
#ifdef Z2
#define defMotorCurrent           84 // 0.4V, 800 ma
#define defDirectionLevels      0b00 // x dir: d1, y dir: d0
#endif

// should these be in motorSettings?  TODO
#define debounceAndSettlingTime 50000 // debounce and time to reverse, 50 ms
#define homeDistFromLimitSwX      160 // home distance from limit switch (1 mm)
#define homeDistFromLimitSwY      160 // (1 mm)

#define MOTORS_RESET                0 // motor unlocked, no current
#define MOTORS_NOT_RESET            1 // motor locked with current

typedef struct MotorSettings {
  char homeUIdx;
  uint16_t homeUsecPerPulse;
  char homeBkupUIdx;
  uint16_t homeBkupUsecPerPulse;
  char directionLevels;  // d1 is X, d0 is Y, 1 is forward
} MotorSettings;


typedef struct MoveState {
  uint8_t  dir;
  uint8_t  ustep;
  uint16_t pps;
  int8_t   acceleration;
  uint16_t pulseCount;
  int8_t   accells[10];
  uint8_t  accellsIdx;
  bool_t   done;
} MoveState;


extern MotorSettings motorSettings;

void startHoming();
void startMoving();

extern uint16_t homingDistX; // how long each axis traveled to get home
void chkHomingX();

#ifdef XY
extern uint16_t homingDistY;
void chkHomingY();
#endif

void chkMoving(char axis);

void initMotor();
void set_resets(bool_t resetHigh);

void homingSpeed();
void homingBackupSpeed();
void directionLevels(char val);
void motorCurrent(char val);

#endif	/* MOTOR_H */

