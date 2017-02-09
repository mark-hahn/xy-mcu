
#include <xc.h>
#include "mcu-cpu.h"
#include "main.h"
#include "timer.h"
#include "motor.h"

Status mcu_status;
Error errorCode;
char errorAxis;

void initMcuCpu() {
  errorCode = 0;
  newStatus(statusUnlocked); 
}

// this also clears time and sets or clears motor reset pins
void newStatus(char newStatus) {
  // timer counting and ints off until move or homing command
  stopTimer();
  if (errorCode) return;
  set_resets(newStatus != statusUnlocked);
  mcu_status = newStatus; 
}

// axis is zero when not specific to axis
void handleError(char axis, Error code) {
  newStatus(statusUnlocked);
  errorAxis = axis;
  errorCode = code;
}

