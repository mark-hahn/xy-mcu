
#ifndef PARSE_SPI_H
#define	PARSE_SPI_H

#include <stdint.h>
#include "motor.h"

#ifdef XY
uint8_t axisFromSpiWord(uint32_t *word);
#endif

uint8_t parseVector(uint32_t *vector, MoveState *moveState);

#endif	/* PARSE_SPI_H */

