#include "ff_platform.h"
#include "ff_mod.h"

#include <SDL.h>
#include <stdlib.h>

#define FF_VOICES 4

/* FFRace.exe 0x00050f88 feeds frequency() a rate scaled off the channel's own,
   so the looping voice needs a fractional step; 16.16 covers 1.0 .. 2.0. */
#define FF_RATE_ONE   0x10000
#define FF_RATE_CHUNK 512

typedef struct {
    Uint8   *data;
    Uint32   len;
    int      volume;
    int      rate;
    int      loop;
    int      from_wav;
} ff_sound;

typedef struct {
    int    slot;
    Uint32 pos;
} ff_voice;

static ff_sound          ff_sounds[FF_SOUND_SLOTS];
static ff_voice          ff_voices[FF_VOICES];
static int               ff_loop_slot = -1;
static Uint32            ff_loop_pos;
static Uint32            ff_loop_step = FF_RATE_ONE;
static SDL_AudioDeviceID ff_audio_dev;
static SDL_AudioSpec     ff_audio_spec;

void Platform_AudioPause(int pause)
{
    if (ff_audio_dev != 0)
        SDL_PauseAudioDevice(ff_audio_dev, pause);
}

/* FFRace.exe Game_Init 0x000115b0 calls volumeSounds(0x000a4db0, 0x20) and
   0x000135dc keeps that setting at or below 0x40 in steps of 4. */
static int ff_master_vol = 0x20;

void Platform_SoundMasterVolume(int volume)
{
    ff_master_vol = volume;
}

/* FFRace.exe 0x00013aec holds the engine channel 0x000a4858 open for the whole
   race while 0x00050f88 rewrites its rate. */
static void MixLoop(Uint8 *out, int len)
{
    const ff_sound *snd;
    Uint32          frames;
    int             done;

    if (ff_loop_slot < 0)
        return;
    snd    = &ff_sounds[ff_loop_slot];
    frames = snd->len / 2;
    if (frames == 0)
        return;

    for (done = 0; done < len; ) {
        Sint16 tmp[FF_RATE_CHUNK];
        int    n = (len - done) / 2;
        int    i;

        if (n > FF_RATE_CHUNK)
            n = FF_RATE_CHUNK;
        if (n <= 0)
            break;
        for (i = 0; i < n; i++) {
            Uint32 f = ff_loop_pos >> 16;

            if (f >= frames) {
                ff_loop_pos -= frames << 16;
                f = ff_loop_pos >> 16;
            }
            tmp[i]       = ((const Sint16 *)snd->data)[f];
            ff_loop_pos += ff_loop_step;
        }
        SDL_MixAudioFormat(out + done, (const Uint8 *)tmp, ff_audio_spec.format,
                           (Uint32)n * 2,
                           snd->volume * ff_master_vol / FF_MASTER_VOLUME_MAX);
        done += n * 2;
    }
}

static void SDLCALL MixVoices(void *user, Uint8 *out, int len)
{
    int i;

    (void)user;
    Mod_Render((short *)out, len / 2);
    MixLoop(out, len);

    /* FFRace.exe 0x000136d8 gives 0x000a4860 hssSound::loop(1) and every other
       object loop(0). */
    for (i = 0; i < FF_VOICES; i++) {
        const ff_sound *snd;
        int             done = 0;

        if (ff_voices[i].slot < 0)
            continue;
        snd = &ff_sounds[ff_voices[i].slot];
        if (snd->len == 0) {
            ff_voices[i].slot = -1;
            continue;
        }

        while (done < len) {
            Uint32 left = snd->len - ff_voices[i].pos;
            Uint32 n    = (left < (Uint32)(len - done)) ? left
                                                       : (Uint32)(len - done);

            SDL_MixAudioFormat(out + done, snd->data + ff_voices[i].pos,
                               ff_audio_spec.format, n,
                               snd->volume * ff_master_vol
                                   / FF_MASTER_VOLUME_MAX);
            ff_voices[i].pos += n;
            done += (int)n;
            if (ff_voices[i].pos < snd->len)
                continue;
            if (!snd->loop) {
                ff_voices[i].slot = -1;
                break;
            }
            ff_voices[i].pos = 0;
        }
    }
}

/* FFRace.exe Game_Init 0x000115b0 opens 0x000a4db0 with hssSpeaker::open
   0x5622, 0x10, 0, 1, 4. */
