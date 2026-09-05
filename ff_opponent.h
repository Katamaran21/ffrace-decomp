#ifndef FF_OPPONENT_H
#define FF_OPPONENT_H

#include "ff_gfx.h"

/* FFRace.exe 0x00046754 at 0x00047f70 .. 0x00048234, the racer loop that runs
   before the player blits of 0x00048300. */
void Opponent_Shadows(const ff_surface *dst);

/* FFRace.exe 0x00046754 at 0x00048a10 .. 0x00048ca4, the racer loop that runs
   after them. */
void Opponent_Ships(const ff_surface *dst);

/* FFRace.exe 0x0004f6bc at 0x000542a0 .. 0x00055478, the per-racer AI pass that
   follows the player's own speed and distance step of 0x00053430. */
void Opponent_Step(int dt_ms);

#endif /* FF_OPPONENT_H */
