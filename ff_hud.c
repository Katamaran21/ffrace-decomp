/* Race overlay ported from FFRace.exe (imagebase 0x00010000). */
#include "ff_hud.h"

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_ingame.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_race.h"
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

/* FFRace.exe 0x00046754 tail: 0x000a463c as 0x96 x 0x32 when 0x000a7728 == 0
   with 0x000a7654 != 0 and 0x000a772c == 0, then 0x000a4638 column
   (0x000a7728 + 3) * 100 when 0x000a7728 < 0, both tinted 0x000a77b8. */
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

    if (count == 0) {
        if (Ingame_SecondMs() == 0 || Ingame_Minutes() != 0)
            return;
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
}
