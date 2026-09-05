/* Settings state ported from FFRace.exe (imagebase 0x00010000). */
#include "ff_settings.h"

#include "ff_audio.h"

/* FFRace.exe .data 0x00083384 initialiser 2. */
static int ff_difficulty = FF_DIFF_MEDIUM;

/* FFRace.exe .data 0x000833bc initialiser 1.0f. */
static float ff_sensitivity = 1.0f;

/* FFRace.exe .data 0x000833c0 initialiser 1. */
static int ff_collisions = 1;

/* FFRace.exe .data 0x000832e0 initialiser 1. */
static int ff_auto_accel = 1;

/* FFRace.exe .data 0x00083398 initialiser 1. */
static int ff_ingame_music = 1;

/* FFRace.exe Settings_Load 0x000556f0 stores 0x20 into volumeSounds and
   volumeMusics of 0x000a4db0 when keys 6 and 7 are absent. */
static int ff_volume_sound = 0x20;
static int ff_volume_music = 0x20;

int Settings_Difficulty(void)
{
    return ff_difficulty;
}

/* FFRace.exe 0x0004fdd4: 0x00083384 + 1, reset to 1 once above 3. */
void Settings_CycleDifficulty(void)
{
    ff_difficulty++;
    if (ff_difficulty > FF_DIFF_ADVANCED)
        ff_difficulty = FF_DIFF_EASY;
}

float Settings_Sensitivity(void)
{
    return ff_sensitivity;
}

/* FFRace.exe 0x0004fd60 adds 0.1 and clamps at 2.0; 0x0004fb84 subtracts 0.1
   and clamps at 0.1.  Both clamp with a negated less-than, so a NaN would
   settle on the bound. */
void Settings_StepSensitivity(int delta)
{
    if (delta > 0) {
        ff_sensitivity = (float)(ff_sensitivity + 0.1);
        if (!(ff_sensitivity < 2.0f))
            ff_sensitivity = 2.0f;
    } else {
        ff_sensitivity = (float)(ff_sensitivity - 0.1);
        if (!(ff_sensitivity > 0.1f))
            ff_sensitivity = 0.1f;
    }
}

int Settings_Collisions(void)
{
    return ff_collisions;
}

/* FFRace.exe 0x0004fe68 writes 1 - 0x000833c0. */
void Settings_ToggleCollisions(void)
{
    ff_collisions = 1 - ff_collisions;
}

int Settings_AutoAccel(void)
{
    return ff_auto_accel;
}

/* FFRace.exe 0x0004fea4 writes 1 - 0x000832e0. */
void Settings_ToggleAutoAccel(void)
{
    ff_auto_accel = 1 - ff_auto_accel;
}

int Settings_InGameMusic(void)
{
    return ff_ingame_music;
}

/* FFRace.exe 0x0004fe18 writes 1 - 0x00083398. */
void Settings_ToggleInGameMusic(void)
{
    ff_ingame_music = 1 - ff_ingame_music;
}

int Settings_SoundVolume(void)
{
    return ff_volume_sound;
}

int Settings_MusicVolume(void)
{
    return ff_volume_music;
}

/* FFRace.exe 0x000135dc: which 1 drives volumeSounds, 2 volumeMusics; the new
   value is old + delta * 4 and is applied only while (unsigned)new <= 0x40. */
void Settings_StepVolume(int which, int delta)
{
    int *v = which == 1 ? &ff_volume_sound : which == 2 ? &ff_volume_music : 0;
    int  n;

    if (v == 0)
        return;
    n = *v + delta * FF_VOLUME_STEP;
    if ((unsigned)n <= FF_VOLUME_MAX) {
        *v = n;
        Audio_PushVolumes();
    }
}
