
#include <xc.h>
#include "vector.h"
#include "main.h"
#include "spi.h"
#include "pins-b.h"
#include "mcu-cpu.h"
#include "motor.h"
#include "event.h"

// separate vector buffers for X and Y
Vector vecBufX[VEC_BUF_SIZE];
Vector vecBufY[VEC_BUF_SIZE];

Vector *vecBufHeadX, *currentVectorX, *vecBufHeadY, *currentVectorY;

void initVectors() {
  vecBufHeadX = currentVectorX = vecBufX;
  vecBufHeadY = currentVectorY = vecBufY;
}

void handleSpiWordInput() {
  char topSpiByte = ((char *) &spiWordIn)[3]; 

  if(errorCode) {
    if (topSpiByte == clearErrorCmd) 
      handleMotorCmd((char *) &spiWordIn);
    // all other input is ignored when error
    return;
  } 
  if(topSpiByte == 0)
    // cmd is zero means empty SPI word, ignore it
    return;
  
  if((topSpiByte & 0xc0) == 0) {
    handleMotorCmd((char *) &spiWordIn);  
    return;
  }
  if ((topSpiByte & 0x80) != 0) { 
    // we have a new non-command X vector
    // add it to vecBufX
    *((unsigned long *)vecBufHeadX) = spiWordIn;
    if (++vecBufHeadX == vecBufY + VEC_BUF_SIZE)
      vecBufHeadX = vecBufX;
    if (vecBufHeadX == currentVectorX)
      handleError(X, errorVecBufOverflow);
    return;
  }
  if ((topSpiByte & 0x40) != 0) { 
    // we have a new non-command Y vector
    // add it to vecBufY
    *((unsigned long *)vecBufHeadY) = spiWordIn;
    if (++vecBufHeadY == vecBufY + VEC_BUF_SIZE)
      vecBufHeadY = vecBufY;
    if (vecBufHeadY == currentVectorY)
      handleError(Y, errorVecBufOverflow);
    return;
  }
}

bool_t vecBufXIsAtHighWater() {
  int diff = (vecBufHeadX - currentVectorX);
  if (diff < 0 ) diff += VEC_BUF_SIZE;
  return (diff >= VEC_BUF_HI_WATER);
}

bool_t vecBufYIsAtHighWater() {
  int diff = (vecBufHeadY - currentVectorY);
  if(diff < 0) diff += VEC_BUF_SIZE;
  return (diff >= VEC_BUF_HI_WATER);
}