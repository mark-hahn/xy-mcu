
#ifndef COMMAND_H
#define	COMMAND_H

// used for flashing new MCU code
#define NEW_RESET_VECTOR   0x200
#define WRITE_FLASH_BLOCKSIZE 32  

void handleSpiWord();

#endif	/* COMMAND_H */

