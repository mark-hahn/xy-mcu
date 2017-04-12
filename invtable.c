
#include <stdint.h>
#include <xc.h>

#include "invtable.h"

// invtable1/invtable2 are defined in invtable1.asm/invtable2.asm
extern const uint16_t invtable1;
extern const uint16_t invtable2;

uint16_t invTableAddr1;
uint16_t invTableAddr2;

void initInvtable() {
  invTableAddr1 = (uint16_t) &invtable1;
  invTableAddr2 = (uint16_t) &invtable2 - 0x0800;
}

UsecsFp usecsFp;  // global set by pps2usecs call

// pps 15-bit fp format: eee vvvv vvvv vvvv
// pps == (0x1000 + man) * 2 ^ (exp - 6)  (i.e. shift adj man right 6)
// 0b0000 0000 0000 0000, smallest pps =>    64, (exp of 0 means -6)
// 0b0111 1111 1111 1111, largest pps  => 16382, (exp of 7 means +1)

// usecs: mantissa is 14 bits from table, exp = 3 bits
// usecs == (0x4000 + man) / 2 ^ exp (i.e. add 0x4000 & shift right exp)
// 0b000, 0b0001 1110 1000 0101, smallest usecs =>  189+, (exp of 0 means -7) 
// 0x1e85 -> 0x5e85 -> 24197 >> 7 => 189+
// 0b111, 0b0011 1101 0000 1001, largest  usecs => 32009, (exp of 7 means  0) 
// 0x3d09 -> 0x7d09 -> 32009 >> 0 => 32009

void pps2usecs(uint16_t pps) {
  // linker maxes out at 2048 words so table is split in two
  // 2nd table addr is already offset by 2048 (0x0800)
  uint16_t ofs = (pps & 0x0fff);
  uint16_t addr = (ofs < 0x1800 ? invTableAddr1 : invTableAddr2) + ofs;
  NVMCON1bits.NVMREGS = 0;
  NVMADRL = addr & 0xff;
  NVMADRH = addr >> 8;
  NVMCON1bits.RD = 1;
  usecsFp.man = (0x4000 | ((uint16_t) NVMDATH << 8) | NVMDATL);
  usecsFp.exp = (pps >> 12);
}
