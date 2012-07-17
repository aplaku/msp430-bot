/******************************************************************************
 * Mark Hageman, Mark Labbato, Aldo Plaku
 * MSP430-Bot
 * 2012 Co-op Design Challenge (Texas Instruments)
 *
 * rand.h
 *
 * Pseudo-random number generation.
 * Not very robust or fair (don't use for crypto).
 * Uses floating multiplication and modulo to generate random-ish numbers
******************************************************************************/

#ifndef RAND_H_
#define RAND_H_

unsigned int lfsr = 0xACE1;

int LFSR_next(void){
	//Directly from wikipedia
	unsigned int bit;
	/* taps: 16 14 13 11; characteristic polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
	bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    lfsr =  (lfsr >> 1) | (bit << 15);
    return lfsr;
}

void SeedRand( unsigned int seed ) {
	lfsr = seed;
}

// Random number based on TimerA CCR1 value
int timer_rand(int max){
	unsigned int val = CCR1 % max;
	return val;
}

#define Random()		((LFSR_next() & 0x8000) >> 15) 			// random in the range [0, 1]

#endif /* RAND_H_ */
