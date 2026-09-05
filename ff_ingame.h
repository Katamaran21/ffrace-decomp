#ifndef FF_INGAME_H
#define FF_INGAME_H

#include "ff_gfx.h"

/* FFRace.exe Race_Init 0x00013aec: 0x000a7728 = 0xfffffffc, 0x000a7658 = 0. */
#define FF_COUNTDOWN_START (-4)

/* FFRace.exe 0x0004f6bc advances 0x000a7728 once per 1000 of 0x000a7654 and
   folds 0x3c of them into 0x000a772c. */
#define FF_SECOND_MS  1000
#define FF_MINUTE_SEC 0x3c

/* FFRace.exe 0x0005b194 stops 24000 pixels short of the surface; the literal at
   0x0005b20c is 0x7fffff9c. */
#define FF_SKY_SHIFT_SHORT 24000

/* FFRace.exe 0x0004e3c8 scales 0x000832b0 by 0x3fe4cccccccccccd. */
#define FF_SKY_SCROLL 0.65

/* FFRace.exe Race_Init 0x00013aec and Track_Generate 0x00014a20. */
void Ingame_Start(int mode, int seed, int length, int theme, int sky);

/* FFRace.exe 0x00083380, cleared by 0x0004f6bc and set by Screen_Set
   0x00014924. */
int  Ingame_Active(void);
void Ingame_Stop(void);

/* FFRace.exe 0x000833b4, which Music_Select 0x00045b34 tests on both of its
   branches. */
int  Ingame_IntroPending(void);

/* FFRace.exe 0x0004f6bc WM_TIMER between 0x000a7664 and the physics pass. */
void Ingame_Tick(void);

/* FFRace.exe 0x0004e3c8. */
void Ingame_Frame(const ff_surface *dst);

/* FFRace.exe 0x000a7650, 0x000a7654 and 0x000a772c. */
int Ingame_Fps(void);
int Ingame_SecondMs(void);
int Ingame_Minutes(void);

/* FFRace.exe 0x000a764c, the WM_TIMER counter 0x00046754 divides by 2. */
int Ingame_Frames(void);

#endif /* FF_INGAME_H */
