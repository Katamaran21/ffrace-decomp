#ifndef FF_SETTINGS_H
#define FF_SETTINGS_H

/* FFRace.exe .data 0x00083384 initialiser 2, cycled by 0x0004fdd4. */
#define FF_DIFF_EASY     1
#define FF_DIFF_MEDIUM   2
#define FF_DIFF_ADVANCED 3

/* FFRace.exe Settings_Store 0x00055644 keys 6 and 7 fall back to 0x20. */
#define FF_VOLUME_MAX  0x40
#define FF_VOLUME_STEP 4

int   Settings_Difficulty(void);
void  Settings_CycleDifficulty(void);
float Settings_Sensitivity(void);
void  Settings_StepSensitivity(int delta);
int   Settings_Collisions(void);
void  Settings_ToggleCollisions(void);
int   Settings_AutoAccel(void);
void  Settings_ToggleAutoAccel(void);
int   Settings_InGameMusic(void);
void  Settings_ToggleInGameMusic(void);
int   Settings_SoundVolume(void);
int   Settings_MusicVolume(void);
void  Settings_StepVolume(int which, int delta);

#endif /* FF_SETTINGS_H */
