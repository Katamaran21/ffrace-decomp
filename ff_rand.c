#include "ff_rand.h"

/* ISO/IEC 9899:1990 7.10.2.1 prints this rand/srand pair as its example
   implementation.  FFRace.exe reaches rand through the coredll import thunk
   0x0005b21c, so the generator it used is not part of the image. */
static unsigned long ff_seed = 1;

void Ff_Srand(unsigned int seed)
{
    ff_seed = seed;
}

int Ff_Rand(void)
{
    ff_seed = ff_seed * 1103515245UL + 12345UL;
    return (int)((unsigned long)(ff_seed / 65536UL) % 32768UL);
}
