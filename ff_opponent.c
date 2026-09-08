#include <math.h>

#include "ff_opponent.h"

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_lap.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_race.h"
#include "ff_racer.h"
#include "ff_rand.h"
#include "ff_settings.h"
#include "ff_ship.h"

/* FFRace.exe .data 0x00083490 initialiser 0x3f4ccccd, the factor 0x00046754
   scales 0x000a3b98 by at 0x00048028 and again at 0x00048b28. */
#define FF_OPPONENT_SCALE 0.8f

/* FFRace.exe 0x00046754 at 0x00047f70 .. 0x00048014 and at 0x00048a10 ..
   0x00048aa4 both form the slot bound as
   ((0x000a779c == 2) + (0x000a779c == 0) * 2) * 3
   + (0x000a779c == 1 && 0x000a77f0 == 1) * 2 + (0x000a779c == 1),
   and 0x00048010 / 0x00048aa0 compare the slot against it. */
static int Opponent_SlotBound(void)
{
    int mode = Race_Mode();

    return ((mode == FF_RACE_ENDLESS_2) + (mode == FF_RACE_SCRIPTED) * 2) * 3
           + (mode == FF_RACE_ENDLESS && Race_Cops() == 1) * 2
           + (mode == FF_RACE_ENDLESS);
}

/* FFRace.exe 0x00046754 at 0x00048018 .. 0x00048234, ahead of the player blits
   of 0x00048300. */
void Opponent_Shadows(const ff_surface *dst)
{
    int bound = Opponent_SlotBound();
    int slot;

    for (slot = FF_RACER_PLAYER; slot < bound; slot++) {
        float s = Racer_Scale(slot) * FF_OPPONENT_SCALE;

        /* FFRace.exe 0x0004803c compares that product against 0x42800000 and
           0x00048068 stores 0x42a00000 into 0x000a3b98 + slot * 4, before
           0x0004806c gates on 0x000a3bd8 + slot * 4. */
        if (s > 64.0f)
            Racer_SetScale(slot, 80.0f);

        /* FFRace.exe 0x00048070 leaves for 0x00048230 when the row is 0. */
        if (Racer_Y(slot) == 0)
            continue;

        /* FFRace.exe 0x00048074 reloads 0x000a3b98 + slot * 4 and 0x0004808c
           takes it through __muls a second time. */
        s = Racer_Scale(slot) * FF_OPPONENT_SCALE;

        /* FFRace.exe 0x00048224 calls 0x0005a7c0 with 0x000a43f8, the x of
           0x000481f8, the y of 0x00048184, __stoi(s) from 0x00048100, the
           0x3fe2000000000000 product of 0x000480e8, *(0x000a4508) from
           0x000480a8, 0x14 three times from 0x000480a4 .. 0x000480c0, and
           *(0x000a4320 + slot * 4) << 6 from 0x000480a0. */
        Blit_Scale64Tint(dst,
                         (int)((double)Racer_X(slot) - (double)s * 0.5),
                         (int)((double)s * 0.234375 + (double)Racer_Y(slot)),
                         (int)s,
                         (int)((double)s * 0.5625),
                         AppAssets_Sprite(dst, FF_RES_SHADOW),
                         0x14, 0x14, 0x14,
                         Racer_Sprite(slot) * 0x40);
    }
}

/* FFRace.exe 0x00046754 at 0x00048aa8 .. 0x00048ca4, after the blink pass of
   0x00048758 and before the 39999 < 0x000a7670 block. */
