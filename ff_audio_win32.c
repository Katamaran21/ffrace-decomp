/* Native waveOut mixer, the counterpart of ff_audio_sdl2.c.  It keeps the same
   three sources in the same order - the module renderer, the retained engine
   channel and the four one-shot voices - because FFRace.exe drives all three
   through one hssSpeaker. */
#include "ff_platform.h"
#include "ff_mod.h"

#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>

#define FF_VOICES  4
#define FF_RATE    22050
#define FF_BUFFERS 4

#ifdef FF_WINCE
#define FF_FRAMES 1024
#else
#define FF_FRAMES 512
#endif

/* FFRace.exe 0x00050f88 feeds frequency() a rate scaled off the channel's own,
   so the looping voice needs a fractional step; 16.16 covers 1.0 .. 2.0. */
#define FF_RATE_ONE   0x10000
#define FF_RATE_CHUNK 512

typedef struct {
    short   *data;
    unsigned len;
    int      volume;
    int      rate;
    int      loop;
} ff_sound;

typedef struct {
    int      slot;
    unsigned pos;
} ff_voice;

static ff_sound ff_sounds[FF_SOUND_SLOTS];
static ff_voice ff_voices[FF_VOICES];
static int      ff_loop_slot = -1;
static unsigned ff_loop_pos;
static unsigned ff_loop_step = FF_RATE_ONE;

/* FFRace.exe Game_Init 0x000115b0 calls volumeSounds(0x000a4db0, 0x20) and
   0x000135dc keeps that setting at or below 0x40 in steps of 4. */
static int ff_master_vol = 0x20;

static HWAVEOUT        ff_wave;
static WAVEHDR         ff_hdr[FF_BUFFERS];
static short          *ff_buf[FF_BUFFERS];
static HANDLE          ff_event;
static HANDLE          ff_thread;
static volatile LONG   ff_thread_run;
static CRITICAL_SECTION ff_lock;
static int             ff_lock_ready;

static void Lock(void)
{
    if (ff_lock_ready)
        EnterCriticalSection(&ff_lock);
}

static void Unlock(void)
{
    if (ff_lock_ready)
        LeaveCriticalSection(&ff_lock);
}

static void MixS16(short *dst, const short *src, int n, int vol)
{
    int i;

    if (vol <= 0)
        return;
    for (i = 0; i < n; i++) {
        int v = dst[i] + src[i] * vol / FF_MIX_VOLUME_MAX;

        if (v < -0x8000)
            v = -0x8000;
        if (v > 0x7fff)
            v = 0x7fff;
        dst[i] = (short)v;
    }
}

/* FFRace.exe 0x00013aec holds the engine channel 0x000a4858 open for the whole
   race while 0x00050f88 rewrites its rate. */
static void MixLoop(short *out, int frames)
{
    const ff_sound *snd;
    unsigned        total;
    int             done;

    if (ff_loop_slot < 0)
        return;
    snd   = &ff_sounds[ff_loop_slot];
    total = snd->len;
    if (total == 0)
        return;

    for (done = 0; done < frames;) {
        short tmp[FF_RATE_CHUNK];
        int   n = frames - done;
        int   i;

        if (n > FF_RATE_CHUNK)
            n = FF_RATE_CHUNK;
        if (n <= 0)
            break;
        for (i = 0; i < n; i++) {
            unsigned f = ff_loop_pos >> 16;

            if (f >= total) {
                ff_loop_pos -= total << 16;
                f = ff_loop_pos >> 16;
            }
            tmp[i] = snd->data[f];
            ff_loop_pos += ff_loop_step;
        }
        MixS16(out + done, tmp, n,
               snd->volume * ff_master_vol / FF_MASTER_VOLUME_MAX);
        done += n;
    }
}

