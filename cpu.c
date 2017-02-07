
#include <xc.h>
#include "cpu.h"
#include "motor.h"
#include "main.h"

Status status;
Error errorCode;
char errorAxis;

// this also clears all state except actual position
void newStatus(char newStatus) {
  bool_t resetHigh = (newStatus != statusUnlocked);
  motorReset(X, resetHigh);
  motorReset(Y, resetHigh);
  status = newStatus; 
  errorAxis = errorCode = 0;
}

// axis is zero (X) when not specific to axis
void handleError(char axis, Error code) {
  newStatus(statusUnlocked);
  errorAxis = axis;
  errorCode = code;
}

