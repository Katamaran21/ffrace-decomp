#include "ff_ship.h"

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_ingame.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_race.h"
#include "ff_render.h"
#include "ff_screen.h"

/* FFRace.exe .data 0x00083438, twenty ints the 0x000483a0 y expression reads as
   (&DAT_00083438)[0x000a77d8]. */
static const int ff_bob[FF_BOB_PHASES] = {
    1, 2, 3, 4, 4, 4, 3, 2, 1, 0, -1, -2, -3, -4, -4, -4, -3, -2, -1, 0
};

/* FFRace.exe Game_Init 0x000115b0 fills 0x000a44e0 + 4 * 0x000832f8 for
   0x000832f8 of 1 .. 6 and leaves every other slot empty. */
static const int ff_ship_res[FF_SHIP_SPRITES] = {
    211, 210, 212, 213, 214, 343
};

static int ff_bob_phase;    /* 0x000a77d8 */
static int ff_wreck;        /* 0x000a77c0 */
static int ff_wreck_sound;

/* FFRace.exe 0x00046754 at 0x00048300 takes 0x000a4508 with (0x14, 0x14, 0x14)
   and 0x40 * 0x000832f8 - 0x40, and 0x000483a0 takes 0x000a44e0 + 4 *
   0x000832f8 keyed on 0x000a7800 with 0x40 * (0x00090279 + 2 * 0x00090281);
   both take 0x000832b0 + 0x000a766c - 0x20 as x. */
void Ship_Draw(const ff_surface *dst)
{
    int layout = Screen_Layout();
    int x      = Menu_CentreX() + Render_BaseX() - 0x20;
    int ship   = Physics_Ship();
    int lean, y;

    /* FFRace.exe 0x00046754 adds 1 to 0x000a77d8 and stores 0 when 0x13 is
       below the sum, inside the 0x000a779c == 0 block that also steps the four
       ints at 0x00085458. */
    if (Race_Mode() == FF_RACE_SCRIPTED) {
        ff_bob_phase++;
        if (0x13 < ff_bob_phase)
            ff_bob_phase = 0;
    }

    Blit_Glyph(dst, x, (0x68 - layout) * 2, 0x40, 0x2c,
               AppAssets_Sprite(dst, FF_RES_SHADOW), 0x14, 0x14, 0x14,
               ship * 0x40 - 0x40, 0);

    if (ship < 1 || ship > FF_SHIP_SPRITES)
        return;

    /* FFRace.exe 0x00048314 reaches 0x000483a0 with r1 = 0 and r2 = 1 while
       0x000a77c0 is 0, and 0x00048424 reaches the same setup with r1 = 1 and
       r2 = 0. */
    y = (((ff_wreck != 0) * 2 - layout) * 2
         - ff_bob[ff_bob_phase] * (ff_wreck == 0)) + 0xc1;

    lean = Physics_Input(FF_INPUT_LEFT) + Physics_Input(FF_INPUT_RIGHT) * 2;

    Blit_Scale64(dst, x, y, 0x40, 0x2c,
                 AppAssets_Sprite(dst, Ship_SpriteRes(ship - 1)), Menu_Key(),
                 lean * 0x40);
}

int Ship_Bob(int phase)
{
    return ff_bob[phase];
}

int Ship_SpriteRes(int index)
{
    return ff_ship_res[index];
}

/* FFRace.exe 0x00046754: 39999 < 0x000a7670 steps 0x000a77c0 down, blits
   0x000a456c 0x38 by 0x38 from row 0x38 * (7 - 0x000a77c0) at 0x000832b0 +
   0x000a766c - 0x1c, 2 * (0x5f - 0x000a7630), steps back up on a remainder of
   __rt_sdiv(2, 0x000a764c), stores 8 below 1, and stores 1 into 0x000a778c. */
void Ship_Wreck(const ff_surface *dst)
{
    int layout;

    if (Physics_Damage() <= 39999)
        return;

    if (ff_wreck == 0)
        ff_wreck_sound = 1;

    layout = Screen_Layout();
    ff_wreck--;

    Blit(dst, Menu_CentreX() + Render_BaseX() - 0x1c, (0x5f - layout) * 2,
         0x38, 0x38, AppAssets_Sprite(dst, FF_RES_WRECK), Menu_Key(),
         0, (7 - ff_wreck) * 0x38);

    if (Ingame_Frames() % 2 > 0)
        ff_wreck++;
    if (ff_wreck < 1)
        ff_wreck = 8;

    Physics_SetInput(FF_INPUT_BRAKE, 1);
}

int Ship_TakeWreckSound(void)
{
    int pending = ff_wreck_sound;

    ff_wreck_sound = 0;
    return pending;
}

int Ship_WreckCount(void)
{
    return ff_wreck;
}