int Platform_AudioInit(void)
{
    SDL_AudioSpec want;
    int           i;

    for (i = 0; i < FF_VOICES; i++)
        ff_voices[i].slot = -1;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return 0;

    SDL_zero(want);
    want.freq     = 22050;
    want.format   = AUDIO_S16LSB;
    want.channels = 1;
    want.samples  = 512;
    want.callback = MixVoices;

    ff_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &ff_audio_spec, 0);
    if (ff_audio_dev == 0)
        return 0;
    Mod_Init(ff_audio_spec.freq);
    SDL_PauseAudioDevice(ff_audio_dev, 0);
    return 1;
}

/* FFRace.exe Music_Select 0x00045b34: hssMusic::load(0x000a4d88, path) then
   loop(0x000a4d88, 1) and volume(0x000a4d88, 0x20 or 0x40). */
int Platform_MusicPlay(const char *path, int volume, int loop)
{
    SDL_RWops     *rw;
    unsigned char *buf;
    Sint64         size;
    int            ok;

    if (ff_audio_dev == 0)
        return 0;
    rw = SDL_RWFromFile(path, "rb");
    if (rw == NULL)
        return 0;
    size = SDL_RWsize(rw);
    if (size <= 0 || size > 0x400000) {
        SDL_RWclose(rw);
        return 0;
    }
    buf = (unsigned char *)malloc((size_t)size);
    if (buf == NULL) {
        SDL_RWclose(rw);
        return 0;
    }
    if (SDL_RWread(rw, buf, 1, (size_t)size) != (size_t)size) {
        SDL_RWclose(rw);
        free(buf);
        return 0;
    }
    SDL_RWclose(rw);

    SDL_LockAudioDevice(ff_audio_dev);
    ok = Mod_Play(buf, (int)size, volume, loop);
    SDL_UnlockAudioDevice(ff_audio_dev);
    return ok;
}

/* FFRace.exe Music_Select 0x00045b34 head: hssSpeaker::stopMusics(0x000a4db0). */
void Platform_MusicStop(void)
{
    if (ff_audio_dev == 0) {
        Mod_Stop();
        return;
    }
    SDL_LockAudioDevice(ff_audio_dev);
    Mod_Stop();
    SDL_UnlockAudioDevice(ff_audio_dev);
}

/* FFRace.exe 0x000135dc param_1 == 2: volumeMusics(0x000a4db0, param_2 * 4 +
   the current setting) while that sum stays below 0x41. */
void Platform_MusicMasterVolume(int volume)
{
    if (ff_audio_dev == 0) {
        Mod_MasterVolume(volume);
        return;
    }
    SDL_LockAudioDevice(ff_audio_dev);
    Mod_MasterVolume(volume);
    SDL_UnlockAudioDevice(ff_audio_dev);
}

int Platform_SoundLoad(int slot, const char *path, int volume, int loop)
{
    SDL_AudioSpec spec;
    SDL_AudioCVT  cvt;
    Uint8        *buf;
    Uint32        len;

    if (ff_audio_dev == 0 || slot < 0 || slot >= FF_SOUND_SLOTS)
        return 0;
    if (SDL_LoadWAV(path, &spec, &buf, &len) == NULL)
        return 0;
    if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
                          ff_audio_spec.format, ff_audio_spec.channels,
                          ff_audio_spec.freq) < 0) {
        SDL_FreeWAV(buf);
        return 0;
    }

    if (cvt.needed) {
        cvt.len = (int)len;
        cvt.buf = (Uint8 *)SDL_malloc((size_t)cvt.len * (size_t)cvt.len_mult);
        if (cvt.buf == NULL) {
            SDL_FreeWAV(buf);
            return 0;
        }
        SDL_memcpy(cvt.buf, buf, (size_t)len);
        SDL_FreeWAV(buf);
        if (SDL_ConvertAudio(&cvt) < 0) {
            SDL_free(cvt.buf);
            return 0;
        }
        ff_sounds[slot].data     = cvt.buf;
        ff_sounds[slot].len      = (Uint32)cvt.len_cvt;
        ff_sounds[slot].from_wav = 0;
    } else {
        ff_sounds[slot].data     = buf;
        ff_sounds[slot].len      = len;
        ff_sounds[slot].from_wav = 1;
    }
    /* FFRace.exe 0x00013aec reads the engine channel's own rate back into
       0x000a7748, which 0x00050f88 then scales. */
    ff_sounds[slot].rate   = spec.freq;
    ff_sounds[slot].volume = volume;
    ff_sounds[slot].loop   = loop;
    return 1;
}

