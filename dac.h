
#ifndef DAC_H
#define	DAC_H


// V = (val * 3.3 / 32 - 0.65)/2; amps = 2 * V
#define set_dac(val) DAC1CON1bits.DAC1R = (val)


void initDac();


#endif	/* DAC_H */


// The following are motor current VREFs measured by dac settings
//  4 -> 0       s.b. -0.119
//  5 -> 0.009   s.b. -0.067
//  6 -> 0.041   s.b. -0.035
//  7 -> 0.083   s.b.  0.036
//  8 -> 0.128   s.b.  0.088
//  9 -> 0.175   s.b.  0.139
// 10 -> 0.222   s.b.  0.191
// 20 -> 0.710   s.b.  0.706
// 31 -> 1.275   s.b.  1.273
