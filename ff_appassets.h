#ifndef FF_APPASSETS_H
#define FF_APPASSETS_H

#include "ff_gfx.h"

/* FFRace.exe Game_Init 0x000115b0 imports every BITMAP resource with
   LoadBitmapW and keeps the RGB565 sprite in one slot per resource id:
   0x000a45d4 holds 0xf7, 0x000a45e0 holds 0xfa, 0x000a45ec holds 0xfd. */
const ff_sprite *AppAssets_Sprite(const ff_surface *dst, int res);

int AppAssets_Preload(const ff_surface *dst, const int *res, int n);

void AppAssets_Free(void);

#endif /* FF_APPASSETS_H */
