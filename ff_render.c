#include "ff_render.h"

#include "ff_consts.h"
#include "ff_race.h"
#include "ff_racer.h"
#include "ff_screen.h"

/* FFRace.exe 0x000a3de0 and 0x000a3e50, the destinations of the two 0x70-byte
   copies at 0x00011b18 and 0x00011b2c. */
static int ff_row[FF_ROW_COUNT];
static int ff_delta[FF_ROW_COUNT];

/* FFRace.exe Game_Init 0x000115b0 at 0x000117cc, the table at 0x000902d0. */
static short ff_ramp[FF_RAMP_ROWS][FF_RAMP_STRIDE];

static float ff_road_scale = FF_ROAD_SCALE_MAX; /* 0x000832d8 */
static int   ff_road_half  = 3;                 /* 0x000832dc */
static int   ff_base_x;                         /* 0x000a766c */
static int   ff_row_offset;                     /* 0x000a7630 */

struct ff_proj {
    float scale_wide;
    float scale_narrow;
    float centre_x;
    float off_wide;
    float off_narrow;
    float den;
};

/* FFRace.exe Game_Init 0x000115b0 at 0x00011a28 divides a numerator stepping by
   12000 by a denominator stepping by 50 plus 30.0f and subtracts 80.0f;
   0x00011ae0 takes the successive differences and 0x00011b00 forces entry 1 to
   0x96. */
void Render_BuildTables(void)
{
    int i, h, k;

    for (i = 0; i < FF_ROW_COUNT; i++) {
        float num = (float)(-12000 + 12000 * i);
        float den = (float)(-50 + 50 * i);

        ff_row[i] = (int)(num / (den + 30.0f) - 80.0f);
    }

    ff_delta[0] = 0;
    for (i = 1; i < FF_ROW_COUNT; i++)
        ff_delta[i] = ff_row[i] - ff_row[i - 1];
    ff_delta[1] = 0x96;

    /* FFRace.exe Game_Init 0x000115b0 at 0x000117e8 divides the literal
       0x42c80000 by (float)h, at 0x0001183c accumulates it, at 0x00011854
       truncates to int, and at 0x0001187c stores 1 instead when the
       sign-extended halfword is below 1; 0x0001167c is the mov sb, #1. */
    for (h = 1; h < FF_RAMP_ROWS; h++) {
        float step = 100.0f / (float)h;
        float acc  = 0.0f;

        for (k = 0; k < h; k++) {
            short v;

            acc += step;
            v = (short)(int)acc;
            ff_ramp[h][k] = v < 1 ? (short)1 : v;
        }
    }
}

/* FFRace.exe 0x000216c4 reads the ramp entry as
   *(short *)(&DAT_000902d0 + ((h * 200 - y_far) + y) * 2). */
const short *Render_RampRow(int h)
{
    return ff_ramp[h];
}

float Render_BandY(float z)
{
    return (256.0f - 12000.0f * z / (50.0f * z + 30.0f)) + 80.0f;
}

int Render_RowOffset(void)
{
    return ff_row_offset;
}

int Render_BaseX(void)
{
    return ff_base_x;
}

float Render_RoadScale(void)
{
    return ff_road_scale;
}

/* FFRace.exe 0x00046754 projects every edge twice, with 0x000832d8 and with the
   layout scale, and interpolates between them over 0x000a3e44 + 1. */
static int Render_ProjectX(const struct ff_proj *p, float world, float t)
{
    float wide   = world * p->scale_wide + p->centre_x - p->off_wide;
    float narrow = world * p->scale_narrow + p->centre_x - p->off_narrow;

    return (int)((double)(((narrow - wide) / p->den) * t + wide) - 0.5) + 1;
}

/* FFRace.exe 0x00046754, the branch on 0x000a779c right after the 0x000a766c
   store: mode 0 walks 0x000a4308 + 8 .. 0x000a4308 + 0x14 with 0x000a42f0,
   0x000a430c and 0x000a76b4, and drops a slot whose 0x000865d0 entry has reached
   0x000865d4 + 0x12; mode 2 writes 0x000a3be0 alone, from 0x000a4310,
   0x000a7754 and 0x000a76bc.  Both subtract 0x23 when 0x000a7630 is 0x14. */
