
#include <stdint.h>
#include <xc.h>

#include "invtable.h"

// invtable1/invtable2 are defined in invtable1.asm/invtable2.asm
extern const uint16_t invtable1;
extern const uint16_t invtable2;

uint16_t invTableAddr1;
uint16_t invTableAddr2;

void initInvtable() {
  invTableAddr1 = (uint16_t) &invtable1 - 0x1000;
  invTableAddr2 = (uint16_t) &invtable2 - 0x1800;
}

UsecsFp usecsFp;  // global set by pps2usecs call

// pps input is 9.6 fixed point format, i.e. in 64ths of pulse/sec, max 511.984
// pps2usecs returns mantissa and exponent for 1e6/pps
// exponent is for adjusting invtable mantissa value
// invtable contains usec values (mantissa) for only pps values >=64 && < 128
// shift 14-bit invtable value left by exp bits to get usecs
// keep exp separate from mantissa for extended precision fraction math
// vel: 8192 -> 128 pps -> tbl[0] >> 1 usecs   exp = -1
// vel: 4096 ->  64 pps -> tbl[0] << 0 usecs,  exp =  0
// vel: 2048 ->  32 pps -> tbl[0] << 1 secs,   exp =  1
UsecsFp pps2usecs(uint16_t pps) {
  int8_t exp = 0;
  if(pps = 0) {
    // our infinity is largest possible return value: 8.388 secs per pulse
    usecsFp.man = 0x3fff;
    usecsFp.exp = 9;
    return usecsFp;
  }
  if(pps > 0x7fff) pps = 0x7fff;
  // this is slow --- should use esp8266 asm instr to find # leading zeros
  while(pps >= 8192) { pps >> 1; exp++; }
  while(pps  < 4096) { pps << 1; exp--; }
  // pps is now >= 4096 (0x1000) and < 8192 (0x2000)
  // linker maxes out at 2048 words so table is split in two
  // invtables are already offset by 4096 (0x1000) and 6144 (0x1800)
  uint16_t addr = (pps < 0x1800 ? invTableAddr1 : invTableAddr2) + pps;
  NVMCON1bits.NVMREGS = 0;
  NVMADRL = addr & 0xff;
  NVMADRH = addr >> 8;
  NVMCON1bits.RD = 1;
  usecsFp.man = (((uint16_t) NVMDATH << 8) | NVMDATL);
  usecsFp.exp = exp;
  return usecsFp;
}
