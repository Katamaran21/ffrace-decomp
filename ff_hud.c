/* Race overlay ported from FFRace.exe (imagebase 0x00010000). */
#include "ff_hud.h"

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_ingame.h"
#include "ff_lap.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_race.h"
#include "ff_racer.h"
#include "ff_results.h"
#include "ff_screen.h"
#include "ff_text.h"

/* FFRace.exe 0x00046754 blits 0x000a463c as 0x96 x 0x32 at 0x000832b0 - 0x4b,
   0x21 - 0x000a7630. */
#define FF_GO_W  0x96
#define FF_GO_H  0x32
#define FF_GO_DX 0x4b
#define FF_GO_Y  0x21

/* FFRace.exe 0x00046754 blits 0x000a4638 as 100 x 100 at 0x000832b0 - 0x32,
   8 - 0x000a7630. */
#define FF_DIGIT_W  100
#define FF_DIGIT_H  100
#define FF_DIGIT_DX 0x32
#define FF_DIGIT_Y  8

/* FFRace.exe 0x00046754 passes 0xb as the Text_DrawGlyph 0x00013548 field and
   steps the clock and score columns by 10. */
#define FF_HUD_FIELD 0xb
#define FF_HUD_PITCH 10

/* FFRace.exe 0x00046754 draws 0x000a772c then 0x000a7728, each as __rt_sdiv 10
   quotient and remainder, at 0x000832b0 - 0x19, -0xf, +5 and +0xf. */
static void Clock(const ff_surface *dst, int x, int y)
{
    int m = Ingame_Minutes();
    int s = Physics_Countdown();

    Text_DrawGlyph(dst, x - 0x19, y, FF_HUD_FIELD, 0xff, 0xff, 0xff, m / 10);
    Text_DrawGlyph(dst, x - 0x0f, y, FF_HUD_FIELD, 0xff, 0xff, 0xff, m % 10);
    Text_DrawGlyph(dst, x + 0x05, y, FF_HUD_FIELD, 0xff, 0xff, 0xff, s / 10);
    Text_DrawGlyph(dst, x + 0x0f, y, FF_HUD_FIELD, 0xff, 0xff, 0xff, s % 10);
}

/* FFRace.exe 0x00046754: 0x000a779c == 1 draws five __rt_sdiv digits of
   0x000a76bc, otherwise 0x00084640 at the first column and 0x000a77c4 at the
   fifth. */
static void Standing(const ff_surface *dst, int x, int y)
{
    int score;
    int div;
    int i;

    if (Race_Mode() == FF_RACE_ENDLESS) {
        score = Race_SegCounter();
        div   = 10000;
        for (i = 0; i < 5; i++) {
            Text_DrawGlyph(dst, x + i * FF_HUD_PITCH, y, FF_HUD_FIELD, 0xff,
                           0xff, 0xff, score / div % 10);
            div /= 10;
        }
        return;
    }
    Text_DrawCentered(dst, x, y, "POS:", 0, 0xff, 0xff, 0xff);
    Text_DrawGlyph(dst, x + 4 * FF_HUD_PITCH, y, FF_HUD_FIELD, 0xff, 0xff, 0xff,
                   Race_Rank());
}

/* FFRace.exe 0x00046754 at 0x0004ae30 steps the standings rows by 0xf from 0x14
   and labels each at 5; its 0x0004af1c row columns 0x5a, 0x65, 0x70, 0x7b, 0x80
   and 0x8b, and its 0x0004bf74 row columns 0x53, 0x5e, 0x69, 0x74, 0x79 and
   0x84, both at y 0x23. */
#define FF_CP_ROW_PITCH 0x0f
#define FF_CP_ROW_BASE  0x14
#define FF_CP_LABEL_X   5
#define FF_CP_DELTA_X   0x5a
#define FF_CP_SMALL_X   0x53
#define FF_CP_SMALL_Y   0x23

/* FFRace.exe 0x00046754 passes 0xfffffffb, 0xfffffffd and 0xfffffffe to
   Text_DrawGlyph 0x00013548, whose 0x30 bias turns them into 0x2b, 0x2d and
   0x2e, and passes 5 as the field of its 0x7b and 0x74 columns. */
#define FF_CP_PLUS   (-5)
#define FF_CP_MINUS  (-3)
#define FF_CP_POINT  (-2)
#define FF_CP_POINT_FIELD 5

/* FFRace.exe 0x0004ae40 and 0x0004bf30 gate on __rt_udiv(500, 0x000a7654)
   leaving a remainder below 0xfa. */
#define FF_CP_BLINK_PERIOD 500
#define FF_CP_BLINK_ON     0xfa

