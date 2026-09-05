#ifndef FF_MENU_SCREENS_H
#define FF_MENU_SCREENS_H

#include "ff_gfx.h"

/* FFRace.exe 0x00039b1c reaches its settled plate and its hit test only when
   0x000a7718 is 0 and 0x00083488 is none of 6, 7, 2. */
int MenuScreens_Plates(int screen);

/* FFRace.exe 0x00039b1c LAB_0003a420 / LAB_0003a428 / LAB_0003a448 with
   0x000a7628 held at 0: the reach test both the plate and the hit test share. */
int MenuScreens_Reach(int screen, int item);

/* FFRace.exe 0x00039b1c LAB_0003a448: the extra condition the settled plate
   carries and the hit test does not. */
int MenuScreens_Plate(int screen, int item);

/* FFRace.exe 0x00039b1c per-screen Text_DrawCentered blocks. */
void MenuScreens_Draw(const ff_surface *dst, int screen, int item, int centre);

/* FFRace.exe 0x00016354 index 0x24, the last career race. */
#define FF_CAREER_LAST 36

#endif /* FF_MENU_SCREENS_H */
