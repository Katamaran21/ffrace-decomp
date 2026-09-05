/* Scaled tilers ported from JumpyBall.exe (imagebase 0x00010000). */
#include "ff_gfx.h"

/* JumpyBall.exe Gfx_CreateBackBuffer 0x00021698: DAT_00068518 span 100.0f,
   DAT_00065ed0 span 64.0f, DAT_00064b48 span 44.0f. */
#define FF_T100_STRIDE 200
#define FF_T100_ROWS   200
#define FF_T64_STRIDE  70
#define FF_T64_ROWS    64
#define FF_T44_STRIDE  50
#define FF_T44_ROWS    50

static short ff_step100[FF_T100_ROWS * FF_T100_STRIDE];
static short ff_step64[FF_T64_ROWS * FF_T64_STRIDE];
static short ff_step44[FF_T44_ROWS * FF_T44_STRIDE];

static void BuildStepTable(short *tbl, int stride, int last_w, float span)
{
    int w, k;

    for (w = 1; w <= last_w; w++) {
        float step = span / (float)w;
        float acc  = 0.0f;
        for (k = 0; k < w; k++) {
            acc = acc + step;
            tbl[w * stride + k] = (short)((int)acc - (int)(acc - step));
        }
    }
}

void Gfx_BuildScaleTables(void)
{
    BuildStepTable(ff_step100, FF_T100_STRIDE, 199, 100.0f);
    BuildStepTable(ff_step64, FF_T64_STRIDE, 63, 64.0f);
    BuildStepTable(ff_step44, FF_T44_STRIDE, 49, 44.0f);
}

/* JumpyBall.exe Blit_TileHV_Core 0x000228e8: each axis picks table row
   (100 + v%100) with hold count (v/100 - 1) when v > 100, else row v. */
static int ScaleRow(int v, int *hold)
{
    if (v > 100) {
        *hold = v / 100 - 1;
        return 100 + v % 100;
    }
    *hold = 0;
    return v;
}

/* JumpyBall.exe Blit_TileHV_Core 0x000228e8: the source pointer advances by
   DAT_00068518[row][n] only once the hold counter saturates, and the counter
   resets solely on a step of 1. */
static int ScaleStep(const short *tbl, int hold, int *held, int *n)
{
    short step;

    if (*held < hold) {
        (*held)++;
        return 0;
    }
    step = tbl[*n];
    (*n)++;
    if (step == 1)
        *held = 0;
    return step;
}

/* JumpyBall.exe Blit_TileHV_Core 0x000228e8 negative-coordinate path, float
   literal 0x42c80000 = 100.0f: for t = 100.0f/extent * coord the source offset
   is (int)-t and the initial hold counter is (int)(-(t - (int)t) * hold). */
static int ScalePreRoll(float t, int hold, int *held0)
{
    *held0 = (int)(-(t - (float)(int)t) * (float)hold);
    return (int)-t;
}

/* JumpyBall.exe Blit_TileH 0x00023aa8 -> Blit_TileH_Core 0x00022e7c.  The two
   __rt_sdiv calls at 0x00022e94 select table row (100 + w%100) and hold count
   (w/100 - 1). */
int Blit_TileH(const ff_surface *dst, int x, int y, int w, int src_row,
               const ff_sprite *src, unsigned key)
{
    const short *tbl;
    uint16_t *d;
    const uint16_t *s;
    int row, hold, held, n, skip, i;

    row = ScaleRow(w, &hold);
    if (y < 0 || !dst->pixels || !src || !src->pixels || w < 1 || src_row < 1 ||
        y > ff_clip_h_row)
        return 0;
    skip = 0;
    if (x < 0) {
        w += x;
        skip = -x;
        x = 0;
        if (w < 0)
            return 0;
    }
    if (ff_clip_w - x < w)
        w = ff_clip_w - x;
    tbl = ff_step100 + row * FF_T100_STRIDE;
    d = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    s = src->pixels + src->w * src_row;
    held = hold;
    n = 0;
    for (i = skip; i > 0; i--)
        s += ScaleStep(tbl, hold, &held, &n);
    if (dst->bpp != 0x10)
        return row;
    for (i = w; i > 0; i--) {
        if (key == FF_NO_KEY || *s != (uint16_t)key)
            *d = *s;
        d += dst->x_pitch;
        s += ScaleStep(tbl, hold, &held, &n);
    }
    return row;
}

