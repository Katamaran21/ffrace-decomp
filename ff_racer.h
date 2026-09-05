#ifndef FF_RACER_H
#define FF_RACER_H

/* FFRace.exe Race_Init 0x00013aec takes __rt_sdiv(0x17, rand()).remainder into
   0x000a77ec before it reaches its srand, so the value comes off the seed the
   previous race left behind. */
void Racer_PickBase(void);

/* FFRace.exe Race_Init 0x00013aec at 0x00013e64 seeds 0x00085454 .. 0x00085464
   from __rt_sdiv(0x14, rand()).remainder and 0x000a4324 .. 0x000a4334 through
   0x00016e9c, then at 0x000141d0 forces all five ship slots to 1 while
   0x000a779c is 1; 0x00014390 writes 0.3f into 0x000a430c .. 0x000a431c and
   0x000144b0 overwrites 0x000a4318 and 0x000a431c with 0x000832fc * 0.625. */
void Racer_Seed(int mode);

/* FFRace.exe 0x000865d0, the per-racer segment index; 0x0004f6bc compares
   0x000865d4 + 1 against slots 2 .. 5 for the POS field. */
int  Racer_Seg(int slot);
void Racer_SetSeg(int slot, int seg);

/* FFRace.exe 0x000a42f0, the per-racer segment count 0x00046754 subtracts
   0x000a76bc from for the sprite-animation divisor. */
int  Racer_SegCount(int slot);
void Racer_SetSegCount(int slot, int count);

/* FFRace.exe 0x000a4308, the travelled distance within the current segment;
   Race_Init 0x00013aec starts every slot at 0.3f. */
float Racer_Dist(int slot);
void  Racer_SetDist(int slot, float dist);

/* FFRace.exe 0x000865e8, the lateral offset across the road; Race_Init
   0x00013aec writes 0, +1.5, -1.5, +1.3, -1.3 into 0x000865ec .. 0x000865fc. */
float Racer_Lat(int slot);
void  Racer_SetLat(int slot, float lat);

/* FFRace.exe 0x000a4320, the 0-based ship sprite 0x00016e9c chose. */
int Racer_Sprite(int slot);

/* FFRace.exe 0x00085450, the rand() % 0x14 bob phase 0x00046754 indexes
   0x00083438 with. */
int Racer_Phase(int slot);

/* FFRace.exe 0x000a3bd8, the projected screen row 0x00046754 computes ahead of
   the road pass; 0 marks the racer as not drawn. */
int  Racer_Y(int slot);
void Racer_SetY(int slot, int y);

/* FFRace.exe 0x000a3c18 and 0x000a3b98, latched by the road drawer 0x000216c4
   at 0x00023824 on the row that matches 0x000a3bd8. */
int   Racer_X(int slot);
void  Racer_SetX(int slot, int x);
float Racer_Scale(int slot);
void  Racer_SetScale(int slot, float scale);

/* FFRace.exe 0x000a42c0 and 0x000a42d8, both indexed by 0x000a770c: the raw
   speed the throttle integrates and the sqrt of it the distance step uses. */
float Racer_Speed(int slot);
void  Racer_SetSpeed(int slot, float speed);
float Racer_Derived(int slot);
void  Racer_SetDerived(int slot, float derived);

/* FFRace.exe 0x00090268 and 0x00090270, the two byte arrays the racer loop of
   0x000542a0 derives from track curvature; 0x00048b40 .. 0x00048b58 reads them
   back as *(0x00090268 + slot) + *(0x00090270 + slot) * 2. */
int  Racer_Lean(int slot);
void Racer_SetLean(int slot, int left, int right);

#endif /* FF_RACER_H */
