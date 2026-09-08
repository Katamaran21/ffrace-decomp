/* End-of-race panel ported from FFRace.exe (imagebase 0x00010000). */
#include "ff_results.h"

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_lap.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_race.h"
#include "ff_racer.h"
#include "ff_screen.h"
#include "ff_ship.h"
#include "ff_text.h"

/* FFRace.exe 0x0004cd10 .. 0x0004cd3c blits 0x000a4634 as 0xaa x 0xa0 at
   0x23 - shift, 0x30 - 0x000a7630 through 0x0005a540 with 0x19 as the blend and
   the 0xff00 | 0xff of 0x0004cce4 .. 0x0004cd04 as the key. */
#define FF_RS_W     0xaa
#define FF_RS_H     0xa0
#define FF_RS_X     0x23
#define FF_RS_Y     0x30
#define FF_RS_BLEND 0x19

/* FFRace.exe 0x0004cf0c .. 0x0004cf20 sets 1 for 0x000a7630 == 0x14 and 0 for
   anything else, then shifts it left by 5. */
static int Results_Shift(void)
{
    return (Screen_Layout() == FF_LAYOUT_176x220) << 5;
}

/* FFRace.exe 0x0004cd80, 0x0004cdd8, 0x0004ced4 and 0x0004d558 all place the
   headline at 0x39 - 0x000a7630, centred on 0x000832b0 and black, and gate it on
   the remainder of __rt_sdiv(2, 0x000a7728). */
#define FF_RS_TITLE_Y 0x39

/* FFRace.exe 0x0004cf08 multiplies 0x00085468 + slot * 4 by 0x19, 0x0004cf30
   adds 0x50 and 0x0004cf70 adds 0x4f, and 0x0004cf38 and 0x0004cf5c place the
   columns at 0x2d - shift and 0x7d - shift. */
#define FF_RS_ROW_PITCH 0x19
#define FF_RS_LABEL_Y   0x50
#define FF_RS_TIME_Y    0x4f
#define FF_RS_LABEL_X   0x2d
#define FF_RS_TIME_X    0x7d

/* FFRace.exe 0x00046594 and 0x0004d5f8 onwards pass 0xb as the Text_DrawGlyph
   0x00013548 field and step every digit column by 10. */
#define FF_RS_FIELD 0x0b
#define FF_RS_PITCH 10

/* FFRace.exe 0x00046594 adds 2 to its row, splits each field with __rt_sdiv 10
   into glyphs at +0, +0xa, +0x19, +0x23, +0x32 and +0x3c, and centres 0x0008458c
   at +0x18 and 0x00084588 at +0x30. */
static void Results_Time(const ff_surface *dst, int m, int s, int hs, int x,
                         int y, int r, int g, int b)
{
    y += 2;

    Text_DrawGlyph(dst, x, y, FF_RS_FIELD, r, g, b, m / 10);
    Text_DrawGlyph(dst, x + 0x0a, y, FF_RS_FIELD, r, g, b, m % 10);
    Text_DrawCentered(dst, x + 0x18, y, ":", 1, r, g, b);
    Text_DrawGlyph(dst, x + 0x19, y, FF_RS_FIELD, r, g, b, s / 10);
    Text_DrawGlyph(dst, x + 0x23, y, FF_RS_FIELD, r, g, b, s % 10);
    Text_DrawCentered(dst, x + 0x30, y, ".", 1, r, g, b);
    Text_DrawGlyph(dst, x + 0x32, y, FF_RS_FIELD, r, g, b, hs / 10);
    Text_DrawGlyph(dst, x + 0x3c, y, FF_RS_FIELD, r, g, b, hs % 10);
}

/* FFRace.exe 0x0004d5f8 onwards takes each digit as the remainder of
   __rt_sdiv(10, __rt_sdiv(div, value)) and draws it white. */
static void Results_Digits(const ff_surface *dst, int value, int div, int count,
                           int x, int y)
{
    int i;

    for (i = 0; i < count; i++) {
        Text_DrawGlyph(dst, x + i * FF_RS_PITCH, y, FF_RS_FIELD, 0xff, 0xff,
                       0xff, value / div % 10);
        div /= 10;
    }
}

/* FFRace.exe 0x0004cefc leaves a slot whose 0x00085468 entry is still 6, and
   0x0004cf44 takes the time from 0x000902b8, 0x000902a0 and 0x00090288 in
   0x82, 0xa5, 0xd2. */
