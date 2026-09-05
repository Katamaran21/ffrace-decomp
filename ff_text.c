#include "ff_text.h"
#include <stddef.h>
#include "ff_assets.h"
#include "ff_bmp.h"
#include "ff_consts.h"

#define FF_GLYPH_N       256
#define FF_GLYPH_FIRST   33
#define FF_GLYPH_STOP    124
#define FF_MARKER_SCAN_W 0x4ba
#define FF_SPACE_W       5
#define FF_SPACE_X       9
#define FF_GLYPH_BIAS    0x30

static int              ff_glyph_w[FF_GLYPH_N];
static int              ff_glyph_x[FF_GLYPH_N];
static ff_sprite        ff_sheet_marker;
static ff_sprite        ff_sheet_column;
static const ff_sprite *ff_font;
static int              ff_font_which;

/* FFRace.exe Font_Load 0x00012f98 argument-1 branch walks row 0 for 0x4ba
   pixels and takes the 0xff00ff pixels in pairs, filling 0x00085000 and
   0x00084c00 from index 0x21 until the offset reaches 0x1f0. */
static void ScanMarkerPairs(const ff_surface *dst, const ff_sprite *font)
{
    uint16_t marker = Color_Pack16(dst, 0x00ff00ff);
    int      x, idx = FF_GLYPH_FIRST, open = 0;

    for (x = 0; x < FF_MARKER_SCAN_W && x < font->w; x++) {
        if (font->pixels[x] != marker)
            continue;
        if (open == 0) {
            ff_glyph_x[idx] = x + 1;
            open            = 1;
        } else {
            ff_glyph_w[idx] = x - ff_glyph_x[idx];
            open            = 0;
            if (++idx == FF_GLYPH_STOP)
                break;
        }
    }
}

/* FFRace.exe Font_Load 0x00012f98 else branch tests 14 rows of stride 0x4bc
   for the packed colour 0; such a column closes the open glyph, and while none
   is open every one of a run overwrites the anchor. */
static void ScanBlackColumns(const ff_surface *dst, const ff_sprite *font)
{
    uint16_t black = Color_Pack16(dst, 0);
    int      x, y, idx = FF_GLYPH_FIRST, gap = 0;

    for (x = 0; x < font->w && idx < FF_GLYPH_N; x++) {
        for (y = 0; y < font->h; y++) {
            if (font->pixels[y * font->w + x] != black)
                break;
        }
        if (y < font->h) {
            gap = 1;
            continue;
        }
        if (gap) {
            gap             = 0;
            ff_glyph_w[idx] = x - ff_glyph_x[idx];
            idx++;
        } else {
            ff_glyph_x[idx] = x + 1;
        }
    }
}

int Font_Load(const ff_surface *dst, int which)
{
    ff_sprite *sheet = (which == 1) ? &ff_sheet_marker : &ff_sheet_column;
    int        res   = (which == 1) ? FF_RES_FONT : FF_RES_FONT_COLUMN;

    if (sheet->pixels == NULL && !Bmp_LoadSprite(dst, Assets_Bitmap(res), sheet))
        return 0;

    if (which == 1)
        ScanMarkerPairs(dst, sheet);
    else
        ScanBlackColumns(dst, sheet);

    /* FFRace.exe Font_Load 0x00012f98 tail stores 5 to 0x00084c80 and 9 to
       0x00085080, the width and the source x of index 0x20. */
    ff_glyph_w[' '] = FF_SPACE_W;
    ff_glyph_x[' '] = FF_SPACE_X;

    ff_font = sheet;
    ff_font_which = which;
    return 1;
}

/* FFRace.exe Font_Load 0x00012f98 stores its argument in 0x0008348c, tested by
   0x00039b1c before it draws any text. */
int Font_Current(void)
{
    return ff_font_which;
}

void Font_Free(void)
{
    Bmp_FreeSprite(&ff_sheet_marker);
    Bmp_FreeSprite(&ff_sheet_column);
    ff_font = NULL;
}

void Text_DrawCentered(const ff_surface *dst, int x, int y, const char *s,
                       int align, int r, int g, int b)
{
    int         total = 0;
    int         pen   = 0;
    const char *p;
    int         c;

    if (ff_font == NULL || s == NULL)
        return;

    /* FFRace.exe Text_DrawCentered 0x00013460 measures only when align > 0 and
       indexes 0x00084c00 with the raw character. */
    if (align > 0) {
        for (p = s; *p != '\0'; p++)
            total += ff_glyph_w[(unsigned char)*p] * align;
        if (total < 0)
            total++;
    }

    for (p = s; *p != '\0'; p++) {
        c = (unsigned char)*p;
        Blit_Glyph(dst, pen + (x - (total >> 1)), y, ff_glyph_w[c], FF_GLYPH_H,
                   ff_font, r, g, b, ff_glyph_x[c], 0);
        pen += ff_glyph_w[c];
    }
}

void Text_DrawGlyph(const ff_surface *dst, int x, int y, int field,
                    int r, int g, int b, int code)
{
    int idx = code + FF_GLYPH_BIAS;

    if (ff_font == NULL || idx < 0 || idx >= FF_GLYPH_N)
        return;

    field -= ff_glyph_w[idx];
    if (field < 0)
        field++;

    Blit_Glyph(dst, x + (field >> 1), y, ff_glyph_w[idx], FF_GLYPH_H, ff_font,
               r, g, b, ff_glyph_x[idx], 0);
}