/* FFRace.exe 0x0004adb8 and 0x0004be84 compare 0x000a772c * 0x3c + 0x000a7728
   against 0x000902bc * 0x3c + 0x000902a4 + 4. */
#define FF_CP_HOLD_SEC 4

/* FFRace.exe 0x0004ad7c and 0x0004be48 both need __rt_sdiv(0x000832c4 / 4,
   0x000a76b4) to leave a remainder above 1, the clock below 0x000902bc * 0x3c +
   0x000902a4 + 4, 0x000a76b4 between 0x000832c4 / 4 and 0x000832c4, and
   0x000a779c at 0. */
static int Checkpoint_Active(void)
{
    int quarter = Race_Length() / 4;
    int pos     = Race_Segment();

    if (quarter < 1)
        return 0;
    if (pos % quarter <= 1)
        return 0;
    if (Ingame_Minutes() * FF_MINUTE_SEC + Physics_Countdown() >=
        Lap_Minutes(FF_RACER_PLAYER) * FF_MINUTE_SEC +
            Lap_Seconds(FF_RACER_PLAYER) + FF_CP_HOLD_SEC)
        return 0;

    return pos > quarter && pos < Race_Length() &&
           Race_Mode() == FF_RACE_SCRIPTED;
}

/* FFRace.exe 0x0004af1c and 0x0004bf74 pick the glyph from 0x0008546c against
   0x00085468 + slot * 4, form the delta from 0x000902a0 and 0x00090288, and
   split it with __rt_sdiv 1000, 100, 10 and 10 across the columns +0, +0xb,
   +0x16, +0x21, +0x26 and +0x31. */
static void Checkpoint_Delta(const ff_surface *dst, int x, int y, int slot)
{
    int delta;
    int sign;

    if (Lap_Order(FF_RACER_PLAYER) < Lap_Order(slot)) {
        sign  = FF_CP_PLUS;
        delta = (Lap_Seconds(slot) - Lap_Seconds(FF_RACER_PLAYER)) * 100 -
                Lap_Hundredths(FF_RACER_PLAYER) + Lap_Hundredths(slot);
    } else {
        sign  = FF_CP_MINUS;
        delta = (Lap_Seconds(FF_RACER_PLAYER) - Lap_Seconds(slot)) * 100 +
                Lap_Hundredths(FF_RACER_PLAYER) - Lap_Hundredths(slot);
    }

    Text_DrawGlyph(dst, x, y, FF_HUD_FIELD, 0xff, 0xff, 0xff, sign);
    Text_DrawGlyph(dst, x + 0x0b, y, FF_HUD_FIELD, 0xff, 0xff, 0xff,
                   delta / 1000 % 10);
    Text_DrawGlyph(dst, x + 0x16, y, FF_HUD_FIELD, 0xff, 0xff, 0xff,
                   delta / 100 % 10);
    Text_DrawGlyph(dst, x + 0x21, y, FF_CP_POINT_FIELD, 0xff, 0xff, 0xff,
                   FF_CP_POINT);
    Text_DrawGlyph(dst, x + 0x26, y, FF_HUD_FIELD, 0xff, 0xff, 0xff,
                   delta / 10 % 10);
    Text_DrawGlyph(dst, x + 0x31, y, FF_HUD_FIELD, 0xff, 0xff, 0xff,
                   delta % 10);
}

/* FFRace.exe 0x0004ae10 draws 0x00084650 at 0x19, 5, 0x0004ae40 blinks
   0x00084648 on the player's row, and 0x0004ae88 walks slots 2 .. 5 naming each
   through 0x00016cec with 0x000a77ec + slot - 1. */
static void Checkpoint_Big(const ff_surface *dst)
{
    int order;
    int slot;

    Text_DrawCentered(dst, 0x19, 5, "CHECK POINT", 0, 0, 0, 0);

    order = Lap_Order(FF_RACER_PLAYER);
    if (order != FF_LAP_NO_ORDER &&
        Ingame_SecondMs() % FF_CP_BLINK_PERIOD < FF_CP_BLINK_ON)
        Text_DrawCentered(dst, FF_CP_LABEL_X,
                          order * FF_CP_ROW_PITCH + FF_CP_ROW_BASE, "PLAYER",
                          0, 0, 0, 0);

    for (slot = FF_RACER_FIRST; slot < FF_RACER_SLOTS; slot++) {
        order = Lap_Order(slot);
        if (order == FF_LAP_NO_ORDER)
            continue;

        order = order * FF_CP_ROW_PITCH + FF_CP_ROW_BASE;
        Text_DrawCentered(dst, FF_CP_LABEL_X, order,
                          Racer_Name(Racer_Base() + slot - 1), 0, 0, 0, 0);
        Checkpoint_Delta(dst, FF_CP_DELTA_X, order, slot);
    }
}

