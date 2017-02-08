
#include <xc.h>
#include <string.h>
#include "vector.h"
#include "main.h"
#include "spi.h"
#include "mcu-cpu.h"
#include "motor.h"

// separate vector buffers for X and Y
Vector vecBufX[VEC_BUF_SIZE];
Vector vecBufY[VEC_BUF_SIZE];

Vector *vecBufHeadX, *currentVectorX, *vecBufHeadY, *currentVectorY;

void initVectors() {
  vecBufHeadX = currentVectorX = vecBufX;
  vecBufHeadY = currentVectorY = vecBufY;
}

// code is duplicated for X and Y because speed is more important than code size
void handleSpiWordInput() {
  char topSpiByte = *((char *) &spiWordIn);
  if(errorCode) {
    if (topSpiByte == clearErrorCmd) 
      handleMotorCmd((char *) &spiWordIn);
    return;
  }
  if ((topSpiByte & 0x80) != 0) { 
    // we have a new X vector
    memcpy(vecBufHeadX, &spiWordIn, sizeof(spiWordIn));
    if (++vecBufHeadX == currentVectorX + VEC_BUF_SIZE)
      vecBufHeadX = vecBufX;
    if (vecBufHeadX == currentVectorX)
      handleError(X, errorVecBufOverflow);
    return;
  }
  // we have a new Y vector
  if ((topSpiByte & 0x40) != 0) { 
    memcpy(vecBufHeadY, &spiWordIn, sizeof(spiWordIn));
    if (++vecBufHeadY == currentVectorY + VEC_BUF_SIZE)
      vecBufHeadY = vecBufY;
    if (vecBufHeadY == currentVectorY) {
      handleError(Y, errorVecBufOverflow);
    }
    return;
  }
  if(topSpiByte != 0) {
    // we have a command not a vector
    handleMotorCmd((char *) &spiWordIn);
    return; 
  }
  // if cmd is zero means empty SPI word, ignore it
}

bool_t vecBufXIsAtHighWater() {
  int diff = (vecBufHeadX - currentVectorX);
  if (diff < 0 ) diff += VEC_BUF_SIZE;
  return (diff > VEC_BUF_HI_WATER);
}

bool_t vecBufYIsAtHighWater() {
  int diff = (vecBufHeadY - currentVectorY);
  if(diff < 0) diff += VEC_BUF_SIZE;
  return (diff > VEC_BUF_HI_WATER);
}