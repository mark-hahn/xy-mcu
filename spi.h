
#ifndef SPI_H
#define	SPI_H

// separate buffers for in and out
extern char *vecBufBytesIn;
extern char spiByteOut;

void spiInit();
void chkSpi();

#endif	/* SPI_H */

