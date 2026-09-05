#ifndef FF_LAP_H
#define FF_LAP_H

/* FFRace.exe Race_Init 0x00013aec writes 6 into 0x0008546c .. 0x0008547c and
   0x00046754 skips a standings row while 0x00085468 + slot * 4 holds it. */
#define FF_LAP_NO_ORDER 6

/* FFRace.exe Race_Init 0x00013aec clears 0x000a77bc, 0x000a7780 and 0x000a7698
   and takes 0x000a77d4 from 0x00083488 == 3. */
void Lap_Reset(int armed);

/* FFRace.exe 0x0004f6bc at 0x00054048, inside the player walk of 0x00054020,
   and at 0x00055228 .. 0x00055420 inside the racer loop of 0x000542a0; both
   gate on 0x000865d0 + slot * 4 leaving remainder 1 against 0x000832c4 / 4. */
void Lap_PlayerCheck(int seg);
void Lap_OpponentCheck(int slot, int seg);

/* FFRace.exe 0x0004f6bc, the 0x000a779c == 2 block between 0x000526f0 and
   0x000532a8, whose 0x000902bc, 0x000902a4 and 0x0009028c stores repeat the
   three of 0x000540e8 and whose 0x000a7698 store repeats the one of
   0x00054160. */
void Lap_CapturePlayer(void);
void Lap_NoteWin(void);

/* FFRace.exe 0x000902b8, 0x000902a0, 0x00090288 and 0x00085468, the four
   tables both walks index by slot * 4 and 0x00046754 reads for the checkpoint
   overlay and the results list. */
int Lap_Minutes(int slot);
int Lap_Seconds(int slot);
int Lap_Hundredths(int slot);
int Lap_Order(int slot);

/* FFRace.exe 0x000a77bc, the checkpoint index both walks share, and
   0x000a7780, the crossing counter the inner block clears. */
int Lap_Index(void);
int Lap_Crossed(void);

/* FFRace.exe 0x000a7698, set at 0x00054160 and by the 0x000a779c == 2 block of
   0x0004f6bc, both gated on 0x000a77d4 == 1. */
int Lap_Won(void);

#endif /* FF_LAP_H */
