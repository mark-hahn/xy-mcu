
#include <xc.h>
#include <string.h>
#include "vectors.h"
#include "spi.h"

// must be 32-bits to fit in SPI word
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

// separate buffers for in and out
Vector vecBufX[VEC_BUF_SIZE];
Vector vecBufY[VEC_BUF_SIZE];

Vector *vecBufHeadX, *vecBufTailX, *vecBufHeadY, *vecBufTailY;

void clrVecBufs() {
  vecBufHeadX = vecBufTailX = vecBufX;
  vecBufHeadY = vecBufTailY = vecBufY;
}

void initVectors() {
  clrVecBufs();
}

void handleNewVecBufByte() {
  if ((spiWordIn[0] & 0x80) != 0) { // we have a new X vector
    // after this copy, new bytes can start coming in
    memcpy(vecBufHeadX, &spiWordIn, sizeof(spiWordIn));
    if (++vecBufHeadX = vecBufTailX + VEC_BUF_SIZE)
      vecBufHeadX = vecBufX;
    if (vecBufHeadX = vecBufTailX) {
      handleError(0, errorVecOverflow);
      
    }

    
    
  }

  spiLastVecBufBytesIn = vecBufBytePtrX;

  
  if(vecBufBytePtrX == (char *) vecBufX + sizeof(vecBufX))
    vecBufBytePtrX -= sizeof(vecBufX);
  // we have new 32-bit cmd or vector word
  if((vecBufBytePtrX - (char *) vecBufX) %4 == 0) {
    vecBufHead = (Vector *) vecBufBytePtrX;
  }
  
  spiLastVecBufBytesIn = vecBufBytePtrX;
}
