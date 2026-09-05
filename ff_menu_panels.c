/* Screen panels ported from FFRace.exe (imagebase 0x00010000). */
#include "ff_menu_panels.h"

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_race.h"
#include "ff_screen.h"
#include "ff_ship.h"
#include "ff_text.h"

/* FFRace.exe 0x00039b1c passes the stat itself as the width of its three
   0x000a468c blits and 0x3c - the stat as their src_x, at height 5. */
#define FF_STAT_H    5
#define FF_STAT_SPAN 0x3c
#define FF_STAT_X    0x85

/* FFRace.exe .data 0x00083358, the stat its 0x00039b1c 0x00083488 == 6 block
   bars under the "Handling" row of panel 0x12c, nine entries 0x28 past
   0x00083330. */
static const int ff_ship_handling[FF_SHIP_COUNT] = {
    0, 60, 53, 45, 38, 30, 30, 31, 30
};

/* FFRace.exe TrackName 0x000165f8, eleven entries with 0 the random track. */
static const char *const ff_track_name[11] = {
    "Random", "Ornixan", "Dakhonhill", "Sulfonys", "Zeroxan", "Midwest",
    "Zone 528", "Scifloor", "The Earth", "Salthiem", "Winter Park"
};

/* FFRace.exe 0x00039b1c subtracts 0x000a7630 * 0.8, and a further 9 on the 0xb0
   layout, from every y of its 0x00083488 == 0x33 block. */
static int Drift(double base)
{
    return (int)(base - (double)Screen_Layout() * 0.8
                 - (double)(Menu_Small() * 9));
}

static const char *TrackName(int track)
{
    if (track < 0)
        track = 0;
    if (track >= 11)
        track = 10;

    return ff_track_name[track];
}

void MenuPanels_Track(const ff_surface *dst, int centre)
{
    const ff_sprite *panel  = AppAssets_Sprite(dst, FF_RES_TRACK_PANEL);
    const ff_sprite *arrows = AppAssets_Sprite(dst, FF_RES_ARROWS);
    int              small  = Menu_Small();
    int              track  = Screen_Track();
    int              locked;
    int              c;

    /* FFRace.exe 0x00039b1c recomputes 0x00016b78(0x000a77d0) + 1 < 0x000a76c0
       for each of the three colour terms of its track name. */
    locked = Race_ThemeUnlocked(Screen_CareerRace()) + 1 < track;
    c      = locked * -0x78 + 0xff;

    if (panel != 0)
        Blit_Keyed(dst, centre - 0x47, Drift((double)(0xc9 - 0x14 * small)),
                   0x8e, 0x1a, panel, 0x19, Menu_Key(), 0, 0);

    Text_DrawCentered(dst, centre, Menu_LabelY(1), TrackName(track), 1,
                      c, c, c);

    /* FFRace.exe 0x00039b1c draws row 0x10 of 0x000a46a8 above the name and row
       0x10 .. 0x20 below it, both tinted 0x0a 0x0a 0x46. */
    if (arrows != 0) {
        Blit_Glyph(dst, centre - 0x18, Drift(74.0), 0x30, 0x10, arrows,
                   0x0a, 0x0a, 0x46, 0, 0);
        Blit_Glyph(dst, centre - 0x18, Drift((double)((0xe - small) * 10)),
                   0x30, 0x10, arrows, 0x0a, 0x0a, 0x46, 0, 0x10);
    }

    Text_DrawCentered(dst, centre, Menu_LabelY(FF_MENU_EXIT), "Race now!", 1,
                      0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre, Menu_CaptionY(243.0),
                      "Choose the track you", 1, 0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre, Menu_CaptionY(258.0), "want to race on", 1,
                      0xff, 0xff, 0xff);
}

static int Handling(int ship)
{
    if (ship < 0)
        ship = 0;
    if (ship >= FF_SHIP_COUNT)
        ship = FF_SHIP_COUNT - 1;

    return ff_ship_handling[ship];
}

static void Stat(const ff_surface *dst, int y, int stat)
{
    const ff_sprite *strip = AppAssets_Sprite(dst, FF_RES_STRIP_SPEED);

    if (strip == 0)
        return;

    Blit(dst, Menu_Small() * -0x20 + FF_STAT_X, y - Screen_Layout(), stat,
         FF_STAT_H, strip, Menu_Key(), FF_STAT_SPAN - stat, 0);
}

void MenuPanels_Ship(const ff_surface *dst, int centre)
{
    const ff_sprite *panel  = AppAssets_Sprite(dst, FF_RES_SHIP_PANEL);
    int              layout = Screen_Layout();
    int              small  = Menu_Small();
    int              ship   = Physics_Ship();

    if (panel != 0)
        Blit_Keyed(dst, small * -0x20 + 0x23, 0x44 - layout, 0xaa, 0xa0, panel,
                   0x19, FF_NO_KEY, 0, 0);

    /* FFRace.exe 0x00039b1c takes 0x000a4508 row 0x000832f8 * 0x40 - 0x40 in
       place of the ship while 0x000833a4 < 0x000832f8. */
    if (Physics_ShipsUnlocked() < ship) {
        const ff_sprite *shadow = AppAssets_Sprite(dst, FF_RES_SHADOW);

        if (shadow != 0)
            Blit_Glyph(dst, centre - 0x20, 0x6c - layout, 0x40, 0x2c, shadow,
                       0, 0, 0, ship * 0x40 - 0x40, 0);
    } else if (ship >= 1 && ship <= FF_SHIP_SPRITES) {
        const ff_sprite *hull = AppAssets_Sprite(dst, Ship_SpriteRes(ship - 1));
        int              lean = Physics_Input(FF_INPUT_LEFT)
                                + Physics_Input(FF_INPUT_RIGHT) * 2;

        if (hull != 0)
            Blit_Scale64(dst, centre - 0x20, 0x6c - layout, 0x40, 0x2c, hull,
                         Menu_Key(), lean * 0x40);
    }

    Stat(dst, 0xb3, Physics_ShipTop(ship));
    Stat(dst, 0xc2, Physics_ShipAccel(ship));
    Stat(dst, 0xd1, Handling(ship));

    /* FFRace.exe 0x00039b1c gates both hint lines on 0x000833a4 < 6 and
       0x000a7630 == 0. */
    if (Physics_ShipsUnlocked() < 6 && layout == FF_LAYOUT_240x320) {
        Text_DrawCentered(dst, centre, 0xf3, "Unlock more ships", 1,
                          0xff, 0xff, 0xff);
        Text_DrawCentered(dst, centre, 0x102, "in career mode", 1,
                          0xff, 0xff, 0xff);
    }
}
