/* Main menu ported from FFRace.exe 0x00039b1c (imagebase 0x00010000). */
#include "ff_menu.h"

#include <stdlib.h>

#include "ff_appassets.h"
#include "ff_audio.h"
#include "ff_consts.h"
#include "ff_menu_screens.h"
#include "ff_screen.h"
#include "ff_text.h"

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW(hinst, id) -> slot: 0xf7 ->
   0x000a45d4, 0xf8 -> 0x000a45d8, 0xf9 -> 0x000a45dc, 0xfa -> 0x000a45e0,
   0xfc -> 0x000a45e8, 0xfd -> 0x000a45ec. */
#define FF_RES_SCANLINES     247
#define FF_RES_BANNER_BOTTOM 248
#define FF_RES_BANNER_TOP    249
#define FF_RES_PLATE         250
#define FF_RES_LOGO          252
#define FF_RES_PUBLISHER     253

/* FFRace.exe Game_Init 0x000115b0 seeds 50 entries of 0x00085480 / 0x00085548;
   0x00039b1c animates the first 0x14. */
#define FF_COL_N      50
#define FF_COL_ACTIVE 20
#define FF_COL_GLYPHS 9
#define FF_COL_PITCH  0x0c
#define FF_COL_PERIOD 0x0f
#define FF_COL_STEP   7
#define FF_COL_TOP    (-100)

/* FFRace.exe 0x00039b1c blits 0x000a45e0 as 0x8c x 0x18 at 0x000832b0 - 0x46. */
#define FF_PLATE_W  0x8c
#define FF_PLATE_H  0x18
#define FF_PLATE_DX 0x46

#define FF_BANNER_H 0x0c

/* FFRace.exe 0x00039b1c Text_DrawGlyph codes 0x20 0x1f 0x13 0x1b 0x15 0x24
   0x1e 0x15 0x27, tint 0 / 0x96 / 0xfa. */
static const int ff_col_code[FF_COL_GLYPHS] = {
    0x20, 0x1f, 0x13, 0x1b, 0x15, 0x24, 0x1e, 0x15, 0x27
};
#define FF_COL_R 0
#define FF_COL_G 0x96
#define FF_COL_B 0xfa

/* FFRace.exe 0x00039b1c Text_DrawCentered y bases 0x4c 0x6d 0x8e 0xaf 0xd0. */
static const int ff_label_y[FF_MENU_ITEMS] = { 0x4c, 0x6d, 0x8e, 0xaf, 0xd0 };

static int      ff_col_x[FF_COL_N];
static int      ff_col_y[FF_COL_N];
static int      ff_item_state[FF_MENU_ITEMS];
static int      ff_activated;
static int      ff_touch_x;
static int      ff_touch_y;
static int      ff_touch_code;
static uint16_t ff_key;

int Menu_Small(void)
{
    return Screen_Layout() == FF_LAYOUT_176x220;
}

int Menu_CentreX(void)
{
    return Menu_Small() ? FF_SMALL_VIEW_CENTER_X : FF_VIEW_CENTER_X;
}

uint16_t Menu_Key(void)
{
    return ff_key;
}

/* FFRace.exe 0x00039b1c plate y: ((layout == 0x14 ? -5 : 0) + 0x21) * i + base
   - layout * 0.8 - (layout == 0x14 ? 9 : 0).  The drift term reads
   0x00084988, which no instruction in the image writes. */
static double PlateY(int i, int base)
{
    return (double)((Menu_Small() * -5 + 0x21) * i + base)
           - (double)Screen_Layout() * 0.8 - (double)(Menu_Small() * 9);
}

int Menu_LabelY(int i)
{
    return (int)((double)(ff_label_y[i] - Menu_Small() * 5 * i)
                 - (double)Screen_Layout() * 0.8 - (double)(Menu_Small() * 9));
}

/* FFRace.exe 0x00039b1c caption y: base - layout * 2.6 - (layout == 0x14 ? 3 : 0). */
int Menu_CaptionY(double base)
{
    return (int)(base - (double)Screen_Layout() * 2.6
                 - (double)(Menu_Small() * 3));
}

int Menu_Init(const ff_surface *dst)
{
    static const int res[9] = { FF_RES_SCANLINES, FF_RES_BANNER_BOTTOM,
                                FF_RES_BANNER_TOP, FF_RES_PLATE,
                                FF_RES_LOGO,       FF_RES_PUBLISHER,
                                FF_RES_TRACK_PANEL, FF_RES_SHIP_PANEL,
                                FF_RES_ARROWS };
    int i;

    /* FFRace.exe Game_Init 0x000115b0: 0x000a7800 = 0x0005ad78(surface, 0x800080). */
    ff_key = Color_Pack16If16bpp(dst, 0x800080);

    if (!Font_Load(dst, 1))
        return 0;
    if (AppAssets_Preload(dst, res, 9) != 0)
        return 0;

    /* FFRace.exe Game_Init 0x000115b0: 0x00085480[i] = rand() % height - 100,
       0x00085548[i] = (rand() % 46) * 5. */
    for (i = 0; i < FF_COL_N; i++) {
        ff_col_y[i] = rand() % ff_clip_h + FF_COL_TOP;
        ff_col_x[i] = (rand() % 46) * 5;
    }
    return 1;
}

void Menu_Touch(int x, int y)
{
    ff_touch_x = x;
    ff_touch_y = y;
}

void Menu_Activate(int item)
{
    ff_touch_code = item + 2;
}

int Menu_TakeActivation(void)
{
    int a = ff_activated;

    ff_activated = 0;
    return a;
}

/* FFRace.exe 0x00039b1c: 0x000a7750 == i + 2, or the touch inside
   (centre +- 0x46) x (plate top, plate top + 0x18). */
