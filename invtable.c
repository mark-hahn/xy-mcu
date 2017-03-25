
//#include <stdint.h>
#include <xc.h>
#include "invtable.h"
#include "main.h"

uint16_t pps2usecs(uint16_t pps) {
  uint16_t addr;
  if(pps < 0x800)
    addr =  ((uint16_t) &invtable1 & 0x3ff) + pps;
  else
    addr =  ((uint16_t) &invtable2 & 0x3ff) + pps - 0x800;
  NVMCON1bits.NVMREGS = 0;
  NVMADRL = addr & 0xff;
  NVMADRH = addr >> 8;
  NVMCON1bits.RD = 1;
  return (((uint16_t) NVMDATH << 8) | NVMDATL);
}
