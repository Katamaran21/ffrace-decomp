#include "ff_race.h"

#include "ff_consts.h"
#include "ff_lap.h"
#include "ff_physics.h"
#include "ff_racer.h"
#include "ff_rand.h"
#include "ff_screen.h"
#include "ff_settings.h"

/* FFRace.exe Track_Generate 0x00014a20 LoadBitmapW, 0x000a7630 == 0 branch,
   indexed by theme 0x000833b8. */
static const int ff_sky_res[FF_THEME_COUNT] = {
    0x13d, 0x113, 0x10b, 0x0e7, 0x13b, 0x123, 0x13c, 0x113, 0x13e, 0x0e7
};

/* FFRace.exe Track_Generate 0x00014a20 LoadBitmapW, 0x000a7630 != 0 branch. */
static const int ff_sky_res_small[FF_THEME_COUNT] = {
    0x151, 0x14e, 0x156, 0x154, 0x153, 0x14f, 0x150, 0x14e, 0x152, 0x154
};

/* FFRace.exe Track_Generate 0x00014a20 writes 0x000a77b8 per theme. */
static const int ff_sky_blend[FF_THEME_COUNT] = {
    0xff, 0, 0, 0xff, 0, 0xff, 0xff, 0xff, 0, 0xff
};

/* FFRace.exe 0x0008e328, 0x0008a4a8, 0x0008c3e8, 0x00086628. */
static float ff_centre[FF_TRACK_SEGMENTS];
static float ff_scenery[FF_TRACK_SEGMENTS];
static int   ff_extra[FF_TRACK_SEGMENTS];
static int   ff_detail[FF_TRACK_SEGMENTS];

/* FFRace.exe 0x000a41f8, 0x000a4130, 0x000a4068, 0x000a3ed8. */
static float ff_win_centre[FF_WINDOW_SEGMENTS];
static int   ff_win_scenery[FF_WINDOW_SEGMENTS];
static int   ff_win_extra[FF_WINDOW_SEGMENTS];
static int   ff_win_detail[FF_WINDOW_SEGMENTS];

static int   ff_mode;          /* 0x000a779c */
static int   ff_cops;          /* 0x000a77f0 */
static int   ff_theme;         /* 0x000833b8 */
static int   ff_sky;           /* 0x000a77c8 */
static int   ff_length;        /* 0x000832c4 */
static float ff_curve_scale;   /* 0x000832c0 */
static float ff_accum;         /* 0x000a76c4 */
static float ff_rate;          /* 0x000a76e8 */
static float ff_run;           /* 0x000a7638 */
static int   ff_scenery_state; /* 0x000a76c8 */
static int   ff_segment;       /* 0x000a76b4 */
static int   ff_seg_counter;   /* 0x000a76bc */
static int   ff_rank;          /* 0x000a77c4 */
static int   ff_chase_seg;     /* 0x000a7754 */
static float ff_player_x;      /* 0x000a3b80[1] */
static float ff_curve;         /* 0x000a77fc */

/* FFRace.exe Track_Generate 0x00014a20 at 0x00014b6c and Race_Init 0x00013aec
   at 0x00014594 store over one loaded copy of 0x000a76c8. */
static void Scenery_Step(void)
{
    int r   = Ff_Rand() % 4;
    int cur = ff_scenery_state;

    if (r == 0 && cur != 5)
        ff_scenery_state = 2;
    if (r == 0 && cur == 5)
        ff_scenery_state = 1;
    if (r == 1)
        ff_scenery_state = 1;
    if (r == 2 && cur != 5)
        ff_scenery_state = 0;
    if (r == 3 && cur == 1)
        ff_scenery_state = 5;
}

/* FFRace.exe 0x00016b78 maps the career race index 0x000a77d0 to the highest
   theme the generator may draw. */