/* JumpyBall.exe Blit_TileHV 0x00023a7c -> Blit_TileHV_Core 0x000228e8. */
int Blit_TileHV(const ff_surface *dst, int x, int y, int w, int h,
                const ff_sprite *src, unsigned key)
{
    uint16_t *drow;
    const uint16_t *srow;
    int row_h, hold_h, row_v, hold_v, held_v, n_v, held0, sx, sy, cw, ch, i;

    row_h = ScaleRow(w, &hold_h);
    row_v = ScaleRow(h, &hold_v);
    held0 = 0;
    sx = 0;
    sy = 0;
    cw = w;
    ch = h;
    if (x < 0) {
        cw = x + w;
        sx = ScalePreRoll(100.0f / (float)w * (float)x, hold_h, &held0);
        x = 0;
    }
    if (y < 0) {
        ch = y + h;
        sy = (int)-(100.0f / (float)h * (float)y);
        y = 0;
    }
    if (ff_clip_w - x < cw)
        cw = ff_clip_w - x;
    if (ff_clip_h_row - y < ch)
        ch = ff_clip_h_row - y;
    if (!dst->pixels || !src || !src->pixels || cw < 1 || ch < 1 || dst->bpp != 0x10)
        return 0;
    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * sy + sx;
    held_v = hold_v;
    n_v = 0;
    for (; ch > 0; ch--) {
        const uint16_t *s = srow;
        uint16_t *d = drow;
        int held_h = held0;
        int n_h = 0;

        for (i = cw; i > 0; i--) {
            if (key == FF_NO_KEY || *s != (uint16_t)key)
                *d = *s;
            d += dst->x_pitch;
            s += ScaleStep(ff_step100 + row_h * FF_T100_STRIDE, hold_h, &held_h, &n_h);
        }
        drow += dst->y_pitch;
        srow += src->w *
                ScaleStep(ff_step100 + row_v * FF_T100_STRIDE, hold_v, &held_v, &n_v);
    }
    return 1;
}

/* JumpyBall.exe Blit_TileHV_Ofs 0x00023ad4 -> Blit_TileHV_Ofs_Core 0x000230f0. */
int Blit_TileHV_Ofs(const ff_surface *dst, int x, int y, int w, int h,
                    const ff_sprite *src, unsigned key, int blend)
{
    uint16_t *drow;
    const uint16_t *srow;
    int row_h, hold_h, row_v, hold_v, held_v, n_v, held0, sx, sy, cw, ch, i;

    row_h = ScaleRow(w, &hold_h);
    row_v = ScaleRow(h, &hold_v);
    held0 = 0;
    sx = 0;
    sy = 0;
    cw = w;
    ch = h;
    if (x < 0) {
        cw = x + w;
        sx = ScalePreRoll(100.0f / (float)w * (float)x, hold_h, &held0);
        x = 0;
    }
    if (y < 0) {
        ch = y + h;
        sy = (int)-(100.0f / (float)h * (float)y);
        y = 0;
    }
    if (ff_clip_w - x < cw)
        cw = ff_clip_w - x;
    if (ff_clip_h_row - y < ch)
        ch = ff_clip_h_row - y;
    if (!dst->pixels || !src || !src->pixels || cw < 1 || ch < 1 || dst->bpp != 0x10)
        return 0;
    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * sy + sx;
    held_v = hold_v;
    n_v = 0;
    for (; ch > 0; ch--) {
        const uint16_t *s = srow;
        uint16_t *d = drow;
        int held_h = held0;
        int n_h = 0;

        for (i = cw; i > 0; i--) {
            if (key == FF_NO_KEY || *s != (uint16_t)key)
                *d = Color_Blend(dst, *s, *d, blend);
            d += dst->x_pitch;
            s += ScaleStep(ff_step100 + row_h * FF_T100_STRIDE, hold_h, &held_h, &n_h);
        }
        drow += dst->y_pitch;
        srow += src->w *
                ScaleStep(ff_step100 + row_v * FF_T100_STRIDE, hold_v, &held_v, &n_v);
    }
    return 1;
}

