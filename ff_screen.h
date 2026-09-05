#ifndef FF_SCREEN_H
#define FF_SCREEN_H

#include "ff_gfx.h"

/* FFRace.exe Screen_Set 0x00014924 stores the id in 0x00083488, the parameter
   in 0x000a7708 and arms the transition counter 0x000a77f4 with 0x14. */
void Screen_Set(int id, int param);

int  Screen_Id(void);
int  Screen_Param(void);

/* FFRace.exe 0x00039b1c writes 0x000a7708 from the plate the stylus hit. */
void Screen_SetParam(int param);

/* FFRace.exe Game_Init 0x000115b0 writes 0x14 to the layout marker 0x000a7630
   only in the ExtEscape width 0xb0 branch. */
void Screen_SetLayout(int layout);
int  Screen_Layout(void);

/* FFRace.exe 0x000a77f4, read and decremented by the 0x00039b1c screen
   branches. */
int  Screen_Transition(void);
void Screen_TransitionStep(void);

/* FFRace.exe WndProc 0x0004f6bc activation dispatch at 0x00050000 selects on
   0x000a7708 and then on 0x00083488.  Returns non-zero only for the
   0x0004e0c0 branch, item 4 of 0x00083488 == 0. */
int Screen_Activate(int screen, int item);

/* FFRace.exe WndProc 0x0004f6bc WM_LBUTTONUP: 0x00083488 == 2 ends with
   Screen_Set(0, 0) whatever the coordinates.  Returns non-zero when the tap was
   consumed there. */
int Screen_Tap(int x, int y);

/* FFRace.exe 0x00016080 / 0x00016354 index 0x000a77d0. */
int Screen_CareerRace(void);

/* FFRace.exe 0x000a77d0, which no instruction in the image writes. */
void Screen_SetCareerRace(int race);

/* FFRace.exe WndProc 0x0004f6bc back-key branch at 0x0004fd58: 0x00083488 in
   {1, 2, 3} takes Screen_Set(0, 0), 5 takes Screen_Set(3, 0) and 0x33 takes
   Screen_Set(1, 0).  0, 4, 0x2a and 0x2b have no case. */
int Screen_Back(void);

/* FFRace.exe 0x0004e0c0. */
int Screen_Farewell(const ff_surface *dst);

/* FFRace.exe 0x000a779c, written by items 0, 1 and 2 of screen 1 at 0x00050098
   and read as the Race_Init 0x00013aec mode by the 0x000500bc start. */
void Screen_SetPendingMode(int mode);
int  Screen_PendingMode(void);

/* FFRace.exe 0x000a76c0, the screen 0x33 cursor: 0 selects the random track and
   1 .. 10 name one, and 0x000500bc passes it to Race_Init 0x00013aec less one.
   0x000501e8 floors the decrement at 0 and 0x000501bc caps the touch increment
   at 9, while the key pair at 0x0004fc78 caps its own at 10. */
void Screen_SetTrack(int track);
int  Screen_Track(void);

/* FFRace.exe 0x000500bc: 0x00083380 = 0, Race_Init 0x00013aec, then
   0x000a7708 = 0 and 0x00083488 = 1 as direct stores, with no 0x00014924 call
   and so no 0x000a77f4 transition. */
void Screen_StartRace(int mode, int theme);

#endif /* FF_SCREEN_H */
