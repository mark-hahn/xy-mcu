
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
    // all other input is ignored when error
    return;
  }
  if(topSpiByte == 0)
    // cmd is zero means empty SPI word, ignore it
    return;
  
  if(topSpiByte & 0xc0 == 0) {
    handleMotorCmd((char *) &spiWordIn);  
    return;
  }
  if ((topSpiByte & 0x80) != 0) { 
    // we have a new absolute X vector
    // add it to vecBufX
    memcpy(vecBufHeadX, &spiWordIn, sizeof(spiWordIn));
    if (++vecBufHeadX == currentVectorX + VEC_BUF_SIZE)
      vecBufHeadX = vecBufX;
    if (vecBufHeadX == currentVectorX)
      handleError(X, errorVecBufOverflow);
    return;
  }
  if ((topSpiByte & 0x40) != 0) { 
    // we have a new absolute Y vector
    // add it to vecBufY
    memcpy(vecBufHeadY, &spiWordIn, sizeof(spiWordIn));
    if (++vecBufHeadY == currentVectorY + VEC_BUF_SIZE)
      vecBufHeadY = vecBufY;
    if (vecBufHeadY == currentVectorY) {
      handleError(Y, errorVecBufOverflow);
    }
    return;
  }
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