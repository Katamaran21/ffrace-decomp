#include <string.h>

#include "ff_ingame.h"

#include "ff_appassets.h"
#include "ff_audio.h"
#include "ff_consts.h"
#include "ff_hud.h"
#include "ff_opponent.h"
#include "ff_physics.h"
#include "ff_platform.h"
#include "ff_race.h"
#include "ff_render.h"
#include "ff_road.h"
#include "ff_screen.h"
#include "ff_ship.h"

/* FFRace.exe 0x000a4430, the surface 0x0005b0cc allocates for the theme sky. */
static uint16_t   ff_sky_pixels[FF_VIEW_W * FF_VIEW_H];
static ff_surface ff_sky;

/* FFRace.exe 0x000a4564, the sprite Track_Generate 0x00014a20 retains. */
static const ff_sprite *ff_sky_sprite;

static int      ff_active;
static unsigned ff_last;     /* 0x000a7664 */
static unsigned ff_dt;       /* 0x000a7660 */
static unsigned ff_sec_base; /* 0x000a7658 */
static unsigned ff_sec_ms;   /* 0x000a7654 */
static int      ff_frames;   /* 0x000a764c */
static int      ff_fps;      /* 0x000a7650 */
static int      ff_minutes;  /* 0x000a772c */

/* FFRace.exe .data 0x000833b4 initialiser 1; 0x0004f6bc clears it in the same
   block that runs 0x00045ea0 once, and Music_Select 0x00045b34 returns early
   while it is set. */
static int      ff_intro_pending = 1;

/* FFRace.exe 0x0005b130 and 0x0005b194 copy 0x000a4430[1] into 0x000a43f8[0],
   the second at a pixel offset and 24000 pixels short. */
static void Sky_Copy(const ff_surface *dst, int off, int count)
{
    memcpy(dst->pixels + off, ff_sky_pixels,
           (size_t)count * sizeof ff_sky_pixels[0]);
}

/* FFRace.exe Track_Generate 0x00014a20 tail: 0x0005b0cc then 0x0005a584 blits
   one 0x00056458 sprite over the whole surface.  0x000a77b8 is stored per theme
   and never read. */
static void Sky_Build(const ff_surface *dst)
{
    uint16_t black = Color_Pack16If16bpp(dst, 0);
    int      i;

    ff_sky.pixels  = ff_sky_pixels;
    ff_sky.bpp     = dst->bpp;
    ff_sky.fmt     = dst->fmt;
    ff_sky.x_pitch = 1;
    ff_sky.y_pitch = ff_clip_w;

    for (i = 0; i < ff_clip_w * ff_clip_h; i++)
        ff_sky_pixels[i] = black;

    ff_sky_sprite = AppAssets_Sprite(dst, Race_SkyResource());
    if (ff_sky_sprite != 0)
        Blit_NoKey(&ff_sky, 0, 0, ff_clip_w, ff_clip_h, ff_sky_sprite, 0, 0);
}

/* FFRace.exe 0x0004f6bc: the 0x00050144 dispatch writes 0x00083380 = 0 and
   calls Race_Init 0x00013aec, and again at 0x000501bc for 0x000a7708 == 2. */
void Ingame_Start(int mode, int seed, int length, int theme, int sky)
{
    const ff_surface *dst = Platform_BackBuffer();

    ff_active = 1;

    Race_Init(mode, seed, length, theme, sky);
    Physics_Reset();
    Physics_SetCountdown(FF_COUNTDOWN_START);

    /* FFRace.exe Race_Init 0x00013aec calls Music_Select 0x00045b34 before the
       block that opens the engine channel. */
    Audio_MusicSelect(Screen_Id(), 0, ff_intro_pending);

    ff_last     = 0;
    ff_dt       = 0;
    ff_sec_base = 0;
    ff_sec_ms   = 0;
    ff_frames   = 0;
    ff_fps      = 0;
    ff_minutes  = 0;

    Sky_Build(dst);

    /* FFRace.exe Race_Init 0x00013aec tail: stopSounds(0x000a4db0), then
       load(0x000a4d10, 0x0005c000), loop(0x000a4d10, 1), volume(0x000a4d10,
       0x18) and playSound keep the channel in 0x000a4858. */
    Audio_StopSounds();
    Audio_EngineStart();
}

int Ingame_Active(void)
{
    return ff_active;
}

void Ingame_Stop(void)
{
    ff_active = 0;
}

int Ingame_IntroPending(void)
{
    return ff_intro_pending;
}

/* FFRace.exe 0x0004f6bc WM_TIMER: 0x000a7664 seeds itself once, 0x000a764c
   counts frames, 0x000a7660 = 0x000a765c - 0x000a7664 unclamped, and
   0x000a7654 = 0x000a765c - 0x000a7658.  Past 1000 it steps 0x000a7728, wraps
   0x3c of them into 0x000a772c, and latches 0x000a764c into 0x000a7650. */
