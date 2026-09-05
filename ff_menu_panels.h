#ifndef FF_MENU_PANELS_H
#define FF_MENU_PANELS_H

#include "ff_gfx.h"

/* FFRace.exe 0x00039b1c, its 0x00083488 == 0x33 block entered at 0x00042078. */
void MenuPanels_Track(const ff_surface *dst, int centre);

/* FFRace.exe 0x00039b1c, its 0x00083488 == 6 block. */
void MenuPanels_Ship(const ff_surface *dst, int centre);

#endif /* FF_MENU_PANELS_H */
