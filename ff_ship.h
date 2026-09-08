#ifndef FF_SHIP_H
#define FF_SHIP_H

#include "ff_gfx.h"

/* FFRace.exe .data 0x00083438, the table 0x00046754 indexes with 0x000a77d8 for
   the player at 0x000483a0 and with 0x00085450 + slot * 4 for every other racer
   at 0x00048ad4. */
#define FF_BOB_PHASES 20

/* FFRace.exe 0x00046754 at 0x00048300 and 0x000483a0, after the band loop and
   before the 0x000a463c tail. */
void Ship_Draw(const ff_surface *dst);

/* FFRace.exe .data 0x00083438. */
int Ship_Bob(int phase);

/* FFRace.exe 0x000a44e0, the LoadBitmapW ship table; 0x00046754 reads entry
   0x000832f8 for the player and entry 0x000a4320[slot] + 1 at 0x00048b74 for
   every other racer. */
int Ship_SpriteRes(int index);

/* FFRace.exe 0x00046754, the 39999 < 0x000a7670 block that steps 0x000a77c0
   and blits 0x000a456c. */
void Ship_Wreck(const ff_surface *dst);

/* FFRace.exe 0x00046754 calls playSound(0x000a4db0, 0x000a4c20, 0x10000000)
   when it enters that block with 0x000a77c0 at 0. */
int Ship_TakeWreckSound(void);

/* FFRace.exe 0x0004ccd0 reads 0x000a77c0 beside 0x000a77cc for the results gate
   and 0x0004ceb0 tests it again for the headline. */
int Ship_WreckCount(void);

#endif /* FF_SHIP_H */
