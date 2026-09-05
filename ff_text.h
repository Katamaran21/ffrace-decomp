#ifndef FF_TEXT_H
#define FF_TEXT_H

#include "ff_gfx.h"

/* FFRace.exe Font_Load 0x00012f98 keeps its argument in 0x0008348c and loads
   one of two sheets: 1 takes BITMAP 330 and pairs the 0xff00ff markers in row
   0, anything else takes BITMAP 254 and takes every column that is black over
   all 14 rows as a separator.  Both fill 0x00084c00 and 0x00085000. */
int Font_Load(const ff_surface *dst, int which);

/* FFRace.exe 0x0008348c, the selector Font_Load 0x00012f98 last stored. */
int Font_Current(void);

void Font_Free(void);

/* FFRace.exe Text_DrawCentered 0x00013460, 237 call sites: align scales the
   measured width and 0 leaves the pen at x. */
void Text_DrawCentered(const ff_surface *dst, int x, int y, const char *s,
                       int align, int r, int g, int b);

/* FFRace.exe Text_DrawGlyph 0x00013548, 92 call sites: code indexes 0x00084cc0
   and 0x000850c0, the glyph tables biased by 0x30, and field is the box the
   glyph is centred in. */
void Text_DrawGlyph(const ff_surface *dst, int x, int y, int field,
                    int r, int g, int b, int code);

#endif /* FF_TEXT_H */
