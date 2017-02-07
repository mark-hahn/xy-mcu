
// cpu.c handles interaction with CPU

#ifndef CPU_H
#define	CPU_H

typedef enum Cmd {
  resetCmd,
  homeCmd, 
  moveCmd, 
  settingsCmd,
  // waypoint data sent has no separate byte for command
  // top bit of first byte means waypoints cmd
  waypointsCmd = 0x80 
} Cmd;   

// d0 to d2 in status byte returned
// max of 8 codes
typedef enum Status {
  // idle
  statusUnlocked    = 0,
  statusLocked      = 1,
  // active
  statusStartHoming = 2,
  statusHoming      = 3,
  statusStartMoving = 4,
  statusMoving      = 5
} Status;
extern Status status;

// d3 in status byte returned
extern char errorAxis;

// d4 to d7 in status byte returned
// max of 16 codes
typedef enum Error {
  none               = 0,
  errorFault         = 1,
  errorLimit         = 2,
  errorVecBufOverflow   = 3,
  errorI2cWCOL       = 4,
  errorI2cSSOPV      = 5,
  errorSettingsLen   = 6,
  errorNowGtWp       = 7,
  errorMoveTooSlow   = 8,
  errorMoveNoWp      = 9,
  errorWrongFinalPos = 10
} Error;

void handleError(char axis, Error code);


#endif	/* CPU_H */

