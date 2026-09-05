#ifndef FF_AUDIO_H
#define FF_AUDIO_H

/* FFRace.exe 0x000136d8 loads nine hssSound objects - 0x000a4ab8 sblam.wav,
   0x000a4b30 smenu.wav, 0x000a4c20 sexpl.wav, 0x000a4ba8 smiss.wav and the
   embedded 0x000a4a40, 0x000a49c8, 0x000a4950, 0x000a48d8, 0x000a4860 - and
   Race_Init 0x00013aec loads 0x000a4d10 from 0x0005c000. */
enum {
    FF_SND_BLAM,
    FF_SND_MENU,
    FF_SND_EXPL,
    FF_SND_MISS,
    FF_SND_BEEP3,
    FF_SND_BEEP2,
    FF_SND_BEEP1,
    FF_SND_GO,
    FF_SND_SIREN,
    FF_SND_ENGINE,
    FF_SND_COUNT
};

void Audio_Init(void);
void Audio_Shutdown(void);

void Audio_Play(int snd);

/* FFRace.exe Race_Init 0x00013aec head and Screen_Set 0x000149d4:
   hssSpeaker::stopSounds(0x000a4db0). */
void Audio_StopSounds(void);

/* FFRace.exe Race_Init 0x00013aec tail: volume(0x000a4d10, 0x18),
   loop(0x000a4d10, 1), then playSound keeps the channel in 0x000a4858. */
void Audio_EngineStart(void);

/* FFRace.exe 0x00050f88: frequency(0x000a4858, 0x000a7748 * (1.0 + 0x000a42c4
   * 0.001)), refreshed while 0x000a42dc stays above 0.1. */
void Audio_EnginePitch(double speed);

/* FFRace.exe Music_Select 0x00045b34 takes mainmenu.tkm at 0x20 for screens 0
   and 1, menucareer.tkm at 0x40 for 3 and 5, otherwise course{rand() % 9 +
   1}.tkm at 0x20; only the menu branch reaches its own playMusic. */
void Audio_MusicSelect(int screen, int in_menu, int intro_pending);

/* FFRace.exe 0x0004f6bc plays 0x000a4d88 when 0x000a7728 reaches 1. */
void Audio_MusicStart(void);

void Audio_MusicStop(void);

/* FFRace.exe Game_Init 0x000115b0 volumeSounds(0x20); 0x000135dc rewrites both
   speaker volumes after every step. */
void Audio_PushVolumes(void);

#endif /* FF_AUDIO_H */
