/* Host layer for the port.  It stands in for JumpyBall.exe's GAPI path
   (Gfx_CreateBackBuffer 0x00021698, Gfx_Present 0x000126fc). */
#ifndef FF_PLATFORM_H
#define FF_PLATFORM_H

#include "ff_gfx.h"

enum {
    FF_KEY_LEFT,
    FF_KEY_RIGHT,
    FF_KEY_UP,
    FF_KEY_DOWN,
    FF_KEY_JUMP,
    FF_KEY_MENU,
    FF_KEY_COUNT
};

int Platform_Init(int w, int h, int scale, const char *title);
void Platform_Shutdown(void);

/* The 16bpp surface every blitter in ff_gfx.h draws into. */
ff_surface *Platform_BackBuffer(void);

void Platform_Present(void);

/* Returns 0 once the window has been closed. */
int Platform_PollEvents(void);

int Platform_KeyDown(int key);

/* jumpyball JumpyBall.exe WndProc 0x0001fd2c compares the incoming VK code
   against g_keyLeft, g_keyRight, g_keyUp, g_keyDown, g_keyJump and g_keyMenu;
   the key-config wizard in the same function writes those six slots. */
#define FF_KEY_UNBOUND 0

int Platform_KeyBinding(int key);
void Platform_SetKeyBinding(int key, int code);

/* jumpyball JumpyBall.exe WndProc 0x0001fd2c binds on WM_KEYDOWN. */
int Platform_NextRawKey(void);
void Platform_FlushRawKeys(void);

/* FFRace.exe WndProc 0x0004f6bc keeps the stylus point in 0x000a77e0 and
   0x000a77e4, which the frame renderer 0x00039b1c reads and clears. */
int Platform_NextTap(int *x, int *y);

/* FFRace.exe WndProc 0x0004f6bc takes its back branch at 0x0004ffd8 when the
   WM_KEYDOWN code equals 0x000833c4 or 0x000a4378. */
int Platform_TakeBack(void);

unsigned Platform_Ticks(void);

void Platform_Delay(unsigned ms);

/* FFRace.exe Game_Init 0x000115b0 opens 0x000a4db0 with open(0x5622, 0x10, 0,
   1, 4) and calls volumeSounds(0x20); 0x000136d8 gives its ten hssSound objects
   volume 0x20 or 0x40. */
#define FF_MIX_VOLUME_MAX 128
#define FF_SOUND_SLOTS 10
#define FF_MASTER_VOLUME_MAX 0x40
#define FF_MUSIC_VOLUME_MAX 0x40

int Platform_AudioInit(void);
void Platform_AudioPause(int pause);

/* FFRace.exe 0x000136d8 follows every hssSound::load with loop(obj, 0) except
   0x000a4860, which takes loop(obj, 1); the flag belongs to the object, so
   0x0004f6bc reaches it through the ordinary playSound of 0x000a4db0. */
int Platform_SoundLoad(int slot, const char *path, int volume, int loop);
void Platform_SoundPlay(int slot);

/* FFRace.exe Race_Init 0x00013aec and Screen_Set 0x000149d4 both open with
   hssSpeaker::stopSounds(0x000a4db0), which drops every voice. */
void Platform_SoundStopAll(void);

void Platform_SoundMasterVolume(int volume);

/* FFRace.exe Race_Init 0x00013aec tail: hssSound::loop(0x000a4d10, 1) then
   hssSpeaker::playSound keeps the channel in 0x000a4858, so the engine sound
   holds a voice of its own for the whole race. */
void Platform_SoundLoop(int slot);
void Platform_SoundLoopStop(void);

/* FFRace.exe 0x00013aec reads the channel's own rate into 0x000a7748, and
   0x0004f6bc at 0x00050f88 writes frequency(0x000a4858, that rate scaled). */
int Platform_SoundRate(int slot);
void Platform_SoundLoopRate(int hz);

int Platform_MusicPlay(const char *path, int volume, int loop);
void Platform_MusicStop(void);
void Platform_MusicMasterVolume(int volume);
void Platform_AudioShutdown(void);

/* Directory the running executable lives in, with a trailing separator. */
const char *Platform_BasePath(void);

const char *Platform_PrefPath(void);

int Platform_FileExists(const char *path);

unsigned char *Platform_ReadFile(const char *path, long *out_len);

int Platform_TouchActive(void);

void Platform_ShowError(const char *title, const char *text);

const char *Platform_LastError(void);

#endif /* FF_PLATFORM_H */
