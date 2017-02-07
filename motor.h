
#ifndef MOTOR_H
#define	MOTOR_H

#include "main.h"

#define defHomeUsecPerPulse      1000 // 50 mm/sec  (.05 / .001)
#define defHomeUIdx                 2 // 0.05 mm/pulse
#define defHomeBkupUsecPerPulse 62500 // 0.1 mm/sec (.00625 / .0625)
#define defHomeBkupUIdx             5 // .00625 mm/pulse
#define defMotorCurrent            20 // 20 -> 1.5 amp, 26 -> 2 amp

#define debounceAndSettlingTime 50000 // debounce and time to reverse, 50 ms

typedef struct MotorSettings {
  char homeUIdx;
  shortTime_t homeUsecPerPulse;
  char homeBkupUIdx;
  shortTime_t homeBkupUsecPerPulse;
} MotorSettings;

extern MotorSettings motorSettings;

void initMotor();
void motorReset(char axis, bool_t resetHigh);
void handleMotorCmd(char volatile *word);
void chkHoming();

#endif	/* MOTOR_H */
