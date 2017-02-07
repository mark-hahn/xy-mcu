
#include <xc.h>
#include <string.h>
#include "vectors.h"
#include "main.h"
#include "spi.h"
#include "cpu.h"
#include "motor.h"

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

// separate vector buffers for X and Y
Vector vecBufX[VEC_BUF_SIZE];
Vector vecBufY[VEC_BUF_SIZE];

Vector *vecBufHeadX, *currentVectorX, *vecBufHeadY, *currentVectorY;

void clrVecBufs() {
  vecBufHeadX = currentVectorX = vecBufX;
  vecBufHeadY = currentVectorY = vecBufY;
}

void initVectors() {
  clrVecBufs();
}

// code is duplicated for X and Y because speed is more important than code size
void handleNewSpiWord() {
  // we have a new X vector
  if ((spiWordIn[0] & 0x80) != 0) { 
    // after this copy, new bytes can start coming in
    memcpy(vecBufHeadX, spiWordIn, sizeof(spiWordIn));
    if (++vecBufHeadX == currentVectorX + VEC_BUF_SIZE)
      vecBufHeadX = vecBufX;
    if (vecBufHeadX == currentVectorX)
      handleError(X, errorVecBufOverflow);
    return;
  }
  // we have a new Y vector
  if ((spiWordIn[0] & 0x40) != 0) { 
    // after this copy, new bytes can start coming in
    memcpy(vecBufHeadY, spiWordIn, sizeof(spiWordIn));
    if (++vecBufHeadY == currentVectorY + VEC_BUF_SIZE)
      vecBufHeadY = vecBufY;
    if (vecBufHeadY == currentVectorY) {
      handleError(Y, errorVecBufOverflow);
    }
    return;
  }
  // we have a command not a vector
  handleMotorCmd(spiWordIn);
}