void Opponent_Ships(const ff_surface *dst)
{
    int bound = Opponent_SlotBound();
    int slot;

    for (slot = FF_RACER_PLAYER; slot < bound; slot++) {
        int   ahead = Racer_SegCount(slot) - Race_SegCounter();
        float bob, s;

        /* FFRace.exe 0x00048ac4 holds the difference at 1 with cmp / movle. */
        if (ahead < 1)
            ahead = 1;

        /* FFRace.exe 0x00048ad8 calls __rt_sdiv with that difference as the
           divisor and 0x00083438 + *(0x00085450 + slot * 4) * 4 as the
           dividend, and 0x00048ae8 takes the quotient through __itos. */
        bob = (float)(Ship_Bob(Racer_Phase(slot)) / ahead);

        /* FFRace.exe 0x00048b10 leaves for 0x00048cac when the row is 0; the
           64.0f test of 0x0004803c does not repeat here. */
        if (Racer_Y(slot) == 0)
            continue;

        s = Racer_Scale(slot) * FF_OPPONENT_SCALE;

        /* FFRace.exe 0x00048ca4 calls 0x0005a78c with 0x000a43f8, the x of
           0x00048c78, the __subs of 0x00048bec, __stoi(s) from 0x00048bc4, the
           0x3fe2000000000000 product of 0x00048ba4, *(0x000a44e0 +
           *(0x000a4320 + slot * 4) * 4 + 4) from 0x00048b74, *(0x000a7800)
           from 0x00048b4c, and (*(0x00090268 + slot) + *(0x00090270 + slot) *
           2) << 6 from 0x00048b40 .. 0x00048b58.

           FFRace.exe 0x0004f6bc writes those two byte arrays in the racer loop
           of 0x000542a0: 0x00054330 and 0x00054344 clear 0x00090268 + slot and
           0x00090270 + slot, then 0x00054404 and 0x0005447c store 1 from the
           0x000543f8 and 0x0005446c curvature tests. */
        Blit_Scale64(dst,
                     (int)((double)Racer_X(slot) - (double)s * 0.5),
                     (int)((float)Racer_Y(slot) - bob),
                     (int)s,
                     (int)((double)s * 0.5625),
                     AppAssets_Sprite(dst, Ship_SpriteRes(Racer_Sprite(slot))),
                     Menu_Key(), Racer_Lean(slot) * 0x40);
    }
}

/* FFRace.exe 0x0004f6bc at 0x000542a0 clears 0x00090268 + slot and
   0x00090270 + slot, forms (0x0008e330)[0x000865d0[slot]] + 0x000a77fc -
   (0x0008e32c)[0x000865d0[slot]], and sets the first byte when that is below
   -0x000832c0 * 0.26 and the second when it is above 0x000832c0 * 0.26; its
   0x00054298 slot != 1 guard never fails because the loop starts at 2. */
static void Opponent_Lean(int slot)
{
    int   seg   = Racer_Seg(slot);
    float scale = Race_CurveScale();
    float delta = (Race_TrackCentre(seg + 2) + Race_Curve()) -
                  Race_TrackCentre(seg + 1);

    Racer_SetLean(slot,
                  (double)delta < (double)(-scale) * 0.26,
                  (double)delta > (double)scale * 0.26);
}

/* FFRace.exe 0x0004f6bc runs three identical bodies from 0x000542f8, guarded by
   0x00083384 == 3, == 2 and == 1; each takes its own rand() divisor and its own
   pair of constants, and no other difficulty reaches any of them. */
static void Opponent_Throttle(int slot)
{
    int   ai   = Physics_AiShip();
    int   diff = Settings_Difficulty();
    int   div, sub, off, top_raw;
    float gain;

    switch (diff) {
    case 3:  div = 3; sub = 0x82; off = 0x17; break;
    case 2:  div = 4; sub = 0x84; off = 0x16; break;
    case 1:  div = 5; sub = 0x85; off = 0x15; break;
    default: return;
    }

    top_raw = Physics_ShipTop(ai) + diff * 4 - 9;

    if ((double)(Racer_Derived(slot) * 10.0f) <
        (double)(top_raw * (sub - Ff_Rand() % div)) * (1.0 / 55.0)) {
        /* FFRace.exe 0x0004f6bc subtracts *(0x00090280 + slot) and
           *(0x00090278 + slot) scaled by 0x000a42d8 + slot * 4 here; that pair
           is the player key state 0x0004f6bc writes only at index 1, so both
           terms are 0 for every slot this loop walks. */
        gain = (float)((ai - Ff_Rand() % 3) + slot + off);

        Racer_SetSpeed(slot,
                       (float)((double)(gain * (float)Physics_ShipAccel(ai)) *
                               (1.0 / 60.0) + (double)Racer_Speed(slot)));
    }
}

/* FFRace.exe 0x0004f6bc drags every slot with no throttle test, then subtracts
   0x42480000 when 0x000832c4 < 0x000865d0[slot] or 0x000a77cc == 1 while
   0x000a779c == 2, and clamps at 0. */
static void Opponent_Drag(int slot)
{
    int   ai    = Physics_AiShip();
    int   diff  = Settings_Difficulty();
    float speed = Racer_Speed(slot);

    speed = (float)((double)speed -
                    (double)(Physics_ShipTop(ai) + diff * 4 - 9) * 0.2);

    if (Race_Length() < Racer_Seg(slot) ||
        (Physics_Finished() == 1 && Race_Mode() == FF_RACE_ENDLESS_2))
        speed -= 50.0f;

    if (speed < 0.0f)
        speed = 0.0f;

    Racer_SetSpeed(slot, speed);
}

