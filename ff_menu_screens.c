/* Menu screen text ported from FFRace.exe 0x00039b1c (imagebase 0x00010000). */
#include "ff_menu_screens.h"

#include <stdio.h>

#include "ff_appassets.h"
#include "ff_consts.h"
#include "ff_menu.h"
#include "ff_menu_panels.h"
#include "ff_screen.h"
#include "ff_settings.h"
#include "ff_text.h"

/* FFRace.exe Game_Init 0x000115b0: LoadBitmapW(0x000a4854, 0x126) fills
   0x000a468c. */
#define FF_RES_BAR 294

/* FFRace.exe 0x0003ff34 / 0x000416d8 / 0x0004183c blit 0x000a468c as w by 5 at
   0x000832b0 + 2 over a 0x40 wide source. */
#define FF_BAR_H    5
#define FF_BAR_DX   2
#define FF_BAR_DY   4
#define FF_BAR_SPAN 0x40

/* FFRace.exe .data 0x00084370 0x00084368 0x0008435c 0x00084354 0x0008434c. */
static const char *const ff_lab_main[FF_MENU_ITEMS] = {
    "Practice", "Career", "Settings", "About", "Exit"
};

/* FFRace.exe .data 0x00084280 0x00084270 0x00084268 0x0008425c 0x00084254. */
static const char *const ff_lab_practice[FF_MENU_ITEMS] = {
    "Normal race", "Infinite race", "Pursuit", "Select ship", "Back"
};

/* FFRace.exe .data 0x000840e4 0x000840d8 0x000840c8 0x000840c0 0x00084254. */
static const char *const ff_lab_career[FF_MENU_ITEMS] = {
    "Chanpionships", "Single race", "Single pursuit", "Garage", "Back"
};

/* FFRace.exe 0x00039b1c draws 0x00083f00 at y base 0x4c, 0x00083ee8 at
   0x6d - layout / 4, 0x00083ef4 at 0x8e - layout / 2 and 0x00084254 at 0xd0. */
static const char *const ff_lab_settings[FF_MENU_ITEMS] = {
    "Sounds...", "Gameplay...", "Controls...", 0, "Back"
};

/* FFRace.exe .data 0x00083e90 0x00083e88 0x00083e7c, selected by 0x00083384. */
static const char *const ff_lab_difficulty[3] = {
    "Easy", "Medium", "Advanced"
};

/* FFRace.exe .data 0x00084338 / 0x00084324, 0x00084310 / 0x000842f8,
   0x000842e0 / 0x000842cc, 0x000842bc / 0x000842a8, 0x0008429c / 0x0008428c. */
static const char *const ff_cap_main[FF_MENU_ITEMS][2] = {
    { "Train yourself on",    "the unlocked tracks"  },
    { "Surpass yourself",     "and become the best!" },
    { "Set the sound volume", "and other settings"   },
    { "About the team",       "who made this game"   },
    { "Return to",            "Windows Mobile"       }
};

/* FFRace.exe .data 0x00084240 / 0x00084234, 0x00084224 / 0x00084210,
   0x00084200 / 0x000841ec, 0x000841d8 / 0x000841c4, 0x000841b4 / 0x000841a8. */
static const char *const ff_cap_practice[FF_MENU_ITEMS][2] = {
    { "Choose your track,",  "and race!"           },
    { "How many time",       "will you resist ?"   },
    { "Put 300 meters",      "between you and him" },
    { "Select the ship you", "want to race with"   },
    { "Return to the",       "main menu"           }
};

/* FFRace.exe .data 0x000840a4 / 0x00084094, 0x00084080 / 0x0008406c,
   0x000841b4 / 0x000841a8; item 0 draws 0x00016080 and 0x00016354 instead. */
static const char *const ff_cap_career[FF_MENU_ITEMS][2] = {
    { 0,                  0                  },
    { "In this mode you", "can win a lot!"    },
    { "In this mode you", "can win a lot!"    },
    { "Choose your ship", "and set up items"  },
    { "Return to the",    "main menu"         }
};

/* FFRace.exe .data 0x00083ec4 / 0x00083eb8, 0x00083eac / 0x00083ea4,
   0x00083ee0 / 0x00083ed4, 0x000841b4 / 0x000841a8. */
static const char *const ff_cap_settings[FF_MENU_ITEMS][2] = {
    { "Sound effects", "and music" },
    { "Gameplay",      "Options"   },
    { "Set the",       "controls"  },
    { 0,               0           },
    { "Return to the", "main menu" }
};

/* FFRace.exe .data 0x00083ec4 / 0x00083d90, 0x00083dc0 / 0x00083d90,
   0x00083d7c / 0x00083d70, 0x000841b4 / 0x00083d64. */
static const char *const ff_cap_sound[FF_MENU_ITEMS][2] = {
    { "Sound effects",    "volume"      },
    { "Music",            "volume"      },
    { "Better framerate", "if disabled" },
    { 0,                  0             },
    { "Return to the",    "config menu" }
};

/* FFRace.exe .data 0x00083e14 / 0x00083e00, 0x00083df4 / 0x00083dec,
   0x00083e30 / 0x00083e20, 0x00083de0 / 0x00083dd0, 0x000841b4 / 0x000841a8. */
static const char *const ff_cap_gameplay[FF_MENU_ITEMS][2] = {
    { "Sensitivity", "of Right and Left" },
    { "Difficulty",  "level"             },
    { "Collisions",  "betweens ships"    },
    { "Automatic",   "acceleration"      },
    { "Return to the", "main menu"       }
};

/* FFRace.exe 0x00016354 returns .data 0x00083a20 down to 0x000837b4 for 0
   through 0x24 and its argument itself past that. */
static const char *const ff_race_name[FF_CAREER_LAST + 1] = {
    "Exam",
    "GSC - Part 1", "GSC - Part 2", "GSC - Part 3", "GSC - Part 4",
    "GSC - Part 5",
    "You VS Exeaco",
    "DGI Contest (1)", "DGI Contest (2)", "DGI Contest (3)",
    "You VS Kisswert",
    "Zeroxan's cup (1)", "Zeroxan's cup (2)",
    "Race in Midwest",
    "GGT - Part 1", "GGT - Part 2", "GGT - Part 3", "GGT - Part 4",
    "You VS Heasy",
    "Pro Contest 2029 (1)", "Pro Contest 2029 (2)",
    "New Year's race",
    "SSL Cup - Part 1", "SSL Cup - Part 2", "SSL Cup - Part 3",
    "SSL Cup - Part 4", "SSL Cup - Part 5",
    "You VS Kondor",
    "Race in Salthiem",
    "SpeedCup Part 1", "SpeedCup Part 2", "SpeedCup Part 3",
    "SpeedCup Part 4", "SpeedCup Part 5", "SpeedCup Part 6",
    "You VS Osaris",
    "GO FURTHER!"
};

/* FFRace.exe 0x00016080 returns .data 0x0008378c down to 0x00083654 for 0
   through 0x27, the literals "0/ 35" through "39/ 35". */
static const char *Progress(int n)
{
    static char s[12];

    if (n < 0 || n > 39)
        return "";
    sprintf(s, "%d/ 35", n);
    return s;
}

/* FFRace.exe 0x00039b1c screen 2 draws twelve lines at 0x000832b0 - 8, except
   0x00084124 at 0x000832b0. */
static const struct {
    double      y;
    int         dx;
    int         align;
    const char *s;
} ff_about[12] = {
    {  76.0, -8, 1, "Version 3.20"      },
    {  91.0, -8, 2, "Coder : "          },
    {  91.0, -8, 0, "Osaris"            },
    { 116.0, -8, 2, "Graphics : "       },
    { 116.0, -8, 0, "HelX Concept"      },
    { 131.0, -8, 0, "Osaris"            },
    { 146.0, -8, 0, "KRim"              },
    { 171.0, -8, 2, "Testers : "        },
    { 171.0, -8, 0, "Kisswert"          },
    { 186.0, -8, 0, "FMARC"             },
    { 194.0, -8, 0, ". . ."             },
    { 219.0,  0, 1, "www.pocketnew.net" }
};

/* FFRace.exe 0x00039b1c screen 2 y: base - layout * 0.8 - (layout == 0x14 ? 7 : 0). */
static int AboutY(double base)
{
    return (int)(base - (double)Screen_Layout() * 0.8
                 - (double)(Menu_Small() * 7));
}

static void About(const ff_surface *dst, int centre)
{
    int i;

    for (i = 0; i < 12; i++)
        Text_DrawCentered(dst, centre + ff_about[i].dx, AboutY(ff_about[i].y),
                          ff_about[i].s, ff_about[i].align, 0xff, 0xff, 0xff);
}

static void Bar(const ff_surface *dst, int x, int y, int w, int src_x)
{
    const ff_sprite *bar = AppAssets_Sprite(dst, FF_RES_BAR);

    if (bar != 0)
        Blit(dst, x, y, w, FF_BAR_H, bar, Menu_Key(), src_x, 0);
}

/* FFRace.exe 0x00039b1c screen 0x2a: 0x00083dc8 and 0x00083dc0 at
   0x000832b0 - 0x1e, the 0x000a4db0 volumes as bars, and 0x00083dac /
   0x00083d98 by 0x00083398. */
static void Sound(const ff_surface *dst, int centre)
{
    int vs = Settings_SoundVolume();
    int vm = Settings_MusicVolume();

    Text_DrawCentered(dst, centre - 0x1e, Menu_LabelY(0), "Sound", 1,
                      0xff, 0xff, 0xff);
    Bar(dst, centre + FF_BAR_DX, Menu_LabelY(0) + FF_BAR_DY, vs,
        FF_BAR_SPAN - vs);
    Text_DrawCentered(dst, centre - 0x1e, Menu_LabelY(1), "Music", 1,
                      0xff, 0xff, 0xff);
    Bar(dst, centre + FF_BAR_DX, Menu_LabelY(1) + FF_BAR_DY, vm,
        FF_BAR_SPAN - vm);
    Text_DrawCentered(dst, centre, Menu_LabelY(2),
                      Settings_InGameMusic() ? "In-game music [X]"
                                             : "In-game music [ ]",
                      1, 0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre, Menu_LabelY(4), "Back", 1, 0xff, 0xff, 0xff);
}

/* FFRace.exe 0x00039b1c screen 0x2b: 0x00083e98 at 0x000832b0 align 2 with the
   0x000833bc bar w = sensitivity * 32.0 and src x = 64.0 - sensitivity * 32.0,
   then 0x00083384, 0x000833c0, 0x000832e0 and 0x00084254. */
static void Gameplay(const ff_surface *dst, int centre)
{
    double s = (double)Settings_Sensitivity();
    int    d = Settings_Difficulty();

    Text_DrawCentered(dst, centre, Menu_LabelY(0), "Sensit. ", 2,
                      0xff, 0xff, 0xff);
    Bar(dst, centre + FF_BAR_DX, Menu_LabelY(0) + FF_BAR_DY,
        (int)(s * 32.0), (int)(64.0 - s * 32.0));
    if (d >= FF_DIFF_EASY && d <= FF_DIFF_ADVANCED)
        Text_DrawCentered(dst, centre, Menu_LabelY(1),
                          ff_lab_difficulty[d - FF_DIFF_EASY], 1,
                          0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre, Menu_LabelY(2),
                      Settings_Collisions() ? "Collisions [X]"
                                            : "Collisions [ ]",
                      1, 0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre, Menu_LabelY(3),
                      Settings_AutoAccel() ? "Auto-acc [X]" : "Auto-acc [ ]",
                      1, 0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre, Menu_LabelY(4), "Back", 1, 0xff, 0xff, 0xff);
}

/* FFRace.exe 0x00039b1c screen 3 item 0: 0x000840b8 at 0x000832b0 + 3 align 2,
   0x00016080(0x000a77d0) at the same x align 0 and 0x00016354(0x000a77d0) at
   0x000832b0 align 1. */
static void CareerCaption(const ff_surface *dst, int centre)
{
    int n = Screen_CareerRace();
    int y = Menu_CaptionY(243.0);

    Text_DrawCentered(dst, centre + 3, y, "Race ", 2, 0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre + 3, y, Progress(n), 0, 0xff, 0xff, 0xff);
    if (n >= 0 && n <= FF_CAREER_LAST)
        Text_DrawCentered(dst, centre, Menu_CaptionY(258.0), ff_race_name[n], 1,
                          0xff, 0xff, 0xff);
}

static void Labels(const ff_surface *dst, const char *const *lab, int centre)
{
    int i;

    for (i = 0; i < FF_MENU_ITEMS; i++) {
        if (lab[i] != 0)
            Text_DrawCentered(dst, centre, Menu_LabelY(i), lab[i], 1,
                              0xff, 0xff, 0xff);
    }
}

/* FFRace.exe 0x00039b1c caption y bases 243.0 and 258.0, guarded by
   0x000a7708 == item. */
static void Captions(const ff_surface *dst, const char *const cap[][2],
                     int item, int centre)
{
    if (item < 0 || item >= FF_MENU_ITEMS || cap[item][0] == 0)
        return;
    Text_DrawCentered(dst, centre, Menu_CaptionY(243.0), cap[item][0], 1,
                      0xff, 0xff, 0xff);
    Text_DrawCentered(dst, centre, Menu_CaptionY(258.0), cap[item][1], 1,
                      0xff, 0xff, 0xff);
}

/* FFRace.exe 0x00039b1c 0x0003a3f4: 0x000a7718 == 0 && 0x00083488 not 6, 7, 2. */
int MenuScreens_Plates(int screen)
{
    return screen != FF_SCREEN_SHIP && screen != FF_SCREEN_GARAGE
           && screen != FF_SCREEN_ABOUT;
}

/* FFRace.exe 0x00039b1c LAB_0003a420 / LAB_0003a428 with 0x000a7628 == 0. */
int MenuScreens_Reach(int screen, int item)
{
    if (screen == FF_SCREEN_KEY || screen == FF_SCREEN_CAREER_SUB)
        return item == FF_MENU_EXIT;
    if (screen == FF_SCREEN_CAREER_MENU)
        return 1;
    if (screen == FF_SCREEN_SOUND || screen == FF_SCREEN_SETTINGS
        || screen == FF_SCREEN_TRACK)
        return item != 3;
    return 1;
}

/* FFRace.exe 0x00039b1c LAB_0003a448. */
int MenuScreens_Plate(int screen, int item)
{
    return ((item != 0 && item != 2) || screen != FF_SCREEN_TRACK)
           && (screen != FF_SCREEN_CAREER_SUB
               || Screen_Layout() != FF_LAYOUT_176x220);
}

void MenuScreens_Draw(const ff_surface *dst, int screen, int item, int centre)
{
    switch (screen) {
    case FF_SCREEN_MAIN:
        Labels(dst, ff_lab_main, centre);
        Captions(dst, ff_cap_main, item, centre);
        break;
    case FF_SCREEN_PRACTICE:
        Labels(dst, ff_lab_practice, centre);
        Captions(dst, ff_cap_practice, item, centre);
        break;
    case FF_SCREEN_ABOUT:
        About(dst, centre);
        break;
    case FF_SCREEN_CAREER_MENU:
        Labels(dst, ff_lab_career, centre);
        if (item == 0)
            CareerCaption(dst, centre);
        else
            Captions(dst, ff_cap_career, item, centre);
        break;
    case FF_SCREEN_SETTINGS:
        Labels(dst, ff_lab_settings, centre);
        Captions(dst, ff_cap_settings, item, centre);
        break;
    case FF_SCREEN_SOUND:
        Sound(dst, centre);
        Captions(dst, ff_cap_sound, item, centre);
        break;
    case FF_SCREEN_GAMEPLAY:
        Gameplay(dst, centre);
        Captions(dst, ff_cap_gameplay, item, centre);
        break;
    case FF_SCREEN_TRACK:
        MenuPanels_Track(dst, centre);
        break;
    case FF_SCREEN_SHIP:
        MenuPanels_Ship(dst, centre);
        break;
    default:
        break;
    }
}
