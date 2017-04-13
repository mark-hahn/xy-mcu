
#include <xc.h>
#include "int.h"
#include "mcu-cpu.h"
#include "mcu-api.h"
#include "util.h"


Error errorCode;

// axis is zero when not specific to axis
void handleError(char axis, Error code) {
  // clean up comm state
  
    errorCode = code;
}