int Race_ThemeUnlocked(int race)
{
    if (race == 0)                    return 0;
    if (race >= 1    && race <= 3)    return 1;
    if (race >= 4    && race <= 6)    return 2;
    if (race >= 7    && race <= 0xb)  return 3;
    if (race >= 0xc  && race <= 0x10) return 4;
    if (race >= 0x11 && race <= 0x13) return 5;
    if (race >= 0x14 && race <= 0x16) return 6;
    if (race >= 0x17 && race <= 0x19) return 7;
    if (race >= 0x1a && race <= 0x1d) return 8;
    if (race >= 0x1e && race <= 0x24) return 9;
    return race;
}

/* FFRace.exe Track_Generate 0x00014a20 and 0x0004f6bc run the same curve body;
   only 0x0004f6bc bounds 0x000a76c4 by +/-0x000832c0 afterwards. */
static void Curve_Step(int bound)
{
    int n = Ff_Rand();

    if (ff_run < 0.0f && (n % 0xf == 0 || ff_run < -27.0f)) {
        int mag = Ff_Rand();
        int sgn = Ff_Rand();
        int amp = (sgn % 2 * 2 - 1) * (mag % 0x14 + 10);

        ff_run  = (float)(Ff_Rand() % 0xc + 3);
        ff_rate = (float)(((double)amp * 0.05) / (double)ff_run) * ff_curve_scale;
    }
    ff_run -= 1.0f;
    if (ff_run < 0.0f)
        ff_rate = 0.0f;
    ff_accum += ff_rate;

    if (bound) {
        if (ff_accum > ff_curve_scale)
            ff_accum = ff_curve_scale;
        if (ff_accum < -ff_curve_scale)
            ff_accum = -ff_curve_scale;
    }
}

/* FFRace.exe Race_Init 0x00013aec and 0x0004f6bc both shift 80 entries of
   0x0008e328, 0x0008a4a8 and 0x00086628 down by one, clear 80 entries of
   0x0008c3e8, then write index 80 of all four. */
static void Track_Shift(void)
{
    int j;

    for (j = 0; j < 80; j++)
        ff_extra[j] = 0;
    for (j = 0; j < 80; j++) {
        ff_centre[j]  = ff_centre[j + 1];
        ff_scenery[j] = ff_scenery[j + 1];
        ff_detail[j]  = ff_detail[j + 1];
    }
    ff_centre[80] += ff_accum;
    ff_scenery[80] = (float)ff_scenery_state;
    ff_extra[80]   = 0;
    ff_detail[80]  = Ff_Rand() % 0x10 - 8;
}

/* FFRace.exe Race_Init 0x00013aec and 0x0004f6bc copy 200 bytes from
   0x00086628 + i, 0x0008e328 + i and 0x0008a4a8 + i into 0x000a3ed8,
   0x000a41f8 and 0x000a4130, the last through __stoi. */
static void Window_Refill(int from)
{
    int i;

    for (i = 0; i < FF_WINDOW_SEGMENTS; i++) {
        ff_win_detail[i]  = ff_detail[from + i];
        ff_win_centre[i]  = ff_centre[from + i];
        ff_win_scenery[i] = (int)ff_scenery[from + i];
    }
}

/* FFRace.exe Track_Generate 0x00014a20. */
static void Track_Generate(int theme, int sky)
{
    int limit, i;

    ff_run   = 0.0f;
    ff_accum = 0.0f;
    ff_rate  = 0.0f;

    if (theme == FF_THEME_RANDOM)
        theme = Ff_Rand() % (Race_ThemeUnlocked(Screen_CareerRace()) + 1);
    ff_theme = theme;

    if (sky == FF_SKY_RANDOM)
        sky = Ff_Rand() % 3;
    ff_sky = sky;

    limit = (ff_mode == FF_RACE_SCRIPTED) ? ff_length + 200 : 0x96;
    if (limit > FF_TRACK_SEGMENTS)
        limit = FF_TRACK_SEGMENTS;

    for (i = 0; i < limit; i++) {
        if (i % 0x14 == 0)
            Scenery_Step();

        Curve_Step(0);

        /* FFRace.exe 0x00014a20 reads 0x0008e324, the last slot of the
           always-zero 0x0008c3e8 table; .data raw size 0x1800 ends at
           0x00084800, so every slot above it starts zeroed. */
        ff_centre[i]  = (i == 0 ? 0.0f : ff_centre[i - 1]) + ff_accum;
        ff_scenery[i] = (float)ff_scenery_state;
        ff_extra[i]   = 0;
        ff_detail[i]  = Ff_Rand() % 0x10 - 8;
    }

    Window_Refill(0);
    for (i = 0; i < FF_WINDOW_SEGMENTS; i++)
        ff_win_extra[i] = ff_extra[i];
}

/* FFRace.exe Race_Init 0x00013aec, the 0 < 0x000a779c block entered at
   0x00014550: 100 shift steps over an 80-entry window that parks the player at
   0x000a76b4 == 0x1e. */
static void Track_Prime(void)
{
    int i;

    for (i = 0; i < 100; i++)
        ff_centre[i] = 0.0f;
    ff_rate  = 0.0f;
    ff_accum = 0.0f;
    ff_curve = 0.0f;
    ff_run   = 0.0f;

    for (i = 0; i < 100; i++) {
        ff_seg_counter++;
        ff_segment    = FF_ENDLESS_SEGMENT;
        Racer_SetSeg(FF_RACER_PLAYER, FF_ENDLESS_SEGMENT);

        if (ff_seg_counter % 0xf == 0)
            Scenery_Step();

        Track_Shift();
    }

    Window_Refill(ff_segment);
    ff_seg_counter = 0;
    ff_player_x    = -ff_win_centre[0];
}

/* FFRace.exe Race_Init 0x00013aec. */
void Race_Init(int mode, int seed, int length, int theme, int sky)
{
    int i;

    Racer_PickBase();

    /* FFRace.exe 0x00013aec: 0x000a7674 = 0x000832f8. */
    Physics_SetAiShip(Physics_Ship());

    /* FFRace.exe 0x00013aec: 0x000832c0 = (0x00083384 - 1) * 0.19 + 0.95. */
    ff_curve_scale = (float)((double)(Settings_Difficulty() - 1) * 0.19 + 0.95);

    if (seed != FF_SEED_NONE)
        Ff_Srand((unsigned int)seed);

    ff_length = length;
    if (mode == FF_RACE_ENDLESS)
        ff_length = FF_TRACK_ENDLESS_LEN;
    ff_mode = mode;

    ff_scenery_state = 0;
    ff_segment       = 0;
    ff_seg_counter   = 0;
    ff_player_x      = 0.0f;
    ff_curve         = 0.0f;
    ff_chase_seg     = 0;
    for (i = 0; i < FF_WINDOW_SEGMENTS; i++) {
        ff_win_centre[i]  = 0.0f;
        ff_win_scenery[i] = 0;
        ff_win_extra[i]   = 0;
        ff_win_detail[i]  = 0;
    }

    /* FFRace.exe Race_Init 0x00013aec: 0x000a77d4 = (0x00083488 == 3). */
    Lap_Reset(Screen_Id() == FF_SCREEN_CAREER_MENU);

    Racer_Seed(ff_mode);

    Track_Generate(theme, sky);

    if (ff_mode > 0)
        Track_Prime();

    /* FFRace.exe Race_Init 0x00013aec, its 0x000a779c != 0 tail: 0x000a7754 and
       0x000a42f8 take 1, or -10 when 0x000a779c is 1, and 0x000865d8 takes 0x1f,
       or 0x15 when 0x000a779c is 1. */
    if (ff_mode != 0) {
        int seg = 0x1f;

        ff_chase_seg = 1;
        if (ff_mode == FF_RACE_ENDLESS) {
            ff_chase_seg = -10;
            seg          = 0x15;
        }
        Racer_SetSeg(FF_RACER_FIRST, seg);
        Racer_SetSegCount(FF_RACER_FIRST, ff_chase_seg);
    }

    ff_curve = -(ff_centre[1] - ff_centre[0]);
}

/* FFRace.exe 0x0004f6bc, the loop it enters while 0x000a4308 + 4 * 0x000a770c
   is not below 0x000832fc. */