static void HitTest(int i, int centre)
{
    double ty = (double)ff_touch_y;
    int    hit = 0;

    if (ff_touch_code == i + 2)
        hit = 1;
    else if (centre - FF_PLATE_DX < ff_touch_x && ff_touch_x < centre + FF_PLATE_DX &&
             ty > PlateY(i, 0x46) && ty < PlateY(i, 0x5e))
        hit = 1;
    if (hit) {
        /* FFRace.exe 0x0003a888 playSound(0x000a4db0, 0x000a4b30, 0x10000000)
           on the commit itself. */
        Audio_Play(FF_SND_MENU);
        ff_activated  = 1;
        Screen_SetParam(i);
        ff_touch_x    = 0;
        ff_touch_code = 0;
    }
}

static void Columns(const ff_surface *dst)
{
    int i, k;

    for (i = 0; i < FF_COL_ACTIVE; i++) {
        ff_col_y[i] += i % FF_COL_PERIOD + FF_COL_STEP;
        if (ff_col_y[i] > ff_clip_h) {
            ff_col_y[i] = FF_COL_TOP;
            ff_col_x[i] = rand() % (ff_clip_w - 9);
        }
        for (k = 0; k < FF_COL_GLYPHS; k++)
            Text_DrawGlyph(dst, ff_col_x[i], ff_col_y[i] + k * FF_COL_PITCH,
                           FF_GLYPH_H, FF_COL_R, FF_COL_G, FF_COL_B,
                           ff_col_code[k]);
    }
}

static void Plates(const ff_surface *dst, const ff_sprite *plate, int centre)
{
    int screen = Screen_Id();
    int gate   = MenuScreens_Plates(screen);
    int i, c, span, blend, y, reach;

    for (i = 0; i < FF_MENU_ITEMS; i++) {
        c = Screen_Transition();
        if (c == 0)
            ff_item_state[i] = 1000;
        if (c > 0) {
            if (ff_item_state[i] != 1000) {
                span  = ff_item_state[i] * (0x15 - c);
                blend = c * -3 + 100;
                y     = (int)PlateY(i, 0x46);
                Blit_Keyed(dst, span + centre - FF_PLATE_DX, y, FF_PLATE_W,
                           FF_PLATE_H, plate, blend, ff_key, 0, 0);
                Blit_Keyed(dst, centre - span - FF_PLATE_DX, y, FF_PLATE_W,
                           FF_PLATE_H, plate, blend, ff_key, 0, 0);
            }
            Screen_TransitionStep();
            c = Screen_Transition();
        }
        reach = gate && MenuScreens_Reach(screen, i);
        if (reach && MenuScreens_Plate(screen, i)) {
            Blit_Keyed(dst, centre - FF_PLATE_DX, (int)PlateY(i, 0x46),
                       FF_PLATE_W, FF_PLATE_H, plate, c * 3 + 0x19, ff_key,
                       0, 0);
            if (c == 0)
                ff_item_state[i] = rand() % 5 + 1;
        }
        if (reach)
            HitTest(i, centre);
    }
}

void Menu_Frame(const ff_surface *dst)
{
    const ff_sprite *scan, *top, *bottom, *plate, *logo, *publisher;
    int y, centre;

    if (Font_Current() != 1 && !Font_Load(dst, 1))
        return;
    scan      = AppAssets_Sprite(dst, FF_RES_SCANLINES);
    top       = AppAssets_Sprite(dst, FF_RES_BANNER_TOP);
    bottom    = AppAssets_Sprite(dst, FF_RES_BANNER_BOTTOM);
    plate     = AppAssets_Sprite(dst, FF_RES_PLATE);
    logo      = AppAssets_Sprite(dst, FF_RES_LOGO);
    publisher = AppAssets_Sprite(dst, FF_RES_PUBLISHER);
    if (!scan || !top || !bottom || !plate || !logo || !publisher)
        return;
    centre = Menu_CentreX();

    for (y = 0; y < ff_clip_h; y += 10)
        Blit(dst, 0, y, ff_clip_w, 10, scan, FF_NO_KEY, 0, 0);

    Columns(dst);

    Blit(dst, 0, 0, ff_clip_w, FF_BANNER_H, top, FF_NO_KEY, 0, 0);
    Blit(dst, 0, FF_BANNER_H, ff_clip_w, FF_BANNER_H, top, FF_NO_KEY, 0, 0);
    Blit(dst, 0, FF_BANNER_H * 2, ff_clip_w, FF_BANNER_H, top, FF_NO_KEY, 0, 0);
    if (Menu_Small())
        Blit(dst, 0, 0x1c, ff_clip_w, FF_BANNER_H, top, FF_NO_KEY, 0, 0);
    Blit(dst, 0, ff_clip_h - 0x24, ff_clip_w, FF_BANNER_H, bottom, FF_NO_KEY, 0, 0);
    Blit(dst, 0, ff_clip_h - 0x18, ff_clip_w, FF_BANNER_H, bottom, FF_NO_KEY, 0, 0);
    Blit(dst, 0, ff_clip_h - 0x0c, ff_clip_w, FF_BANNER_H, bottom, FF_NO_KEY, 0, 0);

    Blit_EdgeBlend(dst, centre - 0x5a, Menu_Small() * -7 + 6, 0xb4, 0x28, logo,
                   ff_key, 0, 0);
    Blit_EdgeBlend(dst, centre - 0x57, 0x125, 0xaf, 0x17, publisher,
                   ff_key, 0, 0);

    Plates(dst, plate, centre);

    MenuScreens_Draw(dst, Screen_Id(), Screen_Param(), centre);
}