static void FillBuffer(short *out, int frames)
{
    int i;

    Lock();
    Mod_Render(out, frames);
    MixLoop(out, frames);

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

        while (done < frames) {
            unsigned left = snd->len - ff_voices[i].pos;
            unsigned n    = (left < (unsigned)(frames - done))
                             ? left
                             : (unsigned)(frames - done);

            MixS16(out + done, snd->data + ff_voices[i].pos, (int)n,
                   snd->volume * ff_master_vol / FF_MASTER_VOLUME_MAX);
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
    Unlock();
}

static DWORD WINAPI FeedThread(LPVOID arg)
{
    (void)arg;
    while (ff_thread_run) {
        int i;

        WaitForSingleObject(ff_event, 10);
        for (i = 0; i < FF_BUFFERS; i++) {
            if ((ff_hdr[i].dwFlags & WHDR_DONE) == 0)
                continue;
            FillBuffer(ff_buf[i], FF_FRAMES);
            ff_hdr[i].dwFlags &= ~WHDR_DONE;
            waveOutWrite(ff_wave, &ff_hdr[i], sizeof ff_hdr[i]);
        }
    }
    return 0;
}

/* FFRace.exe Game_Init 0x000115b0 opens 0x000a4db0 with hssSpeaker::open
   0x5622, 0x10, 0, 1, 4. */
int Platform_AudioInit(void)
{
    WAVEFORMATEX wfx;
    DWORD        tid;
    int          i;

    for (i = 0; i < FF_VOICES; i++)
        ff_voices[i].slot = -1;

    memset(&wfx, 0, sizeof wfx);
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 1;
    wfx.nSamplesPerSec  = FF_RATE;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = 2;
    wfx.nAvgBytesPerSec = FF_RATE * 2;

    ff_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (ff_event == NULL)
        return 0;
    if (waveOutOpen(&ff_wave, WAVE_MAPPER, &wfx, (DWORD_PTR)ff_event, 0,
                    CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        CloseHandle(ff_event);
        ff_event = NULL;
        return 0;
    }

    InitializeCriticalSection(&ff_lock);
    ff_lock_ready = 1;
    Mod_Init(FF_RATE);

    for (i = 0; i < FF_BUFFERS; i++) {
        ff_buf[i] = (short *)calloc(FF_FRAMES, sizeof(short));
        if (ff_buf[i] == NULL)
            return 0;
        memset(&ff_hdr[i], 0, sizeof ff_hdr[i]);
        ff_hdr[i].lpData         = (LPSTR)ff_buf[i];
        ff_hdr[i].dwBufferLength = FF_FRAMES * sizeof(short);
        waveOutPrepareHeader(ff_wave, &ff_hdr[i], sizeof ff_hdr[i]);
        waveOutWrite(ff_wave, &ff_hdr[i], sizeof ff_hdr[i]);
    }

    ff_thread_run = 1;
    ff_thread     = CreateThread(NULL, 0, FeedThread, NULL, 0, &tid);
    if (ff_thread == NULL) {
        ff_thread_run = 0;
        return 0;
    }
    return 1;
}

void Platform_AudioPause(int pause)
{
    if (ff_wave == NULL)
        return;
    if (pause)
        waveOutPause(ff_wave);
    else
        waveOutRestart(ff_wave);
}

static unsigned Rd16(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned Rd32(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) |
           ((unsigned)p[3] << 24);
}

static short SampleAt(const unsigned char *d, unsigned idx, int channels,
                      int bits)
{
    unsigned base = idx * (unsigned)channels;
    int      sum  = 0;
    int      c;

    for (c = 0; c < channels; c++) {
        if (bits == 8)
            sum += ((int)d[base + (unsigned)c] - 0x80) << 8;
        else
            sum += (short)(unsigned short)Rd16(d + (base + (unsigned)c) * 2u);
    }
    return (short)(sum / channels);
}

/* RIFF/WAVE as documented in the Multimedia Programming Interface and Data
   Specifications 1.0 (IBM/Microsoft, August 1991) section "WAVE Format". */
static int LoadWav(const char *path, short **out_data, unsigned *out_frames,
                   int *out_rate)
{
    unsigned char       *raw;
    long                 raw_len = 0;
    unsigned             off;
    const unsigned char *fmt  = NULL;
    const unsigned char *data = NULL;
    unsigned             data_len = 0;
    unsigned             src_frames, dst_frames, j;
    int                  channels, bits, rate;
    short               *dst;

    raw = Platform_ReadFile(path, &raw_len);
    if (raw == NULL)
        return 0;
    if (raw_len < 44 || memcmp(raw, "RIFF", 4) != 0 ||
        memcmp(raw + 8, "WAVE", 4) != 0) {
        free(raw);
        return 0;
    }

    for (off = 12; off + 8 <= (unsigned)raw_len;) {
        unsigned size = Rd32(raw + off + 4);

        if (off + 8 + size > (unsigned)raw_len)
            size = (unsigned)raw_len - off - 8;
        if (memcmp(raw + off, "fmt ", 4) == 0 && size >= 16)
            fmt = raw + off + 8;
        else if (memcmp(raw + off, "data", 4) == 0) {
            data     = raw + off + 8;
            data_len = size;
        }
        off += 8 + size + (size & 1u);
    }
    if (fmt == NULL || data == NULL || Rd16(fmt) != 1) {
        free(raw);
        return 0;
    }

    channels = (int)Rd16(fmt + 2);
    rate     = (int)Rd32(fmt + 4);
    bits     = (int)Rd16(fmt + 14);
    if ((channels != 1 && channels != 2) || (bits != 8 && bits != 16) ||
        rate <= 0) {
        free(raw);
        return 0;
    }

    src_frames = data_len / (unsigned)(channels * bits / 8);
    if (src_frames == 0) {
        free(raw);
        return 0;
    }
    dst_frames = (unsigned)((double)src_frames * (double)FF_RATE / (double)rate);
    if (dst_frames == 0)
        dst_frames = 1;

    dst = (short *)malloc((size_t)dst_frames * sizeof(short));
    if (dst == NULL) {
        free(raw);
        return 0;
    }
    for (j = 0; j < dst_frames; j++) {
        unsigned s = (unsigned)((double)j * (double)rate / (double)FF_RATE);

        if (s >= src_frames)
            s = src_frames - 1;
        dst[j] = SampleAt(data, s, channels, bits);
    }
    free(raw);

    *out_data   = dst;
    *out_frames = dst_frames;
    *out_rate   = rate;
    return 1;
}

int Platform_SoundLoad(int slot, const char *path, int volume, int loop)
{
    short   *data  = NULL;
    unsigned frames = 0;
    int      rate  = 0;

    if (slot < 0 || slot >= FF_SOUND_SLOTS)
        return 0;
    if (!LoadWav(path, &data, &frames, &rate))
        return 0;

    Lock();
    free(ff_sounds[slot].data);
    ff_sounds[slot].data = data;
    ff_sounds[slot].len  = frames;
    /* FFRace.exe 0x00013aec reads the engine channel's own rate back into
       0x000a7748, which 0x00050f88 then scales. */
    ff_sounds[slot].rate   = rate;
    ff_sounds[slot].volume = volume;
    ff_sounds[slot].loop   = loop;
    Unlock();
    return 1;
}

/* FFRace.exe 0x0003a888 plays 0x000a4b30 when a menu plate is hit, and
   0x0004f6bc plays it again on the two ship-panel arrow bands, both through
   hssSpeaker::playSound(0x000a4db0, sound, 0x10000000). */
void Platform_SoundPlay(int slot)
{
    int i;

    if (slot < 0 || slot >= FF_SOUND_SLOTS || ff_sounds[slot].data == NULL)
        return;

    Lock();
    for (i = 0; i < FF_VOICES; i++) {
        if (ff_voices[i].slot < 0) {
            ff_voices[i].slot = slot;
            ff_voices[i].pos  = 0;
            break;
        }
    }
    Unlock();
}

/* FFRace.exe Race_Init 0x00013aec tail: hssSound::loop(0x000a4d10, 1) then
   hssSpeaker::playSound(0x000a4db0, 0x000a4d10, 0x10000000) keeps the channel
   it returns in 0x000a4858. */
void Platform_SoundLoop(int slot)
{
    if (slot < 0 || slot >= FF_SOUND_SLOTS || ff_sounds[slot].data == NULL)
        return;

    Lock();
    ff_loop_slot = slot;
    ff_loop_pos  = 0;
    ff_loop_step = FF_RATE_ONE;
    Unlock();
}

/* FFRace.exe Race_Init 0x00013aec head: hssSpeaker::stopSounds(0x000a4db0)
   drops the channel before the next one is opened. */
void Platform_SoundLoopStop(void)
{
    Lock();
    ff_loop_slot = -1;
    Unlock();
}

/* FFRace.exe Screen_Set 0x000149d4 calls stopSounds before it repaints, so the
   one-shot voices go silent along with the retained channel. */
void Platform_SoundStopAll(void)
{
    int i;

    Lock();
    ff_loop_slot = -1;
    for (i = 0; i < FF_VOICES; i++)
        ff_voices[i].slot = -1;
    Unlock();
}

void Platform_SoundMasterVolume(int volume)
{
    ff_master_vol = volume;
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

    if (ff_loop_slot < 0 || hz <= 0)
        return;
    native = ff_sounds[ff_loop_slot].rate;
    if (native <= 0)
        return;
    if (hz > native * 2)
        hz = native * 2;

    Lock();
    ff_loop_step = (unsigned)hz * FF_RATE_ONE / (unsigned)native;
    Unlock();
}

/* FFRace.exe Music_Select 0x00045b34: hssMusic::load(0x000a4d88, path) then
   loop(0x000a4d88, 1) and volume(0x000a4d88, 0x20 or 0x40). */
int Platform_MusicPlay(const char *path, int volume, int loop)
{
    unsigned char *buf;
    long           size = 0;
    int            ok;

    buf = Platform_ReadFile(path, &size);
    if (buf == NULL)
        return 0;
    if (size > 0x400000) {
        free(buf);
        return 0;
    }

    Lock();
    ok = Mod_Play(buf, (int)size, volume, loop);
    Unlock();
    return ok;
}

/* FFRace.exe Music_Select 0x00045b34 head: hssSpeaker::stopMusics(0x000a4db0). */
void Platform_MusicStop(void)
{
    Lock();
    Mod_Stop();
    Unlock();
}

/* FFRace.exe 0x000135dc param_1 == 2: volumeMusics(0x000a4db0, param_2 * 4 +
   the current setting) while that sum stays below 0x41. */
void Platform_MusicMasterVolume(int volume)
{
    Lock();
    Mod_MasterVolume(volume);
    Unlock();
}

void Platform_AudioShutdown(void)
{
    int i;

    if (ff_thread != NULL) {
        ff_thread_run = 0;
        SetEvent(ff_event);
        WaitForSingleObject(ff_thread, 1000);
        CloseHandle(ff_thread);
        ff_thread = NULL;
    }
    if (ff_wave != NULL) {
        waveOutReset(ff_wave);
        for (i = 0; i < FF_BUFFERS; i++) {
            if (ff_buf[i] == NULL)
                continue;
            waveOutUnprepareHeader(ff_wave, &ff_hdr[i], sizeof ff_hdr[i]);
        }
        waveOutClose(ff_wave);
        ff_wave = NULL;
    }
    if (ff_event != NULL) {
        CloseHandle(ff_event);
        ff_event = NULL;
    }
    for (i = 0; i < FF_BUFFERS; i++) {
        free(ff_buf[i]);
        ff_buf[i] = NULL;
    }

    ff_loop_slot = -1;
    Mod_Stop();
    for (i = 0; i < FF_SOUND_SLOTS; i++) {
        free(ff_sounds[i].data);
        ff_sounds[i].data = NULL;
        ff_sounds[i].len  = 0;
    }
    if (ff_lock_ready) {
        DeleteCriticalSection(&ff_lock);
        ff_lock_ready = 0;
    }
}
