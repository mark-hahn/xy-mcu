

#include <xc.h>
#include "motor.h"

////////////  tuning constants  ////////////////

// for max stepper speed calculator see ...
//   http://techref.massmind.org/techref/io/stepper/estimate.htm
// assuming 1A, 2.6mH, 12V, and 200 steps per rev; min is 433 usecs full step
#define defMinUsecsPerPulse        20 // 50 KHz

#define defHomeUsecPerPulse      1000 // 50 mm/sec  (.05 / .001)
#define defHomeUIdx                 2 // 0.05 mm/pulse
#define defHomeBkupUsecPerPulse 62500 // 0.1 mm/sec (.00625 / .0625)
#define defHomeBkupUIdx             5 // .00625 mm/pulse
#define defMotorCurrent            20 // 20 -> 1.5 amp, 26 -> 2 amp

#define debounceAndSettlingTime 50000 // debounce and time to reverse, 50 ms


////////////  fixed constants  ///////////////

// distance per pulse by ustep idx 
// from 0.2 mm to 0.00625 mm
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

void motorReset(char axis, bool_t resetHigh) {
  set_reset(X, resetHigh);
  set_reset(Y, resetHigh);
}

