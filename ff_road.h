#ifndef FF_ROAD_H
#define FF_ROAD_H

#include "ff_gfx.h"

/* FFRace.exe 0x000216c4, the theme-0 segment drawer 0x00046754 calls through
   the table it indexes with 0x000833b8. */
void Road_DrawSegment(const ff_surface *dst, int near_left, int y_far,
                      int far_left, int y_near, int near_right, int far_right,
                      int depth);

#endif /* FF_ROAD_H */
