#include "ff_appassets.h"
#include <stddef.h>
#include "ff_assets.h"
#include "ff_bmp.h"

#define FF_RES_N 512

static ff_sprite ff_cache[FF_RES_N];
static char      ff_tried[FF_RES_N];

const ff_sprite *AppAssets_Sprite(const ff_surface *dst, int res)
{
    if (res < 0 || res >= FF_RES_N)
        return NULL;

    if (!ff_tried[res]) {
        ff_tried[res] = 1;
        Bmp_LoadSprite(dst, Assets_Bitmap(res), &ff_cache[res]);
    }

    return (ff_cache[res].pixels != NULL) ? &ff_cache[res] : NULL;
}

int AppAssets_Preload(const ff_surface *dst, const int *res, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        if (AppAssets_Sprite(dst, res[i]) == NULL)
            return res[i];
    }
    return 0;
}

void AppAssets_Free(void)
{
    int i;

    for (i = 0; i < FF_RES_N; i++) {
        if (ff_cache[i].pixels != NULL)
            Bmp_FreeSprite(&ff_cache[i]);
        ff_tried[i] = 0;
    }
}
