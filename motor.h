
#ifndef MOTOR_H
#define	MOTOR_H

#include "main.h"

void motorReset(char axis, bool_t resetHigh);
void handleMotorCmd(char volatile *word);

#endif	/* MOTOR_H */
