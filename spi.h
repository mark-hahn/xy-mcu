
#ifndef SPI_H
#define	SPI_H

extern volatile char spiWordIn[4]; // filled by int routine, removed in event loop
extern volatile char spiWordInByteIdx;
extern char spiByteOut; // set by event loop each time SPI byte exchanged

void initSpi();

#endif	/* SPI_H */
