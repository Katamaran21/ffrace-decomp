#ifndef FF_RAND_H
#define FF_RAND_H

/* FFRace.exe reaches rand and srand through the coredll import thunks
   0x0005b21c and 0x0005b220, so Race_Init 0x00013aec, Track_Generate 0x00014a20
   and 0x0004f6bc all draw from one stream. */
void Ff_Srand(unsigned int seed);
int  Ff_Rand(void);

#endif /* FF_RAND_H */
