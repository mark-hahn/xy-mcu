
// cpu.c handles interaction with CPU

#ifndef CPU_H
#define	CPU_H

// notes for CPU ...
//   for max stepper speed calculator see ...
//     http://techref.massmind.org/techref/io/stepper/estimate.htm
//   assuming 1A, 2.6mH, 12V, and 200 steps per rev; min is 433 usecs full step
  
// vector data has no separate byte for command
// either of top two bits of first byte means vector cmd
// otherwise it is one of these commands
typedef enum Cmd {
  // zero is not used so blank SPI words are ignored
  resetCmd        = 1, // clear state and hold reset on motors, unlocking them
  homeCmd         = 2, // homes with no vectors, and saves homing distance
  moveCmd         = 3, // enough vectors need to be loaded to start
  setHomingSpeedX = 4, // set homeUIdx & homeUsecPerPulse settings
  setHomingSpeedY = 5, // set homeBkupUIdx & homeBkupUsecPerPulse settings
  setMotorCurrent = 6  // set motorCurrent (0 to 31) immediately
} Cmd;   

// d0 to d2 in status byte
// max of 8 codes
typedef enum Status {
  statusUnlocked    = 0, // idle with no motor current
  statusLocked      = 1, // idle with motor current
  statusHoming      = 2, // automatically homing without vectors
  statusMoving      = 3  // executing vector moves from vecBuf
} Status;
extern Status status;

// d3 in status byte returned
extern char errorAxis;

// d4 to d7 in status byte
// max of 16 codes
typedef enum Error {
  none                  = 0,
  errorFault            = 1,
  errorLimit            = 2,
  errorVecBufOverflow   = 3,
  errorMoveWhenUnlocked = 4
} Error;

void newStatus(char newStatus);
void handleError(char axis, Error code);

#endif	/* CPU_H */