/* JumpyBall.exe Blit_Variant9 0x00023b08 -> Blit_Variant9_Core 0x00021dd0. */
int Blit_TileHV_Glyph(const ff_surface *dst, int x, int y, int w, int h,
                      const ff_sprite *src, int r, int g, int b)
{
    uint16_t *drow;
    const uint16_t *srow;
    int row_h, hold_h, row_v, hold_v, held_v, n_v, held0, sx, sy, cw, ch, i;

    row_h = ScaleRow(w, &hold_h);
    row_v = ScaleRow(h, &hold_v);
    held0 = 0;
    sx = 0;
    sy = 0;
    cw = w;
    ch = h;
    if (x < 0) {
        cw = x + w;
        sx = ScalePreRoll(100.0f / (float)w * (float)x, hold_h, &held0);
        x = 0;
    }
    if (y < 0) {
        ch = y + h;
        sy = (int)-(100.0f / (float)h * (float)y);
        y = 0;
    }
    if (ff_clip_w - x < cw)
        cw = ff_clip_w - x;
    if (ff_clip_h_row - y < ch)
        ch = ff_clip_h_row - y;
    if (!dst->pixels || !src || !src->pixels || cw < 1 || ch < 1 || dst->bpp != 0x10)
        return 0;
    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * sy + sx;
    held_v = hold_v;
    n_v = 0;
    for (; ch > 0; ch--) {
        const uint16_t *s = srow;
        uint16_t *d = drow;
        int held_h = held0;
        int n_h = 0;

        for (i = cw; i > 0; i--) {
            *d = Color_MaskTint(dst, *s, *d, r, g, b);
            d += dst->x_pitch;
            s += ScaleStep(ff_step100 + row_h * FF_T100_STRIDE, hold_h, &held_h, &n_h);
        }
        drow += dst->y_pitch;
        srow += src->w *
                ScaleStep(ff_step100 + row_v * FF_T100_STRIDE, hold_v, &held_v, &n_v);
    }
    return 1;
}

int Blit_TileH_Ofs(const ff_surface *dst, int x, int y, int w, int src_row,
                   const ff_sprite *src, unsigned key, int blend)
{
    const short *tbl;
    uint16_t *d;
    const uint16_t *s;
    int row, hold, held, n, skip, i;

    row = ScaleRow(w, &hold);
    if (y < 0 || !dst->pixels || !src || !src->pixels || w < 1 || src_row < 1 ||
        y > ff_clip_h_row)
        return 0;
    skip = 0;
    if (x < 0) {
        w += x;
        skip = -x;
        x = 0;
    }
    if (ff_clip_w - x < w)
        w = ff_clip_w - x;
    tbl = ff_step100 + row * FF_T100_STRIDE;
    d = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    s = src->pixels + src->w * src_row;
    held = hold;
    n = 0;
    for (i = skip; i > 0; i--)
        s += ScaleStep(tbl, hold, &held, &n);
    if (dst->bpp != 0x10)
        return 0;
    for (i = w; i > 0; i--) {
        if (key == FF_NO_KEY || *s != (uint16_t)key)
            *d = Color_Blend(dst, *s, *d, blend);
        d += dst->x_pitch;
        s += ScaleStep(tbl, hold, &held, &n);
    }
    return 1;
}

/* FFRace.exe 0x0005a78c forwards eight arguments unchanged to 0x00059334, which
   overwrites the h slot at 0x000593a4 before reading it: 0x00059360 clamps the
   width to 0x3f and shifts x by (w - 0x40) >> 1, 0x00059384 takes the height as
   w * 0x2c >> 6, and 0x000560ec builds 0x000a8bc0 / 0x000a7838 at 64.0 / 44.0. */
