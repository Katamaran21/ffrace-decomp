/* Graphics core, ported from JumpyBall.exe (imagebase 0x00010000).
   Every routine here mirrors one original function; the VA and the name from
   out/names.csv are cited on each declaration. */
#ifndef FF_GFX_H
#define FF_GFX_H

#include <stdint.h>

/* JumpyBall.exe Blit_Core 0x00021a98 reads the destination through the int
   indices [0] pixels, [6] bpp, [7] fmt, [9] xPitch, [10] yPitch. */
#define FF_FMT_RGB565 0x80
#define FF_FMT_RGB555 0x40

typedef struct {
    uint16_t *pixels;
    int       bpp;
    int       fmt;
    int       x_pitch;
    int       y_pitch;
} ff_surface;

/* JumpyBall.exe Sprite_LoadResourceBitmap 0x00021a04 allocates 0xc bytes and
   fills [0] pixels (Surface_ImportPixels), [1] width, [2] height.  Blit_Core
   uses [1] as both the width and the row stride. */
typedef struct {
    uint16_t *pixels;
    int       w;
    int       h;
} ff_sprite;

/* JumpyBall.exe Gfx_CreateBackBuffer 0x00021698 stores the clip bounds in
   0x000269e0 / 0x000269e4 / 0x000269e8; the .data initialisers are 240/320/320. */
extern int ff_clip_w;
extern int ff_clip_h;
extern int ff_clip_h_row;

/* JumpyBall.exe Blit_Core 0x00021a98 treats 0xffff as "no colour key". */
#define FF_NO_KEY 0xffffu

/* JumpyBall.exe Gfx_CreateBackBuffer 0x00021698 builds three scale-step tables:
   0x00068518 (source span 100, stride 200, widths 1..199),
   0x00065ed0 (source span 64,  stride 70,  widths 1..63),
   0x00064b48 (source span 44,  stride 50,  widths 1..49). */
void Gfx_BuildScaleTables(void);

/* JumpyBall.exe Color_Pack16 0x00023bb4 - COLORREF 0x00BBGGRR to RGB565. */
uint16_t Color_Pack16(const ff_surface *dst, uint32_t colorref);

/* JumpyBall.exe Color_Pack16If16bpp 0x00024030 - Color_Pack16 gated on the
   destination bpp word at +0x18 being 0x10, otherwise 0. */
uint16_t Color_Pack16If16bpp(const ff_surface *dst, uint32_t colorref);

/* JumpyBall.exe Color_Blend 0x00023c3c - blend is a 0..100 percentage and it
   weights the DESTINATION pixel; 0 returns the source unchanged. */
uint16_t Color_Blend(const ff_surface *dst, uint16_t src, uint16_t dstpix, int blend);

/* JumpyBall.exe FUN_00023ddc, the pixel operation of Blit_Glyph_Core: the
   source pixel's blue channel is the coverage and (r,g,b) is the tint. */
uint16_t Color_MaskTint(const ff_surface *dst, uint16_t src, uint16_t dstpix,
                        int r, int g, int b);

/* JumpyBall.exe Blit_Core 0x00021a98, reached through Blit_NoKey 0x00023a3c. */
int Blit_NoKey(const ff_surface *dst, int x, int y, int w, int h,
               const ff_sprite *src, int src_x, int src_y);

/* JumpyBall.exe Blit_Core 0x00021a98 with an explicit colour key. */
int Blit_Core(const ff_surface *dst, int x, int y, int w, int h,
              const ff_sprite *src, int src_x, int src_y, unsigned key);

/* JumpyBall.exe Blit_Keyed 0x000239f8 -> Blit_Keyed_Core 0x00022574; the
   wrapper passes (blend, key) ahead of (src_x, src_y). */
int Blit_Keyed(const ff_surface *dst, int x, int y, int w, int h,
               const ff_sprite *src, int blend, unsigned key,
               int src_x, int src_y);

