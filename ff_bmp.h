#ifndef FF_BMP_H
#define FF_BMP_H

#include "ff_gfx.h"

/* Reproduces the JumpyBall.exe Sprite_LoadResourceBitmap 0x00021a04 ->
   Bitmap_LockBits 0x00024054 -> Surface_ImportPixels 0x00023f30 chain: the
   result is w*h packed RGB565 pixels, stride w, top row first. */
int Bmp_LoadSprite(const ff_surface *dst, const char *path, ff_sprite *out);

void Bmp_FreeSprite(ff_sprite *spr);

#endif /* FF_BMP_H */
