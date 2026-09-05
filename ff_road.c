#include "ff_road.h"

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_race.h"
#include "ff_racer.h"
#include "ff_render.h"

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW 0x141 -> 0x000a46e8 and
   LoadBitmapW 0x142 -> 0x000a46ec, the two sprites 0x000216c4 blits. */
#define FF_ROAD_RES  0x141
#define FF_VERGE_RES 0x142

/* FFRace.exe 0x000216c4 splits every row on param_7 < 8 and passes
   (param_7 - 7) * 10 as the blend percentage above it. */
#define FF_FOG_DEPTH 8

/* FFRace.exe 0x000216c4 subtracts the left edge from the code literal
   0x43c80000 to reach the verge source column. */
#define FF_VERGE_SPAN 400.0f

/* FFRace.exe 0x000216c4. */
void Road_DrawSegment(const ff_surface *dst, int near_left, int y_far,
                      int far_left, int y_near, int near_right, int far_right,
                      int depth)
{
    const ff_sprite *road  = AppAssets_Sprite(dst, FF_ROAD_RES);
    const ff_sprite *verge = AppAssets_Sprite(dst, FF_VERGE_RES);
    const short *ramp_row;
    int   len    = Race_Length();
    int   seg    = Race_Segment() + depth;
    int   sign_l = 1;
    int   sign_r = 1;
    int   min_l, max_l, min_r, max_r, h, y, stripe, blend;
    float inv, step_l, step_r, left_x, right_x;

    if (!road || !verge)
        return;

    if (seg > len && (Race_SegCounter() + depth) % 3 == 1)
        stripe = 100;
    else if (seg % (len / 4) == 1)
        stripe = 100;
    else
        stripe = 0;
    min_l = near_left;
    max_l = far_left;
    if (far_left < near_left) {
        sign_l = -1;
        min_l  = far_left;
        max_l  = near_left;
    }

    min_r = near_right;
    max_r = far_right;
    if (far_right < near_right) {
        sign_r = -1;
        min_r  = far_right;
        max_r  = near_right;
    }

    inv     = 1.0f / (float)(y_near - y_far);
    step_l  = (float)(max_l - min_l) * inv;
    step_r  = (float)(max_r - min_r) * inv;
    left_x  = (float)far_left;
    right_x = (float)far_right;
    h       = (y_near - y_far) + 1;

    if (y_far < y_near) {
        ramp_row = Render_RampRow(h);
        step_l   = (float)sign_l * step_l;
        y        = y_far;

        do {
            int   ry    = y - Render_RowOffset();
            int   span  = (int)(right_x - left_x);
            float ratio = FF_ROAD_SCALE_REF / Render_RoadScale();
            float unit  = (float)((double)(float)(((double)ratio * 7.0
                                                   / (double)(float)FF_ROAD_WIDTH)
                                                  * (double)span) * 0.19);
            float next_left;
            int   probe, slot;

            if (ry < FF_VIEW_BOTTOM + 1) {
                int ramp = ramp_row[y - y_far];

                if (depth < FF_FOG_DEPTH) {
                    Blit_TileH(dst, (int)(left_x - 1.0f), ry, span + 3,
                               stripe + ramp, road, FF_NO_KEY);
                    Blit_NoKey(dst, 0, ry, (int)left_x, 1, verge,
                               (int)(FF_VERGE_SPAN - left_x), ramp);
                    Blit_NoKey(dst, (int)right_x, ry,
                               (int)(((float)FF_VIEW_W - right_x) + 1.0f), 1,
                               verge, 0, ramp);
                } else {
                    blend = (depth - 7) * 10;
                    Blit_TileH_Ofs(dst, (int)(left_x - 1.0f), ry, span + 3,
                                   stripe + ramp, road, FF_NO_KEY, blend);
                    Blit_Keyed(dst, 0, ry, (int)left_x, 1, verge, blend,
                               FF_NO_KEY, (int)(FF_VERGE_SPAN - left_x), ramp);
                    Blit_Keyed(dst, (int)right_x, ry,
                               (int)(((float)FF_VIEW_W - right_x) + 1.0f), 1,
                               verge, blend, FF_NO_KEY, 0, ramp);
                }
            }

            /* FFRace.exe 0x000216c4 walks 0x000a3bd8 + 8 .. 0x000a3bd8 + 0x3c
               once per row, and for every entry equal to the row writes
               0x000a3c18 from 0x000865e8 and 0x000832d0, and 0x000a3b98 from
               the row unit. */
            for (slot = FF_RACER_FIRST; slot < FF_RACER_SLOTS; slot++) {
                float  centre;
                double span_d;

                if (Racer_Y(slot) != ry)
                    continue;

                centre = ((right_x + 1.0f) - left_x) * 0.5f + left_x;
                span_d = ((double)right_x + 1.0) - (double)left_x;

                Racer_SetX(slot,
                           (int)(centre - (float)((span_d * (double)Racer_Lat(slot))
                                                  / (double)(float)FF_ROAD_WIDTH)));
                Racer_SetScale(slot, unit);
            }

            next_left = left_x - step_l;
            probe     = (int)next_left - (int)left_x;
            right_x  -= (float)sign_r * step_r;

            if (depth == 0 && ry > FF_VIEW_BOTTOM &&
                next_left - (float)(-(probe * sign_l)) < 0.0f &&
                right_x + 1.0f > (float)FF_VIEW_W)
                y = y_near;

            y++;
            left_x = next_left;
        } while (y < y_near);
    }
}