static void Results_Row(const ff_surface *dst, int slot, const char *label,
                        int r, int g, int b)
{
    int order = Lap_Order(slot);
    int shift;

    if (order == FF_LAP_NO_ORDER)
        return;

    shift = Results_Shift();
    order = order * FF_RS_ROW_PITCH - Screen_Layout();

    Text_DrawCentered(dst, FF_RS_LABEL_X - shift, order + FF_RS_LABEL_Y, label,
                      0, r, g, b);
    Results_Time(dst, Lap_Minutes(slot), Lap_Seconds(slot),
                 Lap_Hundredths(slot), FF_RS_TIME_X - shift,
                 order + FF_RS_TIME_Y, 0x82, 0xa5, 0xd2);
}

static void Results_Panel(const ff_surface *dst)
{
    const ff_sprite *sprite = AppAssets_Sprite(dst, FF_RES_RESULTS);

    if (sprite == 0)
        return;

    Blit_Keyed(dst, FF_RS_X - Results_Shift(), FF_RS_Y - Screen_Layout(),
               FF_RS_W, FF_RS_H, sprite, FF_RS_BLEND, FF_NO_KEY, 0, 0);
}

/* FFRace.exe 0x0004cd40 .. 0x0004cf00 picks 0x00084628 for an even remainder
   with 0x000a77c0 at 0, 0x0008461c for an odd one above it, and 0x00084610 or
   0x00084604 on 0x0008546c; 0x0004cf9c walks the four opponent rows through
   0x00016cec with 0x000a77ec + slot - 1. */
static void Results_Scripted(const ff_surface *dst)
{
    int centre = Menu_CentreX();
    int y      = FF_RS_TITLE_Y - Screen_Layout();
    int blink  = Physics_Countdown() % 2;
    int wreck  = Ship_WreckCount();
    int order  = Lap_Order(FF_RACER_PLAYER);
    int slot;

    Results_Panel(dst);

    if (blink == 0 && wreck == 0)
        Text_DrawCentered(dst, centre, y, "RACE COMPLETED", 1, 0, 0, 0);
    if (blink == 1 && wreck > 0)
        Text_DrawCentered(dst, centre, y, "GAME OVER", 1, 0, 0, 0);
    if (blink == 1 && wreck == 0 && order == 0)
        Text_DrawCentered(dst, centre, y, "YOU WON!", 1, 0, 0, 0);
    if (blink == 1 && wreck == 0 && order > 0)
        Text_DrawCentered(dst, centre, y, "YOU LOSE...", 1, 0, 0, 0);

    Results_Row(dst, FF_RACER_PLAYER, "PLAYER", 0xfa, 0xfa, 0xfa);
    for (slot = FF_RACER_FIRST; slot < FF_RACER_SLOTS; slot++)
        Results_Row(dst, slot, Racer_Name(Racer_Base() + slot - 1), 0x78, 0xa0,
                    0xd2);
}

/* FFRace.exe 0x0004d4c0 .. 0x0004d5d0 takes 0x0008461c for an even remainder,
   then 0x000845ec or 0x0008461c on 0x000a7784 against 0x000a77a8; 0x0004d5f8
   labels 0x00084648, 0x000845e4 and 0x000845dc at 0x2d - shift on rows 0x50,
   0x69 and 0x82, and centres 0x000845cc and 0x000845bc at 0x9b and 0xb4. */
#define FF_RS_REC_LABEL_Y  0x50
#define FF_RS_REC_FLAG_Y   0x69
#define FF_RS_REC_CODE_Y   0x82
#define FF_RS_REC_HINT_Y   0x9b
#define FF_RS_REC_HOST_Y   0xb4
#define FF_RS_REC_DIGIT_X  0x91
#define FF_RS_REC_CODE_X   0x69

/* FFRace.exe 0x0004da8c .. 0x0004daa0 forms 0x144 | 3 and multiplies
   0x000a7784 + 0x10000 by it, and 0x0004dab4 divides the leftmost column of the
   result by 0xe before the eight 10000000 .. 1 columns. */
#define FF_RS_CODE_BIAS 0x10000
#define FF_RS_CODE_MUL  0x147
#define FF_RS_CODE_DIV  0x0e

