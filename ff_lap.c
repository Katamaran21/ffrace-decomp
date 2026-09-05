#include "ff_lap.h"

#include "ff_ingame.h"
#include "ff_physics.h"
#include "ff_race.h"

/* FFRace.exe 0x000902b8, 0x000902a0, 0x00090288 and 0x00085468, the tables the
   player walk of 0x00054020 and the racer loop of 0x000542a0 index by
   slot * 4. */
static int ff_lap_minutes[FF_RACER_SLOTS];
static int ff_lap_seconds[FF_RACER_SLOTS];
static int ff_lap_hundredths[FF_RACER_SLOTS];
static int ff_lap_order[FF_RACER_SLOTS];

static int ff_lap_index;   /* 0x000a77bc */
static int ff_lap_crossed; /* 0x000a7780 */
static int ff_lap_armed;   /* 0x000a77d4 */
static int ff_lap_won;     /* 0x000a7698 */

/* FFRace.exe Race_Init 0x00013aec clears 0x000a77bc, 0x000a7780, 0x000a7784
   and 0x000a7698, stores 6 into 0x0008546c .. 0x0008547c and leaves the three
   time tables untouched. */
void Lap_Reset(int armed)
{
    int slot;

    ff_lap_index   = 0;
    ff_lap_crossed = 0;
    ff_lap_won     = 0;
    ff_lap_armed   = (armed != 0);

    for (slot = FF_RACER_PLAYER; slot < FF_RACER_SLOTS; slot++)
        ff_lap_order[slot] = FF_LAP_NO_ORDER;
}

/* FFRace.exe 0x0004f6bc forms 0x000832c4 / 4 with add #3 / asr #2 at 0x00054050
   and 0x000832c4 / 8 with add #7 / asr #3 at 0x00054088. */
static int Lap_Quarter(void)
{
    return Race_Length() / 4;
}

static int Lap_Tolerance(void)
{
    return Race_Length() / 8;
}

/* FFRace.exe 0x0004f6bc at 0x000540e8 .. 0x00054124 takes the minutes from
   0x000a772c, the seconds from 0x000a7728 and __rt_udiv(10, 0x000a7654) capped
   by cmp #0x63 / movhs; the racer loop of 0x000542a0 reads the same three
   sources, its seconds through [sp, #0x2c] written at 0x000550c0. */
static void Lap_Capture(int slot)
{
    int hundredths = Ingame_SecondMs() / 10;

    if (hundredths >= 0x63)
        hundredths = 0x63;

    ff_lap_minutes[slot]    = Ingame_Minutes();
    ff_lap_seconds[slot]    = Physics_Countdown();
    ff_lap_hundredths[slot] = hundredths;
}

/* FFRace.exe 0x0004f6bc at 0x00054048 divides 0x000865d4 by 0x000832c4 / 4 and
   leaves for 0x000541a0 unless the remainder is 1 and 2 < 0x000865d4; 0x00054090
   and 0x0005409c then bracket 0x000a77bc * (0x000832c4 / 4) by +/- 0x000832c4 / 8
   and 0x0005409c jumps to the stores of 0x000540d8 while the segment is inside
   that window. */
void Lap_PlayerCheck(int seg)
{
    int quarter = Lap_Quarter();
    int expected, tol, order;

    if (quarter < 1)
        return;
    if (seg % quarter != 1 || seg <= 2)
        return;

    expected = ff_lap_index * quarter;
    tol      = Lap_Tolerance();

    if (seg <= expected - tol)
        return;

    /* FFRace.exe 0x000540a8 raises 0x000a77bc and clears 0x000a7780, and
       0x000540bc .. 0x000540cc store 6 into 0x00085470 .. 0x0008547c only. */
    if (seg > expected + tol) {
        int slot;

        ff_lap_index++;
        ff_lap_crossed = 0;
        for (slot = FF_RACER_FIRST; slot < FF_RACER_SLOTS; slot++)
            ff_lap_order[slot] = FF_LAP_NO_ORDER;
    }

    Lap_Capture(FF_RACER_PLAYER);

    /* FFRace.exe 0x00054128 stores 0x000a7780 into 0x0008546c through
       cmp #4 / movge #4, then 0x0005413c raises 0x000a7780. */
    order = ff_lap_crossed;
    if (order >= 4)
        order = 4;
    ff_lap_order[FF_RACER_PLAYER] = order;
    ff_lap_crossed++;

    /* FFRace.exe 0x00054148 compares the segment against 0x000832c4 + 1,
       0x00054154 stores 1 into 0x000a77cc, and 0x00054160 sets 0x000a7698 and
       raises 0x000a77d0 when 0x000a7708 is 0, both under 0x000a77d4 == 1 and
       0x0008546c == 0. */
    if (seg != Race_Length() + 1)
        return;

    Physics_SetFinished(1);

    if (ff_lap_armed && ff_lap_order[FF_RACER_PLAYER] == 0)
        ff_lap_won = 1;
}

/* FFRace.exe 0x0004f6bc at 0x00055228 .. 0x00055420 runs the body of 0x00054048
   without its 2 < segment guard, stores 6 into 0x0008546c .. 0x0008547c instead
   of 0x00085470 .. 0x0008547c, indexes each store with slot * 4 and stores
   0x000a7780 into 0x00085468 + slot * 4 with no cap. */
void Lap_OpponentCheck(int slot, int seg)
{
    int quarter = Lap_Quarter();
    int expected, tol;

    if (slot < FF_RACER_PLAYER || slot >= FF_RACER_SLOTS)
        return;
    if (quarter < 1)
        return;
    if (seg % quarter != 1)
        return;

    expected = ff_lap_index * quarter;
    tol      = Lap_Tolerance();

    if (seg <= expected - tol)
        return;

    if (seg > expected + tol) {
        int i;

        ff_lap_index++;
        ff_lap_crossed = 0;
        for (i = FF_RACER_PLAYER; i < FF_RACER_SLOTS; i++)
            ff_lap_order[i] = FF_LAP_NO_ORDER;
    }

    Lap_Capture(slot);

    ff_lap_order[slot] = ff_lap_crossed;
    ff_lap_crossed++;
}

void Lap_CapturePlayer(void)
{
    Lap_Capture(FF_RACER_PLAYER);
}

void Lap_NoteWin(void)
{
    if (ff_lap_armed)
        ff_lap_won = 1;
}

static int Lap_Slot(int slot)
{
    if (slot < 0)
        return 0;
    if (slot >= FF_RACER_SLOTS)
        return FF_RACER_SLOTS - 1;

    return slot;
}

int Lap_Minutes(int slot)
{
    return ff_lap_minutes[Lap_Slot(slot)];
}

int Lap_Seconds(int slot)
{
    return ff_lap_seconds[Lap_Slot(slot)];
}

int Lap_Hundredths(int slot)
{
    return ff_lap_hundredths[Lap_Slot(slot)];
}

int Lap_Order(int slot)
{
    return ff_lap_order[Lap_Slot(slot)];
}

int Lap_Index(void)
{
    return ff_lap_index;
}

int Lap_Crossed(void)
{
    return ff_lap_crossed;
}

int Lap_Won(void)
{
    return ff_lap_won;
}
