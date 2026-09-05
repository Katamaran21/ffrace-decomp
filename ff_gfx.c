/* Graphics core ported from JumpyBall.exe (imagebase 0x00010000). */
#include "ff_gfx.h"

/* JumpyBall.exe Gfx_CreateBackBuffer 0x00021698, 0x000269e0 / 0x000269e4 /
   0x000269e8 .data initialisers. */
int ff_clip_w     = 240;
int ff_clip_h     = 320;
int ff_clip_h_row = 320;

/* JumpyBall.exe Color_Pack16 0x00023bb4, fmt flag 0x80 branch. */
uint16_t Color_Pack16(const ff_surface *dst, uint32_t colorref)
{
    unsigned v;

    if (!(dst->fmt & FF_FMT_RGB565))
        return 0;
    v = (((colorref & 0xffff) >> 8 & 0xfff8) | ((colorref & 0xf8) << 5)) << 3;
    return (uint16_t)(v | ((colorref >> 16 & 0xff) >> 3));
}

/* JumpyBall.exe Color_Pack16If16bpp 0x00024030 tests the destination word at
   +0x18 against 0x10 before calling Color_Pack16 0x00023bb4. */
uint16_t Color_Pack16If16bpp(const ff_surface *dst, uint32_t colorref)
{
    if (dst->bpp != 16)
        return 0;
    return Color_Pack16(dst, colorref);
}

/* JumpyBall.exe Color_Blend 0x00023c3c. */
uint16_t Color_Blend(const ff_surface *dst, uint16_t src, uint16_t dstpix, int blend)
{
    int sr, sg, sb, dr, dg, db, inv, r, g, b;
    unsigned out;

    if (blend == 0)
        return src;
    if (!(dst->fmt & FF_FMT_RGB565))
        return 0;
    sg = (src >> 3) & 0xfc;
    sr = (src >> 8) & 0xf8;
    sb = (src & 0x1f) << 3;
    dg = (dstpix >> 3) & 0xfc;
    dr = (dstpix >> 8) & 0xf8;
    db = (dstpix & 0x1f) << 3;
    inv = 100 - blend;
    r = (inv * sr + dr * blend) / 100;
    b = (inv * sb + db * blend) / 100;
    g = (inv * sg + dg * blend) / 100;
    out = ((0xfff8 & (unsigned)g) | (unsigned)((r >> 3) << 8)) << 3;
    return (uint16_t)((out | (unsigned)(b >> 3)) & 0xffff);
}

/* JumpyBall.exe FUN_00023ddc 0x00023ddc, the pixel operation of
   Blit_Glyph_Core 0x00022288. */
uint16_t Color_MaskTint(const ff_surface *dst, uint16_t src, uint16_t dstpix,
                        int r, int g, int b)
{
    int dr, dg, db, cov, inv, orr, ogg, obb;
    unsigned out;

    if (!(dst->fmt & FF_FMT_RGB565))
        return 0;
    db = (dstpix & 0x1f) << 3;
    dg = (dstpix >> 3) & 0xfc;
    dr = (dstpix >> 8) & 0xf8;
    cov = (src & 0x1f) << 3;
    inv = 0xff - cov;
    orr = (inv * dr + cov * r) / 0xff;
    obb = (inv * db + cov * b) / 0xff;
    ogg = (inv * dg + cov * g) / 0xff;
    out = ((0xfff8 & (unsigned)ogg) | (unsigned)((orr >> 3) << 8)) << 3;
    return (uint16_t)((out | (unsigned)(obb >> 3)) & 0xffff);
}

