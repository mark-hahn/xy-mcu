
#ifndef SPI_H
#define	SPI_H

#include "main.h"

extern volatile char   spiByteFromCpu;  // set by spi interrupt, used in event loop
extern unsigned long   spiWordIn;       // four of spiByteFromCpu
extern char            spiByteToCpu;    // set by event loop, used in spi interrupt
extern unsigned long   spiWordOut;      // four of spiByteToCpu
extern char            spiWordByteIdx;  // index for spiWordIn and spiWordOut
extern volatile bool_t spiIntHappened;
extern char            nextRetWordType; // type of word to be returned next

void initSpi();
void chkStatusWord();

#endif	/* SPI_H */