static void Results_Endless(const ff_surface *dst)
{
    int centre = Menu_CentreX();
    int layout = Screen_Layout();
    int shift  = Results_Shift();
    int blink  = Physics_Countdown() % 2;
    int seg    = Physics_EndSegment();
    int flag   = Physics_EndFlag();
    int code   = (seg + FF_RS_CODE_BIAS) * FF_RS_CODE_MUL;
    int y      = FF_RS_TITLE_Y - layout;

    Results_Panel(dst);

    if (blink == 0)
        Text_DrawCentered(dst, centre, y, "GAME OVER", 1, 0, 0, 0);
    if (blink == 1 && seg >= flag)
        Text_DrawCentered(dst, centre, y, "NEW RECORD!", 1, 0, 0, 0);
    if (blink == 1 && seg < flag)
        Text_DrawCentered(dst, centre, y, "GAME OVER", 1, 0, 0, 0);

    Text_DrawCentered(dst, FF_RS_LABEL_X - shift, FF_RS_REC_LABEL_Y - layout,
                      "PLAYER", 0, 0xfa, 0xfa, 0xfa);
    Text_DrawCentered(dst, FF_RS_LABEL_X - shift, FF_RS_REC_FLAG_Y - layout,
                      "RECORD", 0, 0x78, 0xa0, 0xd2);
    Text_DrawCentered(dst, FF_RS_LABEL_X - shift, FF_RS_REC_CODE_Y - layout,
                      "Code", 0, 0xfa, 0xfa, 0xfa);
    Text_DrawCentered(dst, centre, FF_RS_REC_HINT_Y - layout, "Submit it at:", 1,
                      0x78, 0xa0, 0xd2);
    Text_DrawCentered(dst, centre, FF_RS_REC_HOST_Y - layout, "pocketnew.net", 1,
                      0xfa, 0xfa, 0xfa);

    Results_Digits(dst, seg, 10000, 5, FF_RS_REC_DIGIT_X - shift,
                   FF_RS_REC_LABEL_Y - layout);
    Results_Digits(dst, flag, 10000, 5, FF_RS_REC_DIGIT_X - shift,
                   FF_RS_REC_FLAG_Y - layout);

    Text_DrawGlyph(dst, FF_RS_REC_CODE_X - shift, FF_RS_REC_CODE_Y - layout,
                   FF_RS_FIELD, 0xff, 0xff, 0xff, code / FF_RS_CODE_DIV % 10);
    Results_Digits(dst, code, 10000000, 8,
                   FF_RS_REC_CODE_X + FF_RS_PITCH - shift,
                   FF_RS_REC_CODE_Y - layout);
}

/* FFRace.exe 0x0004dec0 .. 0x0004df9c takes 0x00084610 or 0x00084604 on an odd
   remainder by comparing 0x000a7668 against 0x000a7784, labels 0x00084638 at
   0x2d - shift on row 0x50 and times 0x000902bc at 0x7d - shift on row 0x4f. */
static void Results_Chase(const ff_surface *dst)
{
    int centre = Menu_CentreX();
    int layout = Screen_Layout();
    int shift  = Results_Shift();
    int y      = FF_RS_TITLE_Y - layout;

    Results_Panel(dst);

    if (Physics_Countdown() % 2 == 1) {
        if (Physics_EndChase() < Physics_EndSegment())
            Text_DrawCentered(dst, centre, y, "YOU WON!", 1, 0, 0, 0);
        if (Physics_EndSegment() < Physics_EndChase())
            Text_DrawCentered(dst, centre, y, "YOU LOSE...", 1, 0, 0, 0);
    }

    Text_DrawCentered(dst, FF_RS_LABEL_X - shift, FF_RS_LABEL_Y - layout, "TIME",
                      0, 0xfa, 0xfa, 0xfa);
    Results_Time(dst, Lap_Minutes(FF_RACER_PLAYER), Lap_Seconds(FF_RACER_PLAYER),
                 Lap_Hundredths(FF_RACER_PLAYER), FF_RS_TIME_X - shift,
                 FF_RS_TIME_Y - layout, 0x82, 0xa5, 0xd2);
}

/* FFRace.exe 0x0004ccc8 admits mode 0 on 0x000a77cc == 1 or 0x000a77c0 above 0,
   and 0x0004d4a4 and 0x0004dea4 admit modes 1 and 2 on 0x000a77cc == 1
   alone. */
void Results_Frame(const ff_surface *dst)
{
    int mode = Race_Mode();
    int done = Physics_Finished() == 1;

    if (mode == FF_RACE_SCRIPTED) {
        if (done || Ship_WreckCount() > 0)
            Results_Scripted(dst);
        return;
    }
    if (mode == FF_RACE_ENDLESS) {
        if (done)
            Results_Endless(dst);
        return;
    }
    if (mode == FF_RACE_ENDLESS_2 && done)
        Results_Chase(dst);
}
