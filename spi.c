
#include <xc.h>
#include "spi.h"

#define VEC_BUF_SIZE     64 // 256/4, allows byte masking and modulo arithmetic
#define VEC_BUF_HI_WATER 40 // flow control limit

typedef struct Vector {
  shortTime_t usecsPerPulse; // same size as timer 1
  // ctrlWord has five bit fields, from msb to lsb ...
  //   1 axis X, both X and Y set means command, not vector
  //   1 axis Y
  //   1 dir (0: backwards, 1: forwards)
  //   3 ustep idx, 0 (full-step) to 5 (1/32 step)
  //  10 pulse count
  unsigned int ctrlWord;
} Vector;

Vector vecBuf[VEC_BUF_SIZE];
char *vecBufBytesIn; // bytes, updated by int routine in main.c
Vector *vecBufHead, *vecBufTail;

void clrVecBuf() {
  vecBufBytesIn = (char *) vecBuf;
  vecBufHead = vecBufTail = vecBuf;
}

char spiByteOut; // set by chkSpi() each time SPI byte exchanged

// this queue maintained by chkSpi()
// status out buf by bytes ...
  // 1 header marker, 0xff, syncs bytes, 0xfe for flow control
  // 1 status, axis, error code, and state
  // 3 home distance X
  // 3 home distance Y
#define STATUS_BUF_SIZE 8  

char statusBuf[STATUS_BUF_SIZE] = {0xff, 0, ...};
char *statusBufOut;  // updated by int routine in main.c

void spiInit() {
  SSP1SSPPS  = 0x07; // A7 => select in
  SSP1CLKPPS = 0x13; // C3 => clock in
  SSP1DATPPS = 0x14; // C4 => data in
  RC5PPS     = 0x16; // Data out => C5
  TRISA7 = 1; // select input
  TRISC3 = 1; // clock input
  TRISC4 = 1; // data input
  TRISC5 = 0; // data output
  SSP1CON1bits.SSPM  = 4; // mode: spi slave with select enabled
  SSP1STATbits.SMP   = 0; // input sample edge (must be zero for slave mode)
  SSP1CON1bits.CKP   = 0; // clk low is idle (should match CPU)
  SSP1STATbits.CKE   = 1; // clk non-idle -> idle bit capture (safer wcol))
  SSP1CON3bits.BOEN  = 0; // enable buffer overflow check (SSPOV))
  
  clrVecBuf();
  statusBufOut = statusBuf;
  
  /* Before enabling the module in SPI Slave mode, the clock
   line must match the proper Idle state (CKP) */
  while(RC3);
  SSP1CON1bits.SSPEN = 1; // enable SPI
}

void chkSpi() {
  if(((unsigned int) vecBufBytesIn) % 4 == 3) {
    // received a new vector
    vecBufHead = (Vector *) vecBufBytesIn;
    if(vecBufHead == vecBuf + VEC_BUF_SIZE) {
      vecBufHead = vecBuf;
      vecBufBytesIn = (char *) vecBuf;
    }
  }
}
