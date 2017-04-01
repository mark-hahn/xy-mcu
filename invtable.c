
#include <stdint.h>
#include <xc.h>

// invtable1/invtable2 are defined in invtable1.asm/invtable2.asm
extern const uint16_t invtable1;
extern const uint16_t invtable2;

uint16_t invTableAddr1;
uint16_t invTableAddr2;

void initInvtable() {
  invTableAddr1 = (uint16_t) &invtable1;
  invTableAddr2 = (uint16_t) &invtable2 - 0x800;
}

uint16_t pps2usecs(uint16_t pps) {
//  return (unsigned short long)(1000000) / pps;

  uint16_t addr = (pps < 0x800 ? invTableAddr1 : invTableAddr2) + pps;
  NVMCON1bits.NVMREGS = 0;
  NVMADRL = addr & 0xff;
  NVMADRH = addr >> 8;
  NVMCON1bits.RD = 1;
  return (((uint16_t) NVMDATH << 8) | NVMDATL);
}
