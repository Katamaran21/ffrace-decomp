#ifndef FF_PHYSICS_H
#define FF_PHYSICS_H

/* FFRace.exe .data 0x000832d0 initialiser 7; 0x0004f6bc halves it with
   (x - 1) >> 1 for the distance from the road centre to either wall. */
#define FF_ROAD_WIDTH 7

/* FFRace.exe .data 0x00083308 and 0x00083330 hold nine entries each, indexed
   by 0x000832f8; entry 0 is unused because Screen_Set 0x00014924 clamps the
   selection to 1 .. 0x000833a4. */
#define FF_SHIP_COUNT 9

/* FFRace.exe 0x0004f6bc reads 0x00090279, 0x00090281, 0x000a7788 and
   0x000a778c, one flag per key from 0x000833c4, 0x000833c8, 0x000833d8 and
   0x000833dc. */
#define FF_INPUT_RIGHT    0
#define FF_INPUT_LEFT     1
#define FF_INPUT_THROTTLE 2
#define FF_INPUT_BRAKE    3

void Physics_Reset(void);

void Physics_SetInput(int which, int down);
int  Physics_Input(int which);

/* FFRace.exe 0x000832f8, clamped against 0x000833a4. */
void Physics_SetShip(int ship);
int  Physics_Ship(void);
void Physics_SetShipsUnlocked(int count);

/* FFRace.exe 0x000833a4, read as a byte by the 0x00039b1c screen 6 block for
   its locked test and its two-line hint. */
int  Physics_ShipsUnlocked(void);

/* FFRace.exe 0x000a7674, latched from 0x000832f8 by Race_Init 0x00013aec and
   used in place of it by the racer loop of 0x000542a0; its 0x00083488 == 3 and
   0x000a7708 == 0 override needs globals this port does not model. */
void Physics_SetAiShip(int ship);
int  Physics_AiShip(void);

/* FFRace.exe .data 0x00083330 and 0x00083308, indexed by 0x000832f8 for the
   player and by 0x000a7674 for every other racer. */
int Physics_ShipTop(int ship);
int Physics_ShipAccel(int ship);

/* FFRace.exe 0x000a7728; 0x0004f6bc renders the frame and returns without
   touching the physics while it is negative. */
void Physics_SetCountdown(int value);
int  Physics_Countdown(void);

/* FFRace.exe 0x0004f6bc, the WM_TIMER physics pass from 0x00051690 to
   0x00053390. */
void Physics_Step(int dt_ms);

/* FFRace.exe 0x000a42c4 and 0x000a42dc, index 0x000a770c of the tables
   0x000a42c0 and 0x000a42d8. */
float Physics_Speed(void);
float Physics_Derived(void);

/* FFRace.exe 0x000a77b4. */
float Physics_Steer(void);

/* FFRace.exe 0x000a7670 and 0x000a77cc. */
int Physics_Damage(void);
int Physics_Finished(void);

/* FFRace.exe 0x00054154, the store of 1 into 0x000a77cc the player checkpoint
   walk of 0x00054020 reaches on segment 0x000832c4 + 1. */
void Physics_SetFinished(int finished);

/* FFRace.exe 0x000a7784 and 0x000a77a8, written when a wall ends a run in
   mode 0x000a779c == 1; the 0x000a779c == 2 block writes 0x000a7784 alone. */
int Physics_EndSegment(void);
int Physics_EndFlag(void);

/* FFRace.exe 0x000a7668, the 0x000a7754 snapshot the 0x000a779c == 2 block
   takes beside 0x000a7784; 0x00046754 picks its WON row over LOSE by comparing
   the two. */
int Physics_EndChase(void);

/* FFRace.exe 0x000a7734, set by the opponent hit test and consumed at the top
   of 0x0004f6bc. */
void Physics_NoteHit(int opponent_segment);

/* FFRace.exe 0x0004f6bc calls playSound(0x000a4db0, 0x000a4ab8, 0x10000000)
   on every wall and opponent hit. */
int Physics_TakeHitSound(void);

/* FFRace.exe 0x0004f6bc calls playSound(0x000a4db0, 0x000a4c20, 0x10000000)
   inside the gate that writes 0x000a77cc = 1 in both wall responses. */
int Physics_TakeEndSound(void);

#endif /* FF_PHYSICS_H */