void Ingame_Tick(void)
{
    unsigned now;
    int      seconds;

    if (ff_last == 0)
        ff_last = Platform_Ticks();
    ff_frames++;
    now       = Platform_Ticks();
    ff_dt     = now - ff_last;
    ff_last   = Platform_Ticks();
    ff_sec_ms = now - ff_sec_base;

    /* FFRace.exe 0x0004f6bc: 0x000833b4 == 1 runs 0x00045ea0, whose head plays
       0x000a4ba8, then clears the flag and calls Music_Select 0x00045b34 again,
       which now reaches its race branch. */
    if (ff_intro_pending) {
        Audio_Play(FF_SND_MISS);
        ff_intro_pending = 0;
        Audio_MusicSelect(Screen_Id(), 0, 0);
    }

    /* FFRace.exe 0x00050f88 rewrites frequency(0x000a4858, ...) whenever
       0x000a77dc, which has no writer, misses 0x000a42dc by more than 0.1. */
    if (Physics_Derived() > 0.1f)
        Audio_EnginePitch(Physics_Speed());

    if (ff_sec_ms > FF_SECOND_MS) {
        seconds = Physics_Countdown() + 1;
        if (seconds == FF_MINUTE_SEC) {
            seconds = 0;
            ff_minutes++;
        }
        Physics_SetCountdown(seconds);

        /* FFRace.exe 0x0004f6bc plays 0x000a4a40, 0x000a49c8, 0x000a4950 and
           0x000a48d8 as 0x000a7728 steps -3, -2, -1 and 0, and playMusic on 1
           behind 0x00083398. */
        if (seconds == -3)
            Audio_Play(FF_SND_BEEP3);
        else if (seconds == -2)
            Audio_Play(FF_SND_BEEP2);
        else if (seconds == -1)
            Audio_Play(FF_SND_BEEP1);
        else if (seconds == 0)
            Audio_Play(FF_SND_GO);
        else if (seconds == 1)
            Audio_MusicStart();

        ff_fps      = ff_frames;
        ff_sec_base = Platform_Ticks();
        ff_frames   = 0;
        ff_sec_ms  -= FF_SECOND_MS;
    }

    Race_UpdateRank();

    /* FFRace.exe 0x0004f6bc WM_KEYDOWN keys 0x000833c4, 0x000833c8,
       0x000833d8 and 0x000833dc. */
    Physics_SetInput(FF_INPUT_LEFT,     Platform_KeyDown(FF_KEY_LEFT));
    Physics_SetInput(FF_INPUT_RIGHT,    Platform_KeyDown(FF_KEY_RIGHT));
    Physics_SetInput(FF_INPUT_THROTTLE, Platform_KeyDown(FF_KEY_UP));
    Physics_SetInput(FF_INPUT_BRAKE,    Platform_KeyDown(FF_KEY_DOWN));

    Physics_Step((int)ff_dt);

    /* FFRace.exe 0x0004f6bc plays 0x000a4c20 inside the gate that writes
       0x000a77cc = 1 and 0x000a4ab8 right after it, in that order, at both wall
       responses, and 0x000a4ab8 alone where 0x000a7734 clears. */
    if (Physics_TakeEndSound())
        Audio_Play(FF_SND_EXPL);
    if (Physics_TakeHitSound())
        Audio_Play(FF_SND_BLAM);
}

/* FFRace.exe 0x0004e3c8: 0x0005b130 copies the whole sky, then for themes 3, 1,
   0x32, 6, 7, 9 and 8 it takes (int)((float)(0x000832b0 * 0.65) * 0x000a77fc)
   into 0 .. 0x000832a8, calls 0x0005b194 at that offset and patches row 0 from
   0x000a4564 at source x 0x000832a8 - offset. */
void Ingame_Frame(const ff_surface *dst)
{
    int theme  = Race_Theme();
    int centre = (Screen_Layout() == FF_LAYOUT_240x320) ? FF_VIEW_CENTER_X
                                                        : FF_SMALL_VIEW_CENTER_X;
    int pixels = ff_clip_w * ff_clip_h;
    int off;

    Sky_Copy(dst, 0, pixels);

    if (theme == 3 || theme == 1 || theme == 0x32 || theme == 6 || theme == 7 ||
        theme == 9 || theme == 8) {
        off = (int)((float)((double)(float)centre * FF_SKY_SCROLL) *
                    Race_Curve());
        while (off < 0)
            off += ff_clip_w;
        while (off >= ff_clip_w)
            off -= ff_clip_w;

        Sky_Copy(dst, off, pixels - FF_SKY_SHIFT_SHORT);
        if (ff_sky_sprite != 0)
            Blit_NoKey(dst, 0, 0, off, 1, ff_sky_sprite, ff_clip_w - off, 0);
    }

    /* FFRace.exe 0x00046754. */
    Render_Road(dst, Road_DrawSegment);
    Opponent_Shadows(dst);
    Ship_Draw(dst);
    Opponent_Ships(dst);
    Ship_Wreck(dst);
    /* FFRace.exe 0x00046754 plays 0x000a4c20 at the head of the 39999 <
       0x000a7670 block, on the frame it enters with 0x000a77c0 at 0. */
    if (Ship_TakeWreckSound())
        Audio_Play(FF_SND_EXPL);
    Hud_Frame(dst);
}

int Ingame_Fps(void)
{
    return ff_fps;
}

int Ingame_SecondMs(void)
{
    return (int)ff_sec_ms;
}

int Ingame_Minutes(void)
{
    return ff_minutes;
}

int Ingame_Frames(void)
{
    return ff_frames;
}
