/* Sound and music ported from FFRace.exe (imagebase 0x00010000). */
#include "ff_audio.h"

#include "ff_assets.h"
#include "ff_consts.h"
#include "ff_platform.h"
#include "ff_settings.h"

#include <stdio.h>
#include <stdlib.h>

/* FFRace.exe 0x000136d8 appends each name to \Sounds\ and calls hssSound::load
   then volume(); Race_Init 0x00013aec loads the engine sample at 0x18, and
   0x000a4860 is the only 0x000136d8 entry taking loop(1). */
static const struct {
    const char *name;
    int         volume;
} ff_snd_table[FF_SND_COUNT] = {
    { "sblam.wav",   0x20 },
    { "smenu.wav",   0x20 },
    { "sexpl.wav",   0x40 },
    { "smiss.wav",   0x40 },
    { "scount3.wav", 0x40 },
    { "scount2.wav", 0x40 },
    { "scount1.wav", 0x40 },
    { "sgo.wav",     0x40 },
    { "ssiren.wav",  0x20 },
    { "sengine.wav", 0x18 }
};

/* FFRace.exe Music_Select 0x00045b34 leaves its pick loaded in 0x000a4d88 and
   the per-second block at 0x0004f6bc plays that object when 0x000a7728 hits 1,
   so the choice and the playMusic are one second apart. */
static char ff_mus_file[16];
static int  ff_mus_volume;
static int  ff_mus_started;

void Audio_Init(void)
{
    int i;

    if (!Platform_AudioInit())
        return;

    for (i = 0; i < FF_SND_COUNT; i++)
        Platform_SoundLoad(i, Assets_Sound(ff_snd_table[i].name),
                           ff_snd_table[i].volume * FF_MIX_VOLUME_MAX
                           / FF_VOLUME_MAX);
    Audio_PushVolumes();
}

void Audio_Shutdown(void)
{
    Platform_AudioShutdown();
}

/* FFRace.exe hands hssSpeaker::playSound(0x000a4db0, sound, 0x10000000) at
   every one of its call sites. */
void Audio_Play(int snd)
{
    if (snd < 0 || snd >= FF_SND_COUNT)
        return;

    Platform_SoundPlay(snd);
}

void Audio_StopSounds(void)
{
    Platform_SoundStopAll();
}

void Audio_EngineStart(void)
{
    Platform_SoundLoop(FF_SND_ENGINE);
}

/* FFRace.exe 0x00050f88: Ordinal_2043((1.0 + 0x000a42c4 * 0.001) *
   0x000a7748), and 0x000a7748 is the engine sample's own rate. */
void Audio_EnginePitch(double speed)
{
    int native = Platform_SoundRate(FF_SND_ENGINE);

    if (native <= 0)
        return;

    Platform_SoundLoopRate((int)((1.0 + speed * 0.001) * (double)native));
}

void Audio_MusicSelect(int screen, int in_menu, int intro_pending)
{
    Platform_MusicStop();
    ff_mus_file[0]  = 0;
    ff_mus_volume   = 0;
    ff_mus_started  = 0;

    /* FFRace.exe 0x00045b34 gates its menu branch on 0x00083380 == 1,
       0x00083394 == 1 and 0x000833b4 == 0 and returns from inside it; the
       screens it names are 0 and 1 at 0x20, 3 and 5 at 0x40. */
    if (in_menu) {
        if (intro_pending)
            return;
        if (screen == FF_SCREEN_MAIN || screen == FF_SCREEN_PRACTICE)
            Platform_MusicPlay(Assets_Music("mainmenu.tkm"), 0x20, 1);
        else if (screen == FF_SCREEN_CAREER_MENU ||
                 screen == FF_SCREEN_CAREER_SUB)
            Platform_MusicPlay(Assets_Music("menucareer.tkm"), 0x40, 1);
        return;
    }

    /* FFRace.exe 0x00045b34 then returns unless 0x00083398 == 1 and
       0x000833b4 == 0; its race branch loads course{rand() % 9 + 1}.tkm at
       0x20 and its own playMusic sits behind 0x00083380 == 1. */
    if (!Settings_InGameMusic() || intro_pending)
        return;

    sprintf(ff_mus_file, "course%d.tkm", rand() % 9 + 1);
    ff_mus_volume = 0x20;
}

void Audio_MusicStart(void)
{
    if (ff_mus_file[0] == 0 || ff_mus_started)
        return;

    ff_mus_started = 1;
    Platform_MusicPlay(Assets_Music(ff_mus_file), ff_mus_volume, 1);
}

void Audio_MusicStop(void)
{
    ff_mus_started = 0;
    Platform_MusicStop();
}

/* FFRace.exe Game_Init 0x000115b0 volumeSounds(0x000a4db0, 0x20), and
   0x000135dc writes volumeSounds for which == 1, volumeMusics for 2. */
void Audio_PushVolumes(void)
{
    Platform_SoundMasterVolume(Settings_SoundVolume());
    Platform_MusicMasterVolume(Settings_MusicVolume());
}
