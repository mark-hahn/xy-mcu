
#ifndef BUFFERS_H
#define	BUFFERS_H

#include <stdint.h>

extern uint8_t spiBufX[128], spiBufY[128], timBufX[128], timBufY[128], retBuf[16];
extern volatile uint8_t spiBufWrX, spiBufRdX, spiBufWrY, spiBufRdY;
extern volatile uint8_t timBufWrX, timBufRdX, timBufWrY, timBufRdY;
extern volatile uint8_t retBufWr, retBufRd;

extern volatile uint8_t cmdStateBitsX;
#ifdef XY
extern volatile uint8_t cmdStateBitsY;
#endif


#endif	/* BUFFERS_H */

