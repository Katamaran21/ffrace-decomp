#ifndef FF_RESULTS_H
#define FF_RESULTS_H

#include "ff_gfx.h"

/* FFRace.exe 0x00046754 at 0x0004ccc4, 0x0004d4a4 and 0x0004dea4, the three
   0x000a779c branches that raise 0x000a4634 once the run has ended; they sit
   after the layout HUD and before the 0x000a463c tail. */
void Results_Frame(const ff_surface *dst);

#endif /* FF_RESULTS_H */