/* JumpyBall.exe Blit_Glyph 0x000239ac -> Blit_Glyph_Core 0x00022288. */
int Blit_Glyph(const ff_surface *dst, int x, int y, int w, int h,
               const ff_sprite *src, int r, int g, int b,
               int src_x, int src_y);

/* JumpyBall.exe Blit_TileH 0x00023aa8 -> Blit_TileH_Core 0x00022e7c: one
   destination scanline, source row src_row, horizontally scaled from 100 px
   through the 0x00068518 table.  Returns the table row it selected. */
int Blit_TileH(const ff_surface *dst, int x, int y, int w, int src_row,
               const ff_sprite *src, unsigned key);

/* JumpyBall.exe Blit_TileHV 0x00023a7c -> Blit_TileHV_Core 0x000228e8: the whole
   source scaled into w x h, both axes stepped through the 0x00068518 table. */
int Blit_TileHV(const ff_surface *dst, int x, int y, int w, int h,
                const ff_sprite *src, unsigned key);

/* JumpyBall.exe Blit_TileHV_Ofs 0x00023ad4 -> Blit_TileHV_Ofs_Core 0x000230f0:
   Blit_TileHV with every pixel written through Color_Blend 0x00023c3c, whose
   blend percentage is the wrapper's 8th argument. */
int Blit_TileHV_Ofs(const ff_surface *dst, int x, int y, int w, int h,
                    const ff_sprite *src, unsigned key, int blend);

/* JumpyBall.exe Blit_TileH_Ofs 0x00023b44 -> Blit_TileH_Ofs_Core 0x000236dc:
   Blit_TileH with every pixel written through Color_Blend 0x00023c3c. */
int Blit_TileH_Ofs(const ff_surface *dst, int x, int y, int w, int src_row,
                   const ff_sprite *src, unsigned key, int blend);

/* JumpyBall.exe Blit_Variant9 0x00023b08 -> Blit_Variant9_Core 0x00021dd0:
   Blit_TileHV with every pixel written through FUN_00023ddc 0x00023ddc and no
   colour key. */
int Blit_TileHV_Glyph(const ff_surface *dst, int x, int y, int w, int h,
                      const ff_sprite *src, int r, int g, int b);

/* JumpyBall.exe Blit 0x00023b78 -> Blit_Core 0x00021a98 with the colour key
   moved ahead of (src_x, src_y). */
int Blit(const ff_surface *dst, int x, int y, int w, int h,
         const ff_sprite *src, unsigned key, int src_x, int src_y);

/* FFRace.exe FUN_00056824 0x00056824 - each of the four neighbours that is not
   the colour key carries weight 1 and the destination carries the rest of 5. */
uint16_t Color_EdgeMean(const ff_surface *dst, uint16_t dstpix,
                        uint16_t left, uint16_t right,
                        uint16_t up, uint16_t down, unsigned key);

/* FFRace.exe Blit_EdgeBlend 0x0005a884 -> 0x00056a04; the wrapper's stack
   shuffle is byte-identical to Blit 0x0005a8c0's. */
int Blit_EdgeBlend(const ff_surface *dst, int x, int y, int w, int h,
                   const ff_sprite *src, unsigned key, int src_x, int src_y);

/* FFRace.exe 0x0005a78c -> 0x00059334: the source is one 0x40 x 0x2c cell at
   src_x, scaled through 0x000a8bc0 / 0x000a7838, and h is dead. */
int Blit_Scale64(const ff_surface *dst, int x, int y, int w, int h,
                 const ff_sprite *src, unsigned key, int src_x);

/* FFRace.exe 0x0005a7c0 -> 0x000596d4: one 0x40 x 0x2c source cell at src_x
   scaled through 0x000a8bc0 / 0x000a7838; 0x00059744 overwrites the h slot
   before it is read, and the inner loop 0x00059978..0x000599c0 has no key test
   and no branch - every pixel goes through 0x0005ab24. */
int Blit_Scale64Tint(const ff_surface *dst, int x, int y, int w, int h,
                     const ff_sprite *src, int r, int g, int b, int src_x);

#endif /* FF_GFX_H */