/* FFRace.exe 0x0004f6bc scales 0x000a42c0 + slot * 4 by 0x3fe6666666666666 plus
   0x3fbeb851eb851eb8 for 0x000a779c == 2 when rand() % (0x3c - t) is 1, where t
   is 0x000a7728 / 2 capped at 0x1e and 0 outside that mode. */
static void Opponent_Mistake(int slot)
{
    int mode = Race_Mode();
    int r    = Ff_Rand();
    int cd   = Physics_Countdown();
    int t;

    if (cd < 0)
        cd++;
    t = (mode == FF_RACE_ENDLESS_2) * (cd >> 1);
    if (t >= 0x1e)
        t = 0x1e;

    if (r % (0x3c - t) == 1 &&
        (Race_SegCounter() < Racer_SegCount(slot) ||
         mode == FF_RACE_ENDLESS_2))
        Racer_SetSpeed(slot,
                       (float)(((double)(mode == FF_RACE_ENDLESS_2) * 0.12 +
                                0.7) * (double)Racer_Speed(slot)));
}

/* FFRace.exe 0x0004f6bc adds __itos((2 - k) * 0x00083384 * 100) when rand() %
   ((0x000a779c == 2) * -200 + 300) is 1, 0x000a76bc is below 0x000832c4 *
   0x3fe5555555555555 or the mode is 2, and 0x000a7728 is above 0; its
   0x00055054 jump leaves the block when (0x00083384 + 2) * 10 is at most
   0x000a7728. */
static void Opponent_Boost(int slot)
{
    int mode = Race_Mode();
    int diff = Settings_Difficulty();
    int cd   = Physics_Countdown();
    int k    = 0;

    if (Ff_Rand() % ((mode == FF_RACE_ENDLESS_2) * -200 + 300) != 1)
        return;

    if (!((double)Race_SegCounter() <
          (double)Race_Length() * 0.6666666666666666) &&
        mode != FF_RACE_ENDLESS_2)
        return;

    if (cd <= 0)
        return;

    if (mode == FF_RACE_ENDLESS_2) {
        if ((diff + 2) * 10 <= cd)
            return;
        k = 1;
    }

    Racer_SetSpeed(slot, (float)((2 - k) * diff * 100) + Racer_Speed(slot));
}

/* FFRace.exe 0x0004f6bc raises 0x000a4308 + slot * 4 by the __muls chain
   Ordinal_2052(0x000a7660) * 0x3a83126f * (0x000a42d8 + slot * 4) * 0x41a00000,
   stores it, then walks whole 0x000832fc steps while it stays at or above that
   length, raising 0x000865d0 + slot * 4 each step and, outside mode 0, rebasing
   it on 0x000865d4 while it raises 0x000a7754. */
static void Opponent_Advance(int slot, int dt_ms)
{
    int   mode   = Race_Mode();
    int   count  = Racer_SegCount(slot);
    int   chase  = Race_ChaseSegment();
    int   offset = chase - Race_SegCounter();
    float dist   = (float)dt_ms * 0.001f * Racer_Derived(slot) * 20.0f +
                   Racer_Dist(slot);

    Racer_SetDist(slot, dist);

    if (dist < (float)FF_SEGMENT_LEN)
        return;

    do {
        count++;
        dist -= (float)FF_SEGMENT_LEN;
        Racer_SetSeg(slot, Racer_Seg(slot) + 1);
        Lap_OpponentCheck(slot, Racer_Seg(slot));

        if (mode != FF_RACE_SCRIPTED) {
            chase++;
            offset++;
            Racer_SetSeg(slot, offset + Race_PlayerSegment());
        }
    } while (dist >= (float)FF_SEGMENT_LEN);

    Race_SetChaseSegment(chase);
    Racer_SetDist(slot, dist);
    Racer_SetSegCount(slot, count);
}

/* FFRace.exe 0x0004f6bc at 0x000542a0 .. 0x00055478 walks slot 2 upwards against
   the bound 0x00047f70 forms, and its 0x00055054 takes 0x000a42d8 + slot * 4
   from Ordinal_1060((0x000a42c0 + slot * 4) * 0x3d888889). */
void Opponent_Step(int dt_ms)
{
    int bound = Opponent_SlotBound();
    int slot;

    for (slot = FF_RACER_FIRST; slot < bound; slot++) {
        Opponent_Lean(slot);
        Opponent_Throttle(slot);
        Opponent_Drag(slot);
        Opponent_Mistake(slot);
        Opponent_Boost(slot);

        Racer_SetDerived(slot,
                         (float)sqrt((double)(Racer_Speed(slot) *
                                              (1.0f / 15.0f))));

        Opponent_Advance(slot, dt_ms);
    }
}
