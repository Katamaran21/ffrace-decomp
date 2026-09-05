#include "ff_racer.h"

#include "ff_consts.h"
#include "ff_physics.h"
#include "ff_race.h"
#include "ff_rand.h"

/* FFRace.exe 0x000865d0, 0x000a42f0, 0x000a4308, 0x000865e8, 0x000a4320 and
   0x00085450, the six per-racer tables Race_Init 0x00013aec seeds and 0x0004f6bc
   steps; slot FF_RACER_PLAYER is the player. */
static int   ff_racer_seg[FF_RACER_SLOTS];
static int   ff_racer_segcount[FF_RACER_SLOTS];
static float ff_racer_dist[FF_RACER_SLOTS];
static float ff_racer_lat[FF_RACER_SLOTS];
static int   ff_racer_sprite[FF_RACER_SLOTS];
static int   ff_racer_phase[FF_RACER_SLOTS];

/* FFRace.exe 0x00046754 projects into 0x000a3bd8 at 0x00048b8c and the road
   drawer 0x000216c4 latches 0x000a3c18 and 0x000a3b98 at 0x00023824. */
static int   ff_racer_y[FF_RACER_SLOTS];
static int   ff_racer_x[FF_RACER_SLOTS];
static float ff_racer_scale[FF_RACER_SLOTS];

/* FFRace.exe 0x000a42c0 and 0x000a42d8, read and written as
   0x000a42c0 + 0x000a770c * 4 by the player pass at 0x00053430 and by the racer
   loop of 0x000542a0. */
static float ff_racer_speed[FF_RACER_SLOTS];
static float ff_racer_derived[FF_RACER_SLOTS];

/* FFRace.exe 0x00090268 and 0x00090270, cleared at 0x00054330 and 0x00054344
   and set at 0x00054404 and 0x0005447c. */
static char  ff_racer_lean_l[FF_RACER_SLOTS];
static char  ff_racer_lean_r[FF_RACER_SLOTS];

static int   ff_racer_base; /* 0x000a77ec */

/* FFRace.exe Race_Init 0x00013aec at 0x00014240 walks 0x000a4324 .. 0x000a4334
   through 0x00016e9c twice and takes __rt_sdiv(6, v + (v == 0x000832f8) - 1). */
static int Opponent_Ship(int slot)
{
    /* FFRace.exe 0x00016e9c, the whole if-chain: 0x24 and 0x1f fall to its
       0x00016fdc return 2, the nine skipped values to its 0x00016fe8 return 6,
       and anything above 0x24 returns its own argument. */
    static const int table[0x25] = {
        1, 2, 3, 4, 5, 6, 1, 2, 6, 3, 5, 6, 1, 2, 3, 4, 6, 1, 6,
        2, 3, 4, 5, 6, 1, 2, 6, 3, 5, 6, 1, 2, 3, 4, 5, 6, 2
    };
    int v = (slot < 0 || slot > 0x24) ? slot : table[slot];

    return (v + (v == Physics_Ship()) - 1) % FF_SHIP_SPRITES;
}

void Racer_PickBase(void)
{
    ff_racer_base = Ff_Rand() % 0x17;
}

void Racer_Seed(int mode)
{
    int i;

    for (i = FF_RACER_PLAYER; i < FF_RACER_SLOTS; i++)
        ff_racer_phase[i] = Ff_Rand() % 0x14;

    for (i = FF_RACER_PLAYER; i < FF_RACER_SLOTS; i++)
        ff_racer_sprite[i] = mode == FF_RACE_ENDLESS
                                 ? 1 : Opponent_Ship(ff_racer_base + i - 1);

    for (i = FF_RACER_PLAYER; i < FF_RACER_SLOTS; i++) {
        ff_racer_dist[i] = 0.3f;
        ff_racer_x[i]    = 0;
        ff_racer_y[i]    = 0;
    }
    for (i = FF_RACER_PLAYER + 1; i < FF_RACER_SLOTS; i++) {
        ff_racer_seg[i]      = 1;
        ff_racer_segcount[i] = 1;
    }
    ff_racer_seg[FF_RACER_PLAYER]      = 0;
    ff_racer_segcount[FF_RACER_PLAYER] = 0;

    /* FFRace.exe 0x00014434 .. 0x00014450 writes 0x000865ec .. 0x000865fc. */
    ff_racer_lat[FF_RACER_PLAYER] = 0.0f;
    ff_racer_lat[2] =  1.5f;
    ff_racer_lat[3] = -1.5f;
    ff_racer_lat[4] =  1.3f;
    ff_racer_lat[5] = -1.3f;

    ff_racer_dist[4] = (float)((double)FF_SEGMENT_LEN * 0.625);
    ff_racer_dist[5] = ff_racer_dist[4];
}

int Racer_Seg(int slot)
{
    return ff_racer_seg[slot];
}

void Racer_SetSeg(int slot, int seg)
{
    ff_racer_seg[slot] = seg;
}

int Racer_SegCount(int slot)
{
    return ff_racer_segcount[slot];
}

void Racer_SetSegCount(int slot, int count)
{
    ff_racer_segcount[slot] = count;
}

float Racer_Dist(int slot)
{
    return ff_racer_dist[slot];
}

void Racer_SetDist(int slot, float dist)
{
    ff_racer_dist[slot] = dist;
}

float Racer_Lat(int slot)
{
    return ff_racer_lat[slot];
}

void Racer_SetLat(int slot, float lat)
{
    ff_racer_lat[slot] = lat;
}

int Racer_Sprite(int slot)
{
    return ff_racer_sprite[slot];
}

int Racer_Phase(int slot)
{
    return ff_racer_phase[slot];
}

int Racer_Y(int slot)
{
    return ff_racer_y[slot];
}

void Racer_SetY(int slot, int y)
{
    ff_racer_y[slot] = y;
}

int Racer_X(int slot)
{
    return ff_racer_x[slot];
}

void Racer_SetX(int slot, int x)
{
    ff_racer_x[slot] = x;
}

float Racer_Scale(int slot)
{
    return ff_racer_scale[slot];
}

void Racer_SetScale(int slot, float scale)
{
    ff_racer_scale[slot] = scale;
}

float Racer_Speed(int slot)
{
    return ff_racer_speed[slot];
}

void Racer_SetSpeed(int slot, float speed)
{
    ff_racer_speed[slot] = speed;
}

float Racer_Derived(int slot)
{
    return ff_racer_derived[slot];
}

void Racer_SetDerived(int slot, float derived)
{
    ff_racer_derived[slot] = derived;
}

/* FFRace.exe 0x00048b40 .. 0x00048b58 loads both bytes and forms
   *(0x00090268 + slot) + *(0x00090270 + slot) * 2. */
int Racer_Lean(int slot)
{
    return ff_racer_lean_l[slot] + ff_racer_lean_r[slot] * 2;
}

void Racer_SetLean(int slot, int left, int right)
{
    ff_racer_lean_l[slot] = (char)left;
    ff_racer_lean_r[slot] = (char)right;
}