void Race_Advance(float distance)
{
    Racer_SetDist(FF_RACER_PLAYER, Racer_Dist(FF_RACER_PLAYER) + distance);

    while (Racer_Dist(FF_RACER_PLAYER) >= (float)FF_SEGMENT_LEN) {
        ff_player_x += ff_curve;
        Racer_SetDist(FF_RACER_PLAYER,
                      Racer_Dist(FF_RACER_PLAYER) - (float)FF_SEGMENT_LEN);
        ff_seg_counter++;
        ff_segment++;
        Racer_SetSeg(FF_RACER_PLAYER, Racer_Seg(FF_RACER_PLAYER) + 1);

        if (ff_mode > 0) {
            ff_segment    = FF_ENDLESS_SEGMENT;
            Racer_SetSeg(FF_RACER_PLAYER, FF_ENDLESS_SEGMENT);

            if (ff_seg_counter % 0xf == 0)
                Scenery_Step();

            Curve_Step(1);
            Track_Shift();
            Window_Refill(ff_segment);
        }

        Lap_PlayerCheck(Racer_Seg(FF_RACER_PLAYER));

        if (ff_mode != FF_RACE_ENDLESS)
            Window_Refill(ff_segment);
    }
}

int Race_Mode(void)
{
    return ff_mode;
}

int Race_Cops(void)
{
    return ff_cops;
}

int Race_Theme(void)
{
    return ff_theme;
}

int Race_Sky(void)
{
    return ff_sky;
}

int Race_Length(void)
{
    return ff_length;
}

int Race_SkyResource(void)
{
    if (ff_theme < 0 || ff_theme >= FF_THEME_COUNT)
        return 0;
    if (Screen_Layout() == FF_LAYOUT_240x320)
        return ff_sky_res[ff_theme];
    return ff_sky_res_small[ff_theme];
}

int Race_SkyBlend(void)
{
    if (ff_theme < 0 || ff_theme >= FF_THEME_COUNT)
        return 0;
    return ff_sky_blend[ff_theme];
}

const float *Race_CentreWindow(void)
{
    return ff_win_centre;
}

const int *Race_SceneryWindow(void)
{
    return ff_win_scenery;
}

const int *Race_DetailWindow(void)
{
    return ff_win_detail;
}

float Race_PlayerX(void)
{
    return ff_player_x;
}

float Race_Dist(void)
{
    return Racer_Dist(FF_RACER_PLAYER);
}

float Race_Curve(void)
{
    return ff_curve;
}

int Race_Segment(void)
{
    return ff_segment;
}

int Race_SegCounter(void)
{
    return ff_seg_counter;
}

int Race_ChaseSegment(void)
{
    return ff_chase_seg;
}

void Race_SetChaseSegment(int seg)
{
    ff_chase_seg = seg;
}

float Race_TrackCentre(int seg)
{
    if (seg < 0)
        seg = 0;
    if (seg >= FF_TRACK_SEGMENTS)
        seg = FF_TRACK_SEGMENTS - 1;

    return ff_centre[seg];
}

float Race_CurveScale(void)
{
    return ff_curve_scale;
}

int Race_PlayerSegment(void)
{
    return Racer_Seg(FF_RACER_PLAYER);
}

void Race_SetPlayerSegment(int seg)
{
    Racer_SetSeg(FF_RACER_PLAYER, seg);
}

/* FFRace.exe 0x0004f6bc, gated on 0x000a76b4 < 0x000832c4: 0x000a77c4 takes 1
   plus one for each of 0x000865d8, 0x000865dc, 0x000865e0 and 0x000865e4 above
   0x000865d4 + 1. */
void Race_UpdateRank(void)
{
    int ahead = Racer_Seg(FF_RACER_PLAYER) + 1;
    int rank  = 1;
    int i;

    if (ff_segment >= ff_length)
        return;

    for (i = FF_RACER_PLAYER + 1; i < FF_RACER_SLOTS; i++)
        if (ahead < Racer_Seg(i))
            rank++;

    ff_rank = rank;
}

int Race_Rank(void)
{
    return ff_rank;
}

void Race_SetCurve(float curve)
{
    ff_curve = curve;
}

void Race_SetPlayerX(float x)
{
    ff_player_x = x;
}