int Blit_Scale64(const ff_surface *dst, int x, int y, int w, int h,
                 const ff_sprite *src, unsigned key, int src_x)
{
    const short *hstep;
    const uint16_t *srow;
    uint16_t *drow;
    int cw, ch, run_w, run_h, src_row, shift, i;

    (void)h;

    cw = w;
    if (cw > 0x3f) {
        shift = w - 0x40;
        if (shift < 0)
            shift = w - 0x3f;
        x += shift >> 1;
        cw = 0x3f;
    }
    ch = cw * 0x2c;
    if (ch < 0)
        ch += 0x3f;
    ch >>= 6;

    run_w   = cw;
    run_h   = ch;
    src_row = 0;
    if (x < 0) {
        run_w = x + cw;
        src_x = (int)((float)src_x - 64.0f / (float)cw * (float)x);
        x = 0;
    }
    if (y < 0) {
        run_h   = y + ch;
        src_row = (int)-(44.0f / (float)ch * (float)y);
        y = 0;
    }
    if (ff_clip_w - x < run_w)
        run_w = ff_clip_w - x;
    if (ff_clip_h_row - y < run_h)
        run_h = ff_clip_h_row - y;
    if (!dst->pixels || !src || !src->pixels || run_w < 1 || run_h < 1
        || dst->bpp != 0x10)
        return 0;

    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * src_row + src_x;
    /* 0x000595d8: a key pixel takes half of each non-key horizontal neighbour
       through 0x0005a984, the right one over the left one's result. */
    for (i = 0; i < run_h; i++) {
        const uint16_t *s = srow;
        uint16_t *d = drow;
        int col;

        hstep = ff_step64 + cw * FF_T64_STRIDE;
        for (col = 0; col < run_w; col++) {
            if ((unsigned)*s != key) {
                *d = *s;
            } else if (col > 0) {
                if ((unsigned)s[-1] != key)
                    *d = Color_Blend(dst, s[-1], *d, 0x32);
                if ((unsigned)s[1] != key)
                    *d = Color_Blend(dst, s[1], *d, 0x32);
            }
            d += dst->x_pitch;
            s += *hstep++;
        }
        drow += dst->y_pitch;
        srow += src->w * ff_step44[ch * FF_T44_STRIDE + i];
    }
    return 1;
}

/* FFRace.exe 0x0005a7c0 forwards ten arguments unchanged to 0x000596d4, whose
   0x00059744 overwrites the h slot before it is read; 0x00059754 clamps the
   width to 0x3f and shifts x by (w - 0x40) >> 1 and 0x00059778 takes the height
   as w * 0x2c >> 6, both exactly as 0x00059334 does. */
int Blit_Scale64Tint(const ff_surface *dst, int x, int y, int w, int h,
                     const ff_sprite *src, int r, int g, int b, int src_x)
{
    const short *hstep;
    const uint16_t *srow;
    uint16_t *drow;
    int cw, ch, run_w, run_h, src_row, shift, i;

    (void)h;

    cw = w;
    if (cw > 0x3f) {
        shift = w - 0x40;
        if (shift < 0)
            shift = w - 0x3f;
        x += shift >> 1;
        cw = 0x3f;
    }
    ch = cw * 0x2c;
    if (ch < 0)
        ch += 0x3f;
    ch >>= 6;

    run_w   = cw;
    run_h   = ch;
    src_row = 0;
    if (x < 0) {
        run_w = x + cw;
        src_x = (int)((float)src_x - 64.0f / (float)cw * (float)x);
        x = 0;
    }
    if (y < 0) {
        run_h   = y + ch;
        src_row = (int)-(44.0f / (float)ch * (float)y);
        y = 0;
    }
    if (ff_clip_w - x < run_w)
        run_w = ff_clip_w - x;
    if (ff_clip_h_row - y < run_h)
        run_h = ff_clip_h_row - y;
    if (!dst->pixels || !src || !src->pixels || run_w < 1 || run_h < 1
        || dst->bpp != 0x10)
        return 0;

    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * src_row + src_x;
    /* FFRace.exe 0x00059978..0x000599c0 carries no key test and no branch. */
    for (i = 0; i < run_h; i++) {
        const uint16_t *s = srow;
        uint16_t *d = drow;
        int col;

        hstep = ff_step64 + cw * FF_T64_STRIDE;
        for (col = 0; col < run_w; col++) {
            *d = Color_MaskTint(dst, *s, *d, r, g, b);
            d += dst->x_pitch;
            s += *hstep++;
        }
        drow += dst->y_pitch;
        srow += src->w * ff_step44[ch * FF_T44_STRIDE + i];
    }
    return 1;
}
