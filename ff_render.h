#ifndef FF_RENDER_H
#define FF_RENDER_H

#include "ff_gfx.h"

/* FFRace.exe .data 0x000832d0 initialiser 7, the road width in track units;
   0x00046754 adds it to the left edge of every band. */
#define FF_ROAD_WIDTH 7

/* FFRace.exe .data 0x000832d8 initialiser 110.0f, the cap 0x00046754 compares
   0x000832d8 against before adding 1.0f. */
#define FF_ROAD_SCALE_MAX 110.0f

/* FFRace.exe .data 0x000832d4 initialiser 110.0f, the numerator 0x000216c4
   divides by 0x000832d8 once per row. */
#define FF_ROAD_SCALE_REF 110.0f

/* FFRace.exe 0x00046754 writes 0x23 into the layout marker 0x000a7630 for the
   duration of the drawer call and restores 0x14 afterwards. */
#define FF_ROW_OFFSET_176x220 0x23

/* FFRace.exe 0x00011b18 and 0x00011b2c each copy 0x70 bytes, so 28 entries of
   0x000a3d00 and of its difference table are reachable. */
#define FF_ROW_COUNT 28

/* FFRace.exe 0x00046754 calls the theme drawer, 0x000216c4 for theme 0, as
   (0x000a766c + near_left, (int)y_far, 0x000a766c + far_left, (int)y_near,
   0x000a766c + near_right, 0x000a766c + far_right, depth); 0x000216c4 blits
   into the global back buffer &DAT_000a43f8. */
typedef void (*ff_segment_drawer)(const ff_surface *dst,
                                  int near_left, int y_far, int far_left,
                                  int y_near, int near_right, int far_right,
                                  int depth);

/* FFRace.exe Game_Init 0x000115b0 at 0x000117cc fills 0x000902d0 with 198 rows
   of 0x190 bytes, row h holding h entries, and stores the row count 199 into
   0x000a777c. */
#define FF_RAMP_ROWS   199
#define FF_RAMP_STRIDE 200

/* FFRace.exe Game_Init 0x000115b0 at 0x00011a00 fills 0x000a3d00 with 29
   entries and copies 0x70 bytes of it to 0x000a3de0, and the successive
   differences to 0x000a3e50. */
void Render_BuildTables(void);

/* FFRace.exe 0x00046754, the band projection built from the code literals
   0x463b8000, 0x42480000, 0x41f00000, 0x43800000 and 0x42a00000. */
float Render_BandY(float z);

/* FFRace.exe 0x00046754, the 0x0008339c + 1 band loop. */
void Render_Road(const ff_surface *dst, ff_segment_drawer drawer);

/* FFRace.exe 0x000a7630 as the drawer sees it, subtracted there from every row
   before the 0x000832a4 + 1 clip. */
int Render_RowOffset(void);

/* FFRace.exe 0x000a766c, written by 0x00046754 before the band loop and read
   again by its ship pass ahead of LAB_00048a10. */
int Render_BaseX(void);

/* FFRace.exe 0x000832d8, stepped by 1.0f per frame in 0x00046754 and divided
   into 0x000832d4 by 0x000216c4. */
float Render_RoadScale(void);

/* FFRace.exe 0x000216c4 reads the ramp entry as
   *(short *)(&DAT_000902d0 + ((h * 200 - y_far) + y) * 2). */
const short *Render_RampRow(int h);

#endif /* FF_RENDER_H */
