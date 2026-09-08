#ifndef FF_RACE_H
#define FF_RACE_H

/* FFRace.exe .data 0x0008339c initialiser 0x11, the depth count the
   back-to-front loop of 0x00046754 walks. */
#define FF_DRAW_DEPTHS 17

/* FFRace.exe Race_Init 0x00013aec and Track_Generate 0x00014a20 both copy 200
   bytes into 0x000a41f8, 0x000a4130, 0x000a4068 and 0x000a3ed8. */
#define FF_WINDOW_SEGMENTS 50

/* FFRace.exe Track_Generate 0x00014a20 fills 0x0008e328, 0x0008a4a8,
   0x0008c3e8 and 0x00086628 for up to 0x000832c4 + 0xc8 entries; the tables sit
   0x1f40 bytes apart, so each holds 2000. */
#define FF_TRACK_SEGMENTS 2000

/* FFRace.exe Race_Init 0x00013aec stores its first argument in 0x000a779c and
   branches on > 0; 1 forces 0x000832c4 = 0x5db and 0x000a7754 = 0xfffffff6. */
#define FF_RACE_SCRIPTED  0
#define FF_RACE_ENDLESS   1
#define FF_RACE_ENDLESS_2 2

/* FFRace.exe .data 0x000832c4 initialiser 0x190; Race_Init 0x00013aec is called
   with 1000 from 0x0004ff28 and with 0x5db forced for mode 1. */
#define FF_TRACK_DEFAULT_LEN 1000
#define FF_TRACK_ENDLESS_LEN 0x5db

/* FFRace.exe Track_Generate 0x00014a20 picks the theme with rand() when its
   first argument is -1, and the sky variant with rand() % 3 when the second is
   -1.  0x000833b8 selects the segment drawer in 0x00046754. */
#define FF_THEME_RANDOM (-1)
#define FF_SKY_RANDOM   (-1)
#define FF_THEME_COUNT  10

/* FFRace.exe Race_Init 0x00013aec: srand() runs only when the seed argument is
   not -1. */
#define FF_SEED_NONE (-1)

/* FFRace.exe .data 0x000832fc initialiser 10; 0x0004f6bc converts it with
   __stoi and subtracts it from the travelled distance once per segment. */
#define FF_SEGMENT_LEN 10

/* FFRace.exe 0x0004f6bc at 0x0005154c writes 6 into 0x000a770c before it walks
   0x000a3b70 downwards, and tests 0x000865d0 + 4 * 5 as the last entry. */
#define FF_RACER_SLOTS 6

/* FFRace.exe 0x0004f6bc stores 1 into 0x000a770c for the player pass. */
#define FF_RACER_PLAYER 1

/* FFRace.exe Race_Init 0x00013aec writes 0x000865d8 and 0x000a42f8, and
   0x00046754 writes 0x000a3be0: element 2 of the racer tables, the first
   non-player slot and the only one the 0x000a779c == 2 pass projects. */
#define FF_RACER_FIRST 2

/* FFRace.exe 0x0004f6bc stores 0x1e into 0x000a76b4 and 0x000865d4 on every
   advance taken while 0x000a779c > 0. */
#define FF_ENDLESS_SEGMENT 0x1e

/* FFRace.exe 0x00016cec compares its argument against 0 .. 0x1a. */
#define FF_RACER_NAMES 27

void Race_Init(int mode, int seed, int length, int theme, int sky);

/* FFRace.exe 0x0004f6bc consumes one segment per FF_SEGMENT_LEN of travelled
   distance held in 0x000a4308 + 4 * 0x000a770c. */
void Race_Advance(float distance);

int Race_Mode(void);
int Race_Theme(void);
int Race_Sky(void);
int Race_Length(void);

/* FFRace.exe 0x000a77f0 sits past the 0x1800 raw span of .data at 0x00083000,
   so it starts at 0; Race_Init 0x00013aec raises it for a career entry of 3 and
   the 0x0004f6bc screen 2 tap toggles it.  0x00047f70 and 0x00048a10 widen the
   opponent slot bound by 2 while it is 1, and 0x0004d3a0 raises the overlay. */
int Race_Cops(void);

/* FFRace.exe Track_Generate 0x00014a20 LoadBitmapW ids, keyed on 0x000833b8 and
   split on the layout marker 0x000a7630. */
int Race_SkyResource(void);

/* FFRace.exe Track_Generate 0x00014a20 writes 0x000a77b8 per theme. */
int Race_SkyBlend(void);

/* FFRace.exe 0x000a41f8, the 50-entry road-centre window 0x00046754 reads. */
const float *Race_CentreWindow(void);

/* FFRace.exe 0x000a4130, the per-depth value 0x0008a4a8 carried through
   __stoi; the segment drawers test it against 0, 1, 0xb and 0xf. */
const int *Race_SceneryWindow(void);

/* FFRace.exe 0x000a3ed8, the rand() % 0x10 - 8 value per segment. */
const int *Race_DetailWindow(void);

/* FFRace.exe 0x000a3b84, index 1 of the racer lateral-offset table 0x000a3b80;
   0x00011a1c clears it and Race_Init 0x00013aec sets it to -0x000a41f8[0]. */
float Race_PlayerX(void);

/* FFRace.exe 0x000a4308 + 4 * 0x000a770c, index 1 of the travelled-distance
   table; 0x00046754 divides it by 0x000832fc for the band fraction. */
float Race_Dist(void);

/* FFRace.exe Race_Init 0x00013aec tail: -(0x0008e32c - 0x0008e328). */
float Race_Curve(void);

/* FFRace.exe 0x000a76b4, the segment the window starts at. */
int Race_Segment(void);

/* FFRace.exe 0x000a76bc, the segment counter 0x000216c4 takes % 3. */
int Race_SegCounter(void);

/* FFRace.exe 0x000a7754, zeroed with the window tables by Race_Init 0x00013aec
   and set to 1, or to -10 for mode FF_RACE_ENDLESS, in its 0x000a779c != 0 tail;
   0x00046754 reads it as the segment count of FF_RACER_FIRST. */
int Race_ChaseSegment(void);

/* FFRace.exe 0x000a7754, stepped once per segment inside the racer loop of
   0x000542a0 .. 0x00055478 and stored back when that loop leaves a slot. */
void Race_SetChaseSegment(int seg);

/* FFRace.exe 0x0008e328, the full generated track; 0x0004f6bc reads entry
   0x000865d4 and the one after it for the wall tests. */
float Race_TrackCentre(int seg);

/* FFRace.exe 0x000832c0, the curve amplitude 0x0004f6bc scales the steering
   limits by. */
float Race_CurveScale(void);

/* FFRace.exe 0x000865d4, index FF_RACER_PLAYER of the racer segment table
   0x000865d0. */
int Race_PlayerSegment(void);
void Race_SetPlayerSegment(int seg);

/* FFRace.exe 0x000a77c4, recomputed by 0x0004f6bc and read by 0x00046754 as the
   POS field. */
void Race_UpdateRank(void);
int  Race_Rank(void);

/* FFRace.exe 0x000a77fc and 0x000a3b84, written by the physics pass in
   0x0004f6bc between 0x00052320 and 0x00053390. */
void Race_SetCurve(float curve);
void Race_SetPlayerX(float x);

/* FFRace.exe 0x00016b78, called three times by the screen 0x33 block of
   0x00039b1c and once by its 0x000500bc race-start gate. */
int Race_ThemeUnlocked(int race);

#endif /* FF_RACE_H */
