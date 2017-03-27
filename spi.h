
#ifndef SPI_H
#define	SPI_H

#include <stdint.h>
#include "main.h"

extern uint32_t spiWord;
extern uint16_t spiInts[2];
extern char    *spiBytes;

extern volatile char spiBytesIn[4];   // a word (four chars) from SPI, big-endian
extern volatile char spiBytesInIdx;  // index for spiBytesIn and spiBytesOut

void initSpi();

#endif	/* SPI_H */