/* JumpyBall.exe Blit_Core 0x00021a98. */
int Blit_Core(const ff_surface *dst, int x, int y, int w, int h,
              const ff_sprite *src, int src_x, int src_y, unsigned key)
{
    uint16_t *drow;
    const uint16_t *srow;
    int cw, i;

    if (!dst->pixels || !src || !src->pixels || w < 1 || h < 1 || src_x < 0 || src_y < 0)
        return 0;
    cw = w;
    if (src->w < w + src_x)
        cw = src->w - src_x;
    if (src->h < h + src_y)
        h = src->h - src_y;
    if (x > ff_clip_w || y > ff_clip_h)
        return 1;
    if (x < 0) {
        src_x -= x;
        cw += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        src_y -= y;
        y = 0;
    }
    if (ff_clip_w - x < cw)
        cw = ff_clip_w - x;
    if (ff_clip_h - y < h)
        h = ff_clip_h - y;
    if (dst->bpp != 0x10)
        return 0;
    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * src_y + src_x;
    for (; h > 0; h--) {
        uint16_t *d = drow;
        const uint16_t *s = srow;
        for (i = cw; i > 0; i--) {
            if (key == FF_NO_KEY || *s != (uint16_t)key)
                *d = *s;
            d += dst->x_pitch;
            s++;
        }
        drow += dst->y_pitch;
        srow += src->w;
    }
    return 1;
}

/* JumpyBall.exe Blit_Keyed 0x000239f8 -> Blit_Keyed_Core 0x00022574. */
int Blit_Keyed(const ff_surface *dst, int x, int y, int w, int h,
               const ff_sprite *src, int blend, unsigned key,
               int src_x, int src_y)
{
    uint16_t *drow;
    const uint16_t *srow;
    int cw, i;

    if (!dst->pixels || !src || !src->pixels || w < 1 || h < 1 || src_x < 0 || src_y < 0)
        return 0;
    if (src->w < w + src_x)
        w = src->w - src_x;
    if (src->h < h + src_y)
        h = src->h - src_y;
    if (x > ff_clip_w || y > ff_clip_h)
        return 1;
    cw = w;
    if (x < 0) {
        cw = x + w;
        src_x -= x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        src_y -= y;
        y = 0;
    }
    if (ff_clip_w - x < cw)
        cw = ff_clip_w - x;
    if (ff_clip_h - y < h)
        h = ff_clip_h - y;
    if (dst->bpp != 0x10)
        return 0;
    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * src_y + src_x;
    for (; h > 0; h--) {
        uint16_t *d = drow;
        const uint16_t *s = srow;
        for (i = cw; i > 0; i--) {
            if (key == FF_NO_KEY || *s != (uint16_t)key)
                *d = Color_Blend(dst, *s, *d, blend);
            d += dst->x_pitch;
            s++;
        }
        drow += dst->y_pitch;
        srow += src->w;
    }
    return 1;
}

/* JumpyBall.exe Blit_Glyph 0x000239ac -> Blit_Glyph_Core 0x00022288; every pixel
   goes through FUN_00023ddc 0x00023ddc and there is no colour key. */
int Blit_Glyph(const ff_surface *dst, int x, int y, int w, int h,
               const ff_sprite *src, int r, int g, int b,
               int src_x, int src_y)
{
    uint16_t *drow;
    const uint16_t *srow;
    int cw, i;

    if (!dst->pixels || !src || !src->pixels || w < 1 || h < 1 || src_x < 0 || src_y < 0)
        return 0;
    if (src->w < w + src_x)
        w = src->w - src_x;
    if (src->h < h + src_y)
        h = src->h - src_y;
    if (x > ff_clip_w || y > ff_clip_h)
        return 1;
    cw = w;
    if (x < 0) {
        cw = x + w;
        src_x -= x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        src_y -= y;
        y = 0;
    }
    if (ff_clip_w - x < cw)
        cw = ff_clip_w - x;
    if (ff_clip_h - y < h)
        h = ff_clip_h - y;
    if (dst->bpp != 0x10)
        return 0;
    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + src->w * src_y + src_x;
    for (; h > 0; h--) {
        uint16_t *d = drow;
        const uint16_t *s = srow;
        for (i = cw; i > 0; i--) {
            *d = Color_MaskTint(dst, *s, *d, r, g, b);
            d += dst->x_pitch;
            s++;
        }
        drow += dst->y_pitch;
        srow += src->w;
    }
    return 1;
}

