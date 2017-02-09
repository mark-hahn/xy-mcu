
#ifndef CPU_H
#define	CPU_H

#include "main.h"

// This file contains all definitions shared by CPU and MCU
// The files should be included in CPU and MCU apps
// If this file must change, make sure it is backwards compatible
// Keep the files in the CPU and MCU apps the same

// notes for CPU ...
//   for max stepper speed calculator see ...
//     http://techref.massmind.org/techref/io/stepper/estimate.htm
//   assuming 1A, 2.6mH, 12V, and 200 steps per rev; min is 433 usecs full step
  
// vector data has no separate byte for command
// either of top two bits of first byte means vector cmd
// otherwise it is one of these commands
typedef enum Cmd {
  // zero is not used so blank SPI words are ignored
  resetCmd             = 1, // clear state and hold reset on motors, unlocking them
  homeCmd              = 2, // goes home using no vectors, and saves homing distance
  moveCmd              = 3, // enough vectors need to be loaded to start
  setHomingSpeed       = 4, // set homeUIdx & homeUsecPerPulse settings
  setHomingBackupSpeed = 5, // set homeBkupUIdx & homeBkupUsecPerPulse settings
  setMotorCurrent      = 6, // set motorCurrent (0 to 31) immediately
  reqHomeDist          = 7, // return home distance, not status, next 2 words
  clearErrorCmd        = 8  // on error, no activity until this command
} Cmd;   

// general mcu states
// values are valid even when error flag is set, tells what was happening
typedef enum Status {
  statusUnlocked    = 0, // idle with no motor current
  statusLocked      = 1, // idle with motor current
  statusHoming      = 2, // automatically homing without vectors
  statusMoving      = 3  // executing vector moves from vecBuf
} Status;
extern Status mcu_status;

// immediate command 32-bit words -- top 2 bits are zero
// command codes are in top byte
// set homing speeds have microstep index in byte 2 and speed param in 3-4
// setMotorCurrent has param in bottom 5 bits
// all others have no params

// absolute vector 32-bit words -- constant speed travel
typedef struct Vector {
  // absolute ctrlWord has five bit fields, from msb to lsb ...
  //   1 bit: axis X vector, both X and Y clr means command, not vector
  //   1 bit: axis Y vector, both X and Y set means delta, not absolute, vector
  //   1 bit: dir (0: backwards, 1: forwards)
  //   3 bits: ustep idx, 0 (full-step) to 5 (1/32 step)
  //  10 bits: pulse count
  unsigned int ctrlWord;
  shortTime_t usecsPerPulse;
} Vector;

// delta 32-bit words -- varying speed travel
// this word appears after an absolute vector with multiple vectors per word
// it has same axis, dir, and micro-index as the previous vector
// delta values are the difference in usecsPerPulse
// there are 4, 3, and 2 deltas possible
// the letter s below is delta sign, 1: minus, 0: plus
// w: first delta, x: 2nd, y: 3rd, z: 4th
// 4 delta format,  7 bits each: 11s0 wwww wwwX XXXX XXyy yyyy yZZZ ZZZZ
// 3 delta format,  9 bits each: 11s1 0www wwww wwXX XXXX XXXy yyyy yyyy
// 2 delta format, 13 bits each: 11s1 10ww wwww wwww wwwX XXXX XXXX XXXX

// the last word of a vector sequence is all 1's, 0xffffffff
// when both axis have reached this marker then the move is finished

// only means anything when error flag is set
// and means nothing if error not axis-specific
extern char errorAxis;

typedef enum Error {
  none                   = 0,
  errorFault             = 1, // driver chip fault
  errorLimit             = 2, // hit error limit switch during move
  errorVecBufOverflow    = 3,
  errorVecBufUnderflow   = 4,
  errorMoveWhenUnlocked  = 5,
  errorMoveWithNoVectors = 6,
  errorSpiSync           = 7
} Error;

// this is non-zero if, and only if, error flag status bit is set
extern Error errorCode;

enum returnWordType_t {
  retypeNone      = 0x00, // whole word of zeros may happen, ignore them
  retypeStatus    = 0x10,
  retypeHomeDistX = 0x20,
  retypeHomeDistY = 0x30
} returnWordType_t;

// returnWordType_t (above) is in top nibble
// all of these flags are returned on every word to CPU
enum retFlags {
  retflagBufXHighWater = 0x08,
  retflagBufYHighWater = 0x04,
  retflagErrorAxis     = 0x02, // default zero if no axis specified
  retFlagError         = 0x01  // when error everything halts until cmd clrs it
};

// this is the status word returned when returnWordType is retypeStatus
typedef struct ReturnStatus {
  char flags;
  char status;
  char errorCode;
  char reserved;
} ReturnStatus;

void initMcuCpu();
void newStatus(char newStatus);
void handleError(char axis, Error code);

#endif	/* CPU_H */