/* FFRace.exe 0x0003a888 plays 0x000a4b30 when a menu plate is hit, and
   0x0004f6bc plays it again on the two ship-panel arrow bands, both through
   hssSpeaker::playSound(0x000a4db0, sound, 0x10000000). */
void Platform_SoundPlay(int slot)
{
    int i;

    if (ff_audio_dev == 0 || slot < 0 || slot >= FF_SOUND_SLOTS ||
        ff_sounds[slot].data == NULL)
        return;

    SDL_LockAudioDevice(ff_audio_dev);
    for (i = 0; i < FF_VOICES; i++) {
        if (ff_voices[i].slot < 0) {
            ff_voices[i].slot = slot;
            ff_voices[i].pos  = 0;
            break;
        }
    }
    SDL_UnlockAudioDevice(ff_audio_dev);
}

/* FFRace.exe Race_Init 0x00013aec tail: hssSound::loop(0x000a4d10, 1) then
   hssSpeaker::playSound(0x000a4db0, 0x000a4d10, 0x10000000) keeps the channel
   it returns in 0x000a4858. */
void Platform_SoundLoop(int slot)
{
    if (ff_audio_dev == 0 || slot < 0 || slot >= FF_SOUND_SLOTS ||
        ff_sounds[slot].data == NULL)
        return;

    SDL_LockAudioDevice(ff_audio_dev);
    ff_loop_slot = slot;
    ff_loop_pos  = 0;
    ff_loop_step = FF_RATE_ONE;
    SDL_UnlockAudioDevice(ff_audio_dev);
}

/* FFRace.exe Race_Init 0x00013aec head: hssSpeaker::stopSounds(0x000a4db0)
   drops the channel before the next one is opened. */
void Platform_SoundLoopStop(void)
{
    if (ff_audio_dev == 0) {
        ff_loop_slot = -1;
        return;
    }
    SDL_LockAudioDevice(ff_audio_dev);
    ff_loop_slot = -1;
    SDL_UnlockAudioDevice(ff_audio_dev);
}

/* FFRace.exe Screen_Set 0x000149d4 calls stopSounds before it repaints, so the
   one-shot voices go silent along with the retained channel. */
void Platform_SoundStopAll(void)
{
    int i;

    if (ff_audio_dev != 0)
        SDL_LockAudioDevice(ff_audio_dev);
    ff_loop_slot = -1;
    for (i = 0; i < FF_VOICES; i++)
        ff_voices[i].slot = -1;
    if (ff_audio_dev != 0)
        SDL_UnlockAudioDevice(ff_audio_dev);
}

/* FFRace.exe 0x00013aec stores hssSound::frequency() of the engine channel in
   0x000a7748. */
int Platform_SoundRate(int slot)
{
    if (slot < 0 || slot >= FF_SOUND_SLOTS)
        return 0;

    return ff_sounds[slot].rate;
}

/* FFRace.exe 0x00050f88 writes frequency(0x000a4858, 0x000a7748 * (1.0 +
   0x000a42c4 * 0.001)). */
void Platform_SoundLoopRate(int hz)
{
    int native;

    if (ff_audio_dev == 0 || ff_loop_slot < 0 || hz <= 0)
        return;
    native = ff_sounds[ff_loop_slot].rate;
    if (native <= 0)
        return;
    if (hz > native * 2)
        hz = native * 2;

    SDL_LockAudioDevice(ff_audio_dev);
    ff_loop_step = (Uint32)hz * FF_RATE_ONE / (Uint32)native;
    SDL_UnlockAudioDevice(ff_audio_dev);
}

void Platform_AudioShutdown(void)
{
    int i;

    if (ff_audio_dev != 0) {
        SDL_CloseAudioDevice(ff_audio_dev);
        ff_audio_dev = 0;
    }
    ff_loop_slot = -1;
    Mod_Stop();
    for (i = 0; i < FF_SOUND_SLOTS; i++) {
        if (ff_sounds[i].data == NULL)
            continue;
        if (ff_sounds[i].from_wav)
            SDL_FreeWAV(ff_sounds[i].data);
        else
            SDL_free(ff_sounds[i].data);
        ff_sounds[i].data = NULL;
    }
}
