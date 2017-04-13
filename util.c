
#include <xc.h>
#include "int.h"
#include "mcu-cpu.h"
#include "mcu-api.h"
#include "util.h"


Error errorCode;
volatile uint8_t statusCountX;
volatile uint8_t statusCountY;

// axis is zero when not specific to axis
void handleError(char axis, Error code) {
  // clean up comm state
  statusCountX = statusCountY = 0;
  errorCode = code;
}

// 10FY YGXX  status count > 0 (F:Y, G:X), buf space, XX,YY: 2^(2*XX), 1, 4, 16, 64
uint8_t defStateByte() {
  uint8_t spaceX = bufSpaceInX();
  
  return 0x80 | ((statusCountX > 0) << 5) | 
}