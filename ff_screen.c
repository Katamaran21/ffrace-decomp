/* Screen state ported from FFRace.exe (imagebase 0x00010000). */
#include "ff_screen.h"

#include "ff_appassets.h"
#include "ff_audio.h"
#include "ff_consts.h"
#include "ff_ingame.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_race.h"
#include "ff_settings.h"

/* FFRace.exe Game_Init 0x000115b0 fills slot 0x000a46a4 from
   LoadBitmapW(hinst, 0x12e); 0x0004e0c0 blits it as 0xb0 x 0x87 at
   0x000832b0 - 0x58. */
#define FF_RES_FAREWELL 302
#define FF_FAREWELL_W   0xb0
#define FF_FAREWELL_H   0x87
#define FF_FAREWELL_DX  0x58

static int ff_screen_id;
static int ff_screen_param;
static int ff_screen_layout = FF_LAYOUT_240x320;
static int ff_screen_transition;

/* FFRace.exe .data 0x000a77d0, zero until a career race is won. */
static int ff_career_race;

/* FFRace.exe .data 0x000a779c, the mode items 0, 1 and 2 of screen 1 store at
   0x0004ffe8, 0x00050224 and 0x000502e4 before 0x00050098 opens screen 0x33. */
static int ff_pending_mode;

/* FFRace.exe .data 0x000a76c0, zero-initialised, so screen 0x33 opens on the
   random track. */
static int ff_track;

/* FFRace.exe Screen_Set 0x00014924 writes 0x00083380 = 1, clamps 0x000832f8
   against 0x000833a4 and calls stopSounds(0x000a4db0); its tail 0x000149d4
   calls Music_Select 0x00045b34 when 0x00083380 was 0 on entry. */
void Screen_Set(int id, int param)
{
    int was_ingame = Ingame_Active();

    Ingame_Stop();

    ff_screen_id         = id;
    ff_screen_param      = param;
    ff_screen_transition = FF_TRANSITION_FRAMES;

    Audio_StopSounds();
    if (was_ingame)
        Audio_MusicSelect(id, 1, Ingame_IntroPending());
}

int Screen_Id(void)
{
    return ff_screen_id;
}

int Screen_Param(void)
{
    return ff_screen_param;
}

void Screen_SetParam(int param)
{
    ff_screen_param = param;
}

void Screen_SetLayout(int layout)
{
    ff_screen_layout = layout;
}

int Screen_Layout(void)
{
    return ff_screen_layout;
}

int Screen_Transition(void)
{
    return ff_screen_transition;
}

void Screen_TransitionStep(void)
{
    if (ff_screen_transition > 0)
        ff_screen_transition--;
}

/* FFRace.exe 0x00039b1c emits Text_DrawCentered for 0x00083488 in {0, 1, 2, 3,
   4, 0x2a, 0x2b} and a panel block for 6 and 0x33; the 5, 7 and 10 blocks are
   not ported yet. */
static int Rendered(int id)
{
    return id == FF_SCREEN_MAIN || id == FF_SCREEN_PRACTICE
           || id == FF_SCREEN_ABOUT || id == FF_SCREEN_CAREER_MENU
           || id == FF_SCREEN_SETTINGS || id == FF_SCREEN_SOUND
           || id == FF_SCREEN_GAMEPLAY || id == FF_SCREEN_SHIP
           || id == FF_SCREEN_TRACK;
}

static void Go(int id, int param)
{
    if (Rendered(id))
        Screen_Set(id, param);
}

/* FFRace.exe 0x0004f6bc at 0x00050144 and 0x000501bc: both 0x00083488 == 3
   starts clear 0x00083380 and pass (mode, 0xffffffff, 1000, 0xffffffff,
   0xffffffff) to Race_Init 0x00013aec. */
static void StartRace(int mode)
{
    Ingame_Start(mode, FF_SEED_NONE, FF_TRACK_DEFAULT_LEN, FF_THEME_RANDOM,
                 FF_SKY_RANDOM);
}

/* FFRace.exe 0x000500bc: Race_Init(0x000a779c, -1, 1000, 0x000a76c0 - 1, -1),
   then 0x000a7708 = 0 and 0x00083488 = 1 as direct stores, with no 0x00014924
   call and so no 0x000a77f4 transition. */
void Screen_StartRace(int mode, int theme)
{
    Ingame_Start(mode, FF_SEED_NONE, FF_TRACK_DEFAULT_LEN, theme,
                 FF_SKY_RANDOM);

    ff_screen_id    = FF_SCREEN_PRACTICE;
    ff_screen_param = 0;
}

/* FFRace.exe 0x000a779c, written by the three 0x00050098 predecessors. */
void Screen_SetPendingMode(int mode)
{
    ff_pending_mode = mode;
}

int Screen_PendingMode(void)
{
    return ff_pending_mode;
}

/* FFRace.exe 0x000a76c0, the screen 0x33 cursor. */
void Screen_SetTrack(int track)
{
    ff_track = track;
}

int Screen_Track(void)
{
    return ff_track;
}

/* FFRace.exe 0x0004f6bc at 0x000500bc and 0x00050144: the 0x33 starts and the
   0x000a7708 = 4 park both test 0x000a76c0 against 0x00016b78(0x000a77d0) + 1. */
static int TrackUnlocked(void)
{
    return ff_track <= Race_ThemeUnlocked(ff_career_race) + 1;
}

/* FFRace.exe 0x0004ffe8, 0x00050224 and 0x000502e4 write 0, 1 and 2 into
   0x000a779c and fall into 0x00050098, which opens 0x33 with 0x000a7708 = 4. */
static void Practice(int mode)
{
    ff_pending_mode = mode;
    Go(FF_SCREEN_TRACK, 4);
}

/* FFRace.exe 0x000500bc and its 0x00050144 fallthrough. */
static void TrackGo(void)
{
    if (TrackUnlocked())
        Screen_StartRace(ff_pending_mode, ff_track - 1);
    else
        ff_screen_param = 4;
}

/* FFRace.exe 0x000501e8. */
static void TrackPrev(void)
{
    ff_track--;
    if (ff_track < 0)
        ff_track = 0;
    ff_screen_param = 4;
}

/* FFRace.exe 0x000501bc. */
static void TrackNext(void)
{
    if (ff_track + 1 < 10)
        ff_track++;
    else
        ff_track = 9;
    ff_screen_param = 4;
}

/* FFRace.exe WndProc 0x0004f6bc activation dispatch 0x00050000 selects on
   0x000a7708 and then on 0x00083488, with 0x000a7628 held at 0.  0x0004e0c0 is
   the only branch that leaves the message loop. */
int Screen_Activate(int screen, int item)
{
    switch (item) {
    case 0:
        switch (screen) {
        case FF_SCREEN_MAIN:        Go(FF_SCREEN_PRACTICE, 0);    break;
        case FF_SCREEN_PRACTICE:    Practice(FF_RACE_SCRIPTED);   break;
        case FF_SCREEN_TRACK:       TrackPrev();                  break;
        case FF_SCREEN_SETTINGS:    Go(FF_SCREEN_SOUND, 0);       break;
        case FF_SCREEN_SHIP:        Go(FF_SCREEN_PRACTICE, 0);    break;
        case FF_SCREEN_GARAGE:      Go(FF_SCREEN_CAREER_MENU, 0); break;
        case FF_SCREEN_SOUND:       Settings_StepVolume(1, 1);    break;
        case FF_SCREEN_GAMEPLAY:    Settings_StepSensitivity(1);  break;
        default: break;
        }
        break;
    case 1:
        switch (screen) {
        case FF_SCREEN_MAIN:        Go(FF_SCREEN_CAREER_MENU, 0); break;
        case FF_SCREEN_PRACTICE:    Practice(FF_RACE_ENDLESS);    break;
        case FF_SCREEN_TRACK:       TrackGo();                    break;
        case FF_SCREEN_CAREER_MENU: StartRace(FF_RACE_SCRIPTED);  break;
        case FF_SCREEN_SETTINGS:    Go(FF_SCREEN_GAMEPLAY, 0);    break;
        case FF_SCREEN_SHIP:        Go(FF_SCREEN_PRACTICE, 0);    break;
        case FF_SCREEN_GARAGE:      Go(FF_SCREEN_CAREER_MENU, 0); break;
        case FF_SCREEN_SOUND:       Settings_StepVolume(2, 1);    break;
        case FF_SCREEN_GAMEPLAY:    Settings_CycleDifficulty();   break;
        default: break;
        }
        break;
    case 2:
        switch (screen) {
        case FF_SCREEN_MAIN:        Go(FF_SCREEN_SETTINGS, 0);    break;
        case FF_SCREEN_PRACTICE:    Practice(FF_RACE_ENDLESS_2);  break;
        case FF_SCREEN_TRACK:       TrackNext();                  break;
        case FF_SCREEN_CAREER_MENU: StartRace(FF_RACE_ENDLESS_2); break;
        case FF_SCREEN_SOUND:       Settings_ToggleInGameMusic(); break;
        case FF_SCREEN_GAMEPLAY:    Settings_ToggleCollisions();  break;
        default: break;
        }
        break;
    case 3:
        switch (screen) {
        case FF_SCREEN_MAIN:        Go(FF_SCREEN_ABOUT, 4);       break;
        case FF_SCREEN_PRACTICE:    Go(FF_SCREEN_SHIP, 0);        break;
        case FF_SCREEN_CAREER_MENU: Go(FF_SCREEN_GARAGE, 0);      break;
        case FF_SCREEN_GAMEPLAY:    Settings_ToggleAutoAccel();   break;
        default: break;
        }
        break;
    case FF_MENU_EXIT:
        switch (screen) {
        case FF_SCREEN_MAIN:        return 1;
        case FF_SCREEN_TRACK:       TrackGo();                    break;
        case FF_SCREEN_PRACTICE:
        case FF_SCREEN_ABOUT:
        case FF_SCREEN_CAREER_MENU:
        case FF_SCREEN_SETTINGS:    Go(FF_SCREEN_MAIN, 0);        break;
        case FF_SCREEN_CAREER_SUB:  Go(FF_SCREEN_CAREER_MENU, 0); break;
        case FF_SCREEN_SOUND:
        case FF_SCREEN_GAMEPLAY:    Go(FF_SCREEN_SETTINGS, 0);    break;
        default: break;
        }
        break;
    default:
        break;
    }
    return 0;
}

/* FFRace.exe WndProc 0x0004f6bc WM_LBUTTONUP: 0x00083488 == 2 ends with
   Screen_Set(0, 0) for every tap.  Its 6 / 7 branch leaves for 1 or 3 unless
   100 <= 0x000a77e4 <= 0xa0, and inside that band steps 0x000832f8 down below
   0x52 and up above 0x9f, wrapping 1 .. 6 and writing 0x000a7708 = 4. */
int Screen_Tap(int x, int y)
{
    if (ff_screen_id == FF_SCREEN_ABOUT) {
        Screen_Set(FF_SCREEN_MAIN, 0);
        return 1;
    }
    if (ff_screen_id == FF_SCREEN_SHIP || ff_screen_id == FF_SCREEN_GARAGE) {
        int ship;

        if (y < 100 || y > 0xa0 || (x > 0x52 && x < 0x9f)) {
            Screen_Set(ff_screen_id == FF_SCREEN_SHIP ? FF_SCREEN_PRACTICE
                                                      : FF_SCREEN_CAREER_MENU,
                       0);
        } else if (x < 0x52) {
            /* FFRace.exe 0x0004f6bc playSound(0x000a4db0, 0x000a4b30,
               0x10000000) on each of the two 0x000832f8 steps. */
            Audio_Play(FF_SND_MENU);
            ship = Physics_Ship() - 1;
            if (ship < 1)
                ship = FF_SHIP_SPRITES;
            Physics_SetShip(ship);
            ff_screen_param = 4;
        } else if (x > 0x9f) {
            Audio_Play(FF_SND_MENU);
            ship = Physics_Ship() + 1;
            if (ship > FF_SHIP_SPRITES)
                ship = 1;
            Physics_SetShip(ship);
            ff_screen_param = 4;
        }
        return 1;
    }
    return 0;
}

/* FFRace.exe 0x00016080 / 0x00016354 index 0x000a77d0. */
int Screen_CareerRace(void)
{
    return ff_career_race;
}

/* FFRace.exe 0x000a77d0 counts won career races; the image has no writer for
   it, so 0x00016b78 stays on its bucket 0 and only theme 0 is ever drawn. */
void Screen_SetCareerRace(int race)
{
    if (race < 0)
        race = 0;

    ff_career_race = race;
}

/* FFRace.exe WndProc 0x0004f6bc back-key branch 0x0004fd58: 0x00083488 == 1 at
   0x0004fd98, 2 at 0x0004fdb0 and 3 at 0x0004fdc8 take Screen_Set(0, 0), 5 at
   0x0004fde0 takes Screen_Set(3, 0) and 0x33 at 0x0004fdf8 takes
   Screen_Set(1, 0).  0, 4, 0x2a and 0x2b have no case. */
int Screen_Back(void)
{
    /* FFRace.exe 0x0004f6bc at 0x000506a8: with 0x00083380 != 1 the 0x000833d4
       key calls Screen_Set 0x00014924 with 0x00083488 and 0x000a7708. */
    if (Ingame_Active()) {
        Screen_Set(ff_screen_id, ff_screen_param);
        return 1;
    }
    if (ff_screen_id == FF_SCREEN_PRACTICE || ff_screen_id == FF_SCREEN_ABOUT ||
        ff_screen_id == FF_SCREEN_CAREER_MENU) {
        Screen_Set(FF_SCREEN_MAIN, 0);
        return 1;
    }
    if (ff_screen_id == FF_SCREEN_CAREER_SUB) {
        Go(FF_SCREEN_CAREER_MENU, 0);
        return 1;
    }
    if (ff_screen_id == FF_SCREEN_TRACK) {
        Go(FF_SCREEN_PRACTICE, 0);
        return 1;
    }
    return 0;
}

/* FFRace.exe 0x00039a7c fills 0x000832a8 x 0x000832ac pixels with
   Color_Pack16If16bpp(surface, 0). */
static void Clear(const ff_surface *dst)
{
    uint16_t c = Color_Pack16If16bpp(dst, 0);
    int      x, y;

    for (y = 0; y < ff_clip_h; y++) {
        for (x = 0; x < ff_clip_w; x++)
            dst->pixels[y * dst->y_pitch + x * dst->x_pitch] = c;
    }
}

/* FFRace.exe 0x0004e0c0 restores 0x000a4430 through 0x0005b130 and then calls
   0x00039a7c, which overwrites every pixel of the frame.  The four
   Text_DrawCentered lines and the second panel blit are reached only when
   0x000a7628 is 1, and no instruction in the image writes 0x000a7628. */
int Screen_Farewell(const ff_surface *dst)
{
    const ff_sprite *panel = AppAssets_Sprite(dst, FF_RES_FAREWELL);
    int              small = ff_screen_layout == FF_LAYOUT_176x220;

    Clear(dst);
    if (panel == 0)
        return 0;
    Blit_Keyed(dst,
               (small ? FF_SMALL_VIEW_CENTER_X : FF_VIEW_CENTER_X) - FF_FAREWELL_DX,
               small ? 0x2a : 0x5c, FF_FAREWELL_W, FF_FAREWELL_H, panel,
               0, FF_NO_KEY, 0, 0);
    return 1;
}