/* FFRace.exe 0x0004bedc draws 0x00084650 centred at 0x000832b0, 5, 0x0004bf0c
   draws 0x00084638 at 0x20, 0x23 while 0x0008546c is not 6, and 0x0004bf58 takes
   the slots of 2 .. 5 whose 0x00085468 entry is 0, or 1 with 0x0008546c at 0. */
static void Checkpoint_Small(const ff_surface *dst)
{
    int player = Lap_Order(FF_RACER_PLAYER);
    int slot;

    Text_DrawCentered(dst, Menu_CentreX(), 5, "CHECK POINT", 1, 0, 0, 0);

    if (player != FF_LAP_NO_ORDER)
        Text_DrawCentered(dst, 0x20, FF_CP_SMALL_Y, "TIME", 0, 0, 0, 0);

    if (Ingame_SecondMs() % FF_CP_BLINK_PERIOD >= FF_CP_BLINK_ON)
        return;

    for (slot = FF_RACER_FIRST; slot < FF_RACER_SLOTS; slot++) {
        int order = Lap_Order(slot);

        if (order == 0 || (order == 1 && player == 0))
            Checkpoint_Delta(dst, FF_CP_SMALL_X, FF_CP_SMALL_Y, slot);
    }
}

/* FFRace.exe 0x00046754 for 0x000a7630 == 0: 0x0008458c at 0x000832b0,
   0x000a4584 / 0x000a4588 as 0x40 wide at 5, 0x114 with 0x000a7670 / 1000 + 1
   rows, 0x000a458c / 0x000a4590 as 0x46 x 0x28 at 0xa5, 0x113 filled
   0x000a42dc * 5.0 + 7.0; 0x000a42dc * 31.0 black at 0xba, 0xc4, 0xce. */
static void Hud_Big(const ff_surface *dst)
{
    const ff_sprite *sprite;
    int              centre  = Menu_CentreX();
    unsigned         key     = Menu_Key();
    float            derived = Physics_Derived();
    int              speed   = (int)(derived * 31.0f);

    if (Checkpoint_Active())
        Checkpoint_Big(dst);

    Text_DrawCentered(dst, centre, 0x117, ":", 1, 0xff, 0xff, 0xff);
    Clock(dst, centre, 0x118);
    Standing(dst, centre - 0x1a, 0x12e);

    sprite = AppAssets_Sprite(dst, FF_RES_DAMAGE_BACK);
    if (sprite != 0)
        Blit(dst, 5, 0x114, 0x40, 0x26, sprite, key, 0, 0);
    sprite = AppAssets_Sprite(dst, FF_RES_DAMAGE_FILL);
    if (sprite != 0)
        Blit(dst, 5, 0x114, 0x40, Physics_Damage() / 1000 + 1, sprite, key, 0,
             0);

    sprite = AppAssets_Sprite(dst, FF_RES_SPEED_BACK);
    if (sprite != 0)
        Blit(dst, 0xa5, 0x113, 0x46, 0x28, sprite, key, 0, 0);
    sprite = AppAssets_Sprite(dst, FF_RES_SPEED_FILL);
    if (sprite != 0)
        Blit(dst, 0xa5, 0x113, (int)(derived * 5.0f + 7.0f), 0x28, sprite, key,
             0, 0);

    Text_DrawGlyph(dst, 0xba, 0x120, FF_HUD_FIELD, 0, 0, 0, speed / 100);
    Text_DrawGlyph(dst, 0xc4, 0x120, FF_HUD_FIELD, 0, 0, 0, speed / 10 % 10);
    Text_DrawGlyph(dst, 0xce, 0x120, FF_HUD_FIELD, 0, 0, 0, speed % 10);
}

/* FFRace.exe 0x00046754 for 0x000a7630 != 0: 0x000a4684 as 0xb0 x 0x19 at 0,
   0xc3 keyed 0xffff, 0x000a4688 as 0x1c wide at 10, 0xcb with 0x000a7670 /
   0x960 + 1 rows, 0x000a468c as 5 rows at 0x7f, 0xc4 sized 0x000a42dc * 3.5
   from column 60.0 - that; 0x0008458c at 0x000832b0 - 0x000a7630 / 4, 0xce. */