static void Render_ProjectRacers(float inv_seg, float dist)
{
    float shift   = (float)((ff_row_offset == FF_LAYOUT_176x220) * 0x23);
    int   segment = Race_Segment();
    int   player  = Race_PlayerSegment();
    int   slot;

    if (Race_Mode() == FF_RACE_SCRIPTED) {
        for (slot = FF_RACER_FIRST; slot < FF_RACER_SLOTS; slot++) {
            float z = (Racer_Dist(slot) * inv_seg + (float)Racer_SegCount(slot))
                      - inv_seg * dist - (float)segment;

            if (!(z >= 0.0f) || player + 0x12 <= Racer_Seg(slot))
                Racer_SetY(slot, 0);
            else
                Racer_SetY(slot, (int)(Render_BandY(z) - shift));
        }
    } else if (Race_Mode() == FF_RACE_ENDLESS_2) {
        int   chase   = Race_ChaseSegment();
        int   counter = Race_SegCounter();
        float z = (inv_seg * Racer_Dist(FF_RACER_FIRST) + (float)chase)
                  - inv_seg * dist - (float)counter;

        if (!(z >= 0.0f) || counter + 0x12 <= chase)
            Racer_SetY(FF_RACER_FIRST, 0);
        else
            Racer_SetY(FF_RACER_FIRST, (int)(Render_BandY(z) - shift));
    }
}

/* FFRace.exe 0x00046754. */
void Render_Road(const ff_surface *dst, ff_segment_drawer drawer)
{
    const float *centre = Race_CentreWindow();
    float curve    = Race_Curve();
    float player_x = Race_PlayerX();
    float dist     = Race_Dist();
    float narrow_half, inv_seg, frac;
    struct ff_proj proj;
    int a;

    ff_row_offset = Screen_Layout();
    if (ff_row_offset == FF_LAYOUT_240x320) {
        proj.scale_narrow = 2.0f;
        proj.centre_x     = (float)FF_VIEW_CENTER_X;
        narrow_half       = 1.0f;
    } else {
        proj.scale_narrow = 1.0f;
        proj.centre_x     = (float)FF_SMALL_VIEW_CENTER_X;
        narrow_half       = 0.5f;
    }

    if (ff_road_scale < FF_ROAD_SCALE_MAX)
        ff_road_scale += 1.0f;
    ff_road_half = (int)(ff_road_scale * 0.5f);

    inv_seg   = 1.0f / (float)FF_SEGMENT_LEN;
    ff_base_x = (int)((double)(-(((curve + centre[1]) - centre[0]) * (inv_seg * dist)
                                 + centre[0] + player_x)) * 10.0);

    Render_ProjectRacers(inv_seg, dist);

    proj.scale_wide = ff_road_scale;
    proj.off_wide   = (float)(FF_ROAD_WIDTH * ff_road_half);
    proj.off_narrow = (float)FF_ROAD_WIDTH * narrow_half;
    proj.den        = (float)(ff_row[25] + 1);

    frac = dist / (float)FF_SEGMENT_LEN;

    for (a = 0; a < FF_DRAW_DEPTHS + 1; a++) {
        int depth = FF_DRAW_DEPTHS - a;
        float y_near, y_far, t, w;
        int near_left, near_right, far_left, far_right;

        if (depth == 0)
            y_near = (float)(256 - ff_row[1]) + (float)ff_delta[1] * frac;
        else
            y_near = Render_BandY((float)depth - frac);

        if (a == 0)
            y_far = Render_BandY((float)(depth + 1));
        else
            y_far = Render_BandY((float)(depth + 1) - frac);

        t = 256.0f - y_far;
        w = (float)(depth + 1) * curve + centre[depth + 1];
        far_left  = Render_ProjectX(&proj, w + player_x, t);
        far_right = Render_ProjectX(&proj, ((float)FF_ROAD_WIDTH + w) + player_x, t);

        t = 256.0f - y_near;
        w = (float)depth * curve + centre[depth];
        near_left  = Render_ProjectX(&proj, w + player_x, t);
        near_right = Render_ProjectX(&proj, ((float)FF_ROAD_WIDTH + w) + player_x, t);

        if (ff_row_offset == FF_LAYOUT_176x220)
            ff_row_offset = FF_ROW_OFFSET_176x220;

        if (y_far + 1.0f > y_near)
            y_near = y_far + 1.0f;

        drawer(dst, ff_base_x + near_left, (int)y_far, ff_base_x + far_left,
               (int)y_near, ff_base_x + near_right, ff_base_x + far_right,
               depth);

        if (ff_row_offset == FF_ROW_OFFSET_176x220)
            ff_row_offset = FF_LAYOUT_176x220;
    }
}
