
#ifndef PARSE_SPI_H
#define	PARSE_SPI_H

#include <stdint.h>
#include "motor.h"

uint8_t axisFromSpiWord(uint32_t *word);
void    parseMove(uint32_t *word, MoveState *moveState);

#endif	/* PARSE_SPI_H */

