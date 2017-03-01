
#ifndef MOTOR_H
#define	MOTOR_H

#include "main.h"

#define defHomeUsecPerPulse      1000 // 1000 => 50 mm/sec  (.05 / .001)
#define defHomeUIdx                 2 // 0.05 mm/pulse
#define defHomeBkupUsecPerPulse  1000 // 1000 => 6.25 mm/sec (0.00625 / 0.001)
#define defHomeBkupUIdx             5 // 5 => 0.00625 mm/pulse
#define defMotorCurrent            20 // 20 -> 1.5 amp, 26 -> 2 amp
#define defDirectionLevelXY      0b11 // x dir: d1, y dir: d0

// should these be in motorSettings?  TODO
#define debounceAndSettlingTime 50000 // debounce and time to reverse, 50 ms
#define homeDistFromLimitSwX      160 // home distance from limit switch (1 mm)
#define homeDistFromLimitSwY      160 // (1 mm)

#define MOTORS_RESET                0 // motor unlocked, no current
#define MOTORS_NOT_RESET            1 // motor locked with current

typedef struct MotorSettings {
  char homeUIdx;
  shortTime_t homeUsecPerPulse;
  char homeBkupUIdx;
  shortTime_t homeBkupUsecPerPulse;
  char directionLevelXY;  // d1 is X, d0 is Y, 1 is forward
} MotorSettings;

extern MotorSettings motorSettings;

extern pos_t homingDistX; // how long each axis traveled to get home
extern pos_t homingDistY;

void initMotor();
void set_sleep();
void set_resets(bool_t resetHigh);

void homingSpeed();
void homingBackupSpeed();
void directionLevelXY(char val);
void motorCurrent(char val);

void startHoming();
void chkHomingX();
void chkHomingY();

void startMoving();
void chkMovingX();
void chkMovingY();

#endif	/* MOTOR_H */

