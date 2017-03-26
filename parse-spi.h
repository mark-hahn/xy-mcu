/* 
 * File:   parse-spi.h
 * Author: root
 *
 * Created on March 25, 2017, 4:24 PM
 */

#ifndef PARSE_SPI_H
#define	PARSE_SPI_H

#include <stdint.h>

char numLeading1s(uint32_t *word);
char axisFromSpiWord(uint16_t *word);

#endif	/* PARSE_SPI_H */