/* JumpyBall.exe Blit_NoKey 0x00023a3c -> Blit_Core 0x00021a98 with key 0xffff. */
int Blit_NoKey(const ff_surface *dst, int x, int y, int w, int h,
               const ff_sprite *src, int src_x, int src_y)
{
    return Blit_Core(dst, x, y, w, h, src, src_x, src_y, FF_NO_KEY);
}

/* JumpyBall.exe Blit 0x00023b78 -> Blit_Core 0x00021a98. */
int Blit(const ff_surface *dst, int x, int y, int w, int h,
         const ff_sprite *src, unsigned key, int src_x, int src_y)
{
    return Blit_Core(dst, x, y, w, h, src, src_x, src_y, key);
}

/* FFRace.exe FUN_00056824 0x00056824. */
uint16_t Color_EdgeMean(const ff_surface *dst, uint16_t dstpix,
                        uint16_t left, uint16_t right,
                        uint16_t up, uint16_t down, unsigned key)
{
    uint16_t nb[4];
    int r = 0, g = 0, b = 0, n = 4, i, w;
    unsigned out;

    if (!(dst->fmt & FF_FMT_RGB565))
        return 0;
    nb[0] = left;
    nb[1] = right;
    nb[2] = up;
    nb[3] = down;
    for (i = 0; i < 4; i++) {
        if (nb[i] == (uint16_t)key) {
            n--;
            continue;
        }
        b += (nb[i] & 0x1f) * 800;
        g += ((nb[i] >> 3) & 0xfc) * 100;
        r += ((nb[i] >> 8) & 0xf8) * 100;
    }
    w = 5 - n;
    b = ((dstpix & 0x1f) * w * 800 + b) / 500;
    g = (((dstpix >> 3) & 0xfc) * w * 100 + g) / 500;
    r = (((dstpix >> 8) & 0xf8) * w * 100 + r) / 500;
    out = ((0xfff8 & (unsigned)g) | (unsigned)((r >> 3) << 8)) << 3;
    return (uint16_t)((out | (unsigned)(b >> 3)) & 0xffff);
}

/* FFRace.exe Blit_EdgeBlend 0x0005a884 -> 0x00056a04. */
int Blit_EdgeBlend(const ff_surface *dst, int x, int y, int w, int h,
                   const ff_sprite *src, unsigned key, int src_x, int src_y)
{
    uint16_t *drow;
    const uint16_t *srow;
    int cw, row, col, stride;

    if (!dst->pixels || !src || !src->pixels || w < 1 || h < 1 || src_x < 0 || src_y < 0)
        return 0;
    if (src->w < w + src_x)
        w = src->w - src_x;
    if (src->h < h + src_y)
        h = src->h - src_y;
    if (x > ff_clip_w || y > ff_clip_h)
        return 1;
    cw = w;
    if (x < 0) {
        cw = x + w;
        src_x -= x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        src_y -= y;
        y = 0;
    }
    if (ff_clip_w - x < cw)
        cw = ff_clip_w - x;
    if (ff_clip_h - y < h)
        h = ff_clip_h - y;
    if (dst->bpp != 0x10)
        return 0;
    stride = src->w;
    drow = dst->pixels + dst->x_pitch * x + dst->y_pitch * y;
    srow = src->pixels + stride * src_y + src_x;
    for (row = 0; row < h; row++) {
        uint16_t *d = drow;
        const uint16_t *s = srow;

        for (col = 0; col < cw; col++) {
            uint16_t v = *s;

            if (key == FF_NO_KEY || v != (uint16_t)key)
                *d = v;
            else if (col > 0 && col < cw - 1)
                *d = Color_EdgeMean(dst, *d, s[-1], s[1],
                                    (row < 1 || row >= h - 1) ? (uint16_t)key : s[-stride],
                                    (row < 1 || row >= h - 1) ? (uint16_t)key : s[stride],
                                    key);
            else if (row > 0 && row < h - 1)
                *d = Color_EdgeMean(dst, *d, (uint16_t)key, (uint16_t)key,
                                    s[-stride], s[stride], key);
            d += dst->x_pitch;
            s++;
        }
        drow += dst->y_pitch;
        srow += stride;
    }
    return 1;
}
