
#ifndef PARSE_SPI_H
#define	PARSE_SPI_H

#include <stdint.h>

uint8_t axisFromSpiWord(uint32_t *word);
void    accelsFromSpiWord(uint32_t *word, int8_t *accels);

#endif	/* PARSE_SPI_H */