static void Hud_Small(const ff_surface *dst)
{
    const ff_sprite *sprite;
    unsigned         key    = Menu_Key();
    double           bar    = (double)Physics_Derived() * 3.5;
    int              centre = Menu_CentreX() - Screen_Layout() / 4;

    if (Checkpoint_Active())
        Checkpoint_Small(dst);

    sprite = AppAssets_Sprite(dst, FF_RES_STRIP);
    if (sprite != 0)
        Blit(dst, 0, 0xc3, 0xb0, 0x19, sprite, FF_NO_KEY, 0, 0);
    sprite = AppAssets_Sprite(dst, FF_RES_STRIP_DAMAGE);
    if (sprite != 0)
        Blit(dst, 10, 0xcb, 0x1c, Physics_Damage() / 0x960 + 1, sprite, key, 0,
             0);
    sprite = AppAssets_Sprite(dst, FF_RES_STRIP_SPEED);
    if (sprite != 0)
        Blit(dst, 0x7f, 0xc4, (int)bar, 5, sprite, key, (int)(60.0 - bar), 0);

    Text_DrawCentered(dst, centre, 0xce, ":", 1, 0xff, 0xff, 0xff);
    Clock(dst, centre, 0xce);
    Standing(dst, 0x7d, 0xce);
}

/* FFRace.exe 0x0004d3e4 passes 0xf, and 0x0004e03c, 0x0004e070 and 0x0004e0a0
   each pass 0x1e, with no 0x000a7630 term. */
#define FF_COPS_TITLE_Y 0x0f
#define FF_COPS_NAG_Y   0x1e

/* FFRace.exe 0x0004d3a0 .. 0x0004d3d4 gates on 0x000a77f0 == 1,
   0x000832e4 + 4 < 0x000a7728 and 0x000a779c == 1; 0x0004d3f8 draws
   0x000845f8 centred on 0x000832b0 in black and 0x0004d418 leaves through
   0x00014924 with 0 and 1. */
static void Hud_Cops(const ff_surface *dst)
{
    int centre = Menu_CentreX();
    int sec    = Physics_Countdown();

    if (Race_Cops() != 1 || FF_COPS_SEC + 4 >= sec ||
        Race_Mode() != FF_RACE_ENDLESS)
        return;

    Text_DrawCentered(dst, centre, FF_COPS_TITLE_Y, "THE COPS :", 1, 0, 0, 0);

    if (FF_COPS_SEC + 0xc < sec)
        Screen_Set(FF_SCREEN_MAIN, 1);
    else if (FF_COPS_SEC + 8 < sec)
        Text_DrawCentered(dst, centre, FF_COPS_NAG_Y, "This is only a demo!", 1,
                          0, 0, 0);
    else if (FF_COPS_SEC + 6 < sec)
        Text_DrawCentered(dst, centre, FF_COPS_NAG_Y, "Stop now!", 1, 0, 0, 0);
    else
        Text_DrawCentered(dst, centre, FF_COPS_NAG_Y, "Hey!", 1, 0, 0, 0);
}

/* FFRace.exe 0x00046754 tail: 0x000a463c as 0x96 x 0x32 when 0x000a7728 == 0
   with 0x000a7654 != 0 and 0x000a772c == 0, then 0x000a4638 column
   (0x000a7728 + 3) * 100 when 0x000a7728 < 0, both tinted 0x000a77b8; its
   0x0004d388 skip of the first pair reaches 0x0004d38c, not the return. */
void Hud_Frame(const ff_surface *dst)
{
    const ff_sprite *sprite;
    int              layout = Screen_Layout();
    int              tint   = Race_SkyBlend();
    int              count  = Physics_Countdown();

    if (layout == FF_LAYOUT_240x320)
        Hud_Big(dst);
    else
        Hud_Small(dst);

    Results_Frame(dst);

    if (count == 0 && Ingame_SecondMs() != 0 && Ingame_Minutes() == 0) {
        sprite = AppAssets_Sprite(dst, FF_RES_GO);
        if (sprite != 0)
            Blit_Glyph(dst, Menu_CentreX() - FF_GO_DX, FF_GO_Y - layout, FF_GO_W,
                       FF_GO_H, sprite, tint, tint, tint, 0, 0);
    }
    if (count < 0) {
        sprite = AppAssets_Sprite(dst, FF_RES_COUNTDOWN);
        if (sprite != 0)
            Blit_Glyph(dst, Menu_CentreX() - FF_DIGIT_DX, FF_DIGIT_Y - layout,
                       FF_DIGIT_W, FF_DIGIT_H, sprite, tint, tint, tint,
                       (count + 3) * FF_DIGIT_W, 0);
    }

    Hud_Cops(dst);
}
