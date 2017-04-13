
#include <stdint.h>

// ring buffers
uint8_t spiBufX[128], spiBufY[128], timBufX[128], timBufY[128], retBuf[16];
volatile uint8_t spiBufWrX, spiBufRdX, spiBufWrY, spiBufRdY;
volatile uint8_t timBufWrX, timBufRdX, timBufWrY, timBufRdY;
volatile uint8_t retBufWr, retBufRd;

volatile uint8_t cmdStateBitsX;
#ifdef XY
volatile uint8_t cmdStateBitsY;
#endif

// main top-level non-interrupt loop to handle all processing
void runProcess() {
  while(1) {
    
    
  }
}