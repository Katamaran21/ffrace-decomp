#include <stdlib.h>
#include <string.h>

#include "ff_mod.h"
#include "ff_mod_priv.h"

mod_state ff_M;

static int BeU16(const unsigned char *p)
{
    return (p[0] << 8) | p[1];
}

int Mod_Period(int idx)
{
    if (idx < 0) idx = 0;
    if (idx >= MOD_NOTES_ALL) idx = MOD_NOTES_ALL - 1;
    return (int)ff_mod_period[idx / FF_MOD_NOTES][idx % FF_MOD_NOTES];
}

unsigned int Mod_Step(int period)
{
    if (period < 0) period = 0;
    if (period > MOD_STEPMAX) period = MOD_STEPMAX;
    return ff_M.step[period];
}

unsigned int Mod_Cell(int ch)
{
    const unsigned char *p =
        ff_M.pat + (ff_M.cellofs + ff_M.patno * ff_M.chans * 0x40 + ch) * 4;

    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

/* jumpyball hss.dll hssSpeaker::findBestNoteIndex 0x00103604 */
static int FindBestNote(int period)
{
    int i, best = 0, bestd = -1;

    for (i = 0; i < FF_MOD_NOTES; i++) {
        int v = (int)ff_mod_period[0][i];
        int d = v < period ? period - v : v - period;

        if (v == period) return i;
        if (bestd < 0 || d < bestd) {
            bestd = d;
            best  = i;
        }
    }
    return best;
}

/* jumpyball hss.dll hssMusic::translatePTSign 0x00102648 */
static int TranslateSign(const unsigned char *p)
{
    int n = 0;

    if ((p[0] | 0x20) == 'm' && (p[2] | 0x20) == 'k') return 4;
    if ((p[1] | 0x20) != 'c' && (p[2] | 0x20) != 'c') return 0;
    if (p[0] >= '0' && p[0] <= '9') n = p[0] - '0';
    if (p[1] >= '0' && p[1] <= '9') n = n * 10 + (p[1] - '0');
    return n;
}

/* jumpyball hss.dll hssSpeaker::createTables 0x00103664 */
void Mod_Init(int out_rate)
{
    int p;

    ff_M.outrate   = out_rate;
    ff_M.mastervol = 0x40;
    for (p = 0; p < 0x1000; p++) ff_M.note[p] = (short)FindBestNote(p);
    ff_M.step[0] = 0;
    for (p = 1; p <= MOD_STEPMAX; p++)
        ff_M.step[p] =
            (unsigned int)((double)(MOD_CLOCK / p) * 65536.0 / (double)out_rate);
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
void Mod_SetTempo(void)
{
    int d = ((MOD_BASEFREQ << 8) / 0xac44) * ff_M.tickhz;

    if (d <= 0) d = 1;
    ff_M.spt = (int)(((unsigned int)ff_M.outrate << 8) / (unsigned int)d);
}

/* jumpyball hss.dll hssSpeaker::volumeMusics 0x0010567c */
void Mod_MasterVolume(int volume)
{
    if (volume > 0x40) volume = 0x40;
    if (volume < 0) volume = 0;
    ff_M.mastervol = volume;
    ff_M.volbase   = (ff_M.mastervol * ff_M.trackvol) >> 6;
}

/* jumpyball hss.dll hssSpeaker::stopMusics 0x001042f0 */
void Mod_Stop(void)
{
    ff_M.playing = 0;
    free(ff_M.file);
    ff_M.file = 0;
    ff_M.pat  = 0;
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static int MixFrame(void)
{
    int acc = 0, ch;

    for (ch = 0; ch < ff_M.chans; ch++) {
        mod_track  *t = &ff_M.tr[ch];
        mod_sample *s = &ff_M.smp[t->sample];

        if (s->data == 0) continue;
        if (s->replen != 0 && s->repstart + s->replen <= t->pos)
            t->pos -= s->replen;
        if (t->pos < s->len) {
            acc    += (int)s->data[t->pos >> 16] * t->mixvol;
            t->pos += t->step;
        }
    }
    return acc;
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void Tick(void)
{
    if (ff_M.tick == ff_M.speed) {
        if (ff_M.patdelay == 0) {
            Mod_ProcessRow();
            Mod_AdvanceRow();
            ff_M.tick = 0;
        } else {
            ff_M.tick = 0;
            ff_M.patdelay--;
        }
    } else if (ff_M.sctr < ff_M.spt) {
        ff_M.sctr++;
    } else {
        ff_M.sctr = 0;
        ff_M.tick++;
        if (ff_M.tick != ff_M.speed) Mod_TickEffects();
    }
}

/* jumpyball hss.dll hssSpeaker::playMusic 0x00103f1c */
static void LoadSamples(unsigned char *data, int size, unsigned char *sampbase)
{
    unsigned int running = 0;
    int i;

    for (i = 1; i < 32; i++) {
        const unsigned char *h = data + 30 * i + 12;
        mod_sample          *s = &ff_M.smp[i];
        long avail = (long)(data + size - sampbase) - (long)running;
        unsigned int bytes = (unsigned int)BeU16(h) * 2;

        if (avail < 0) avail = 0;
        if (bytes > (unsigned int)avail) bytes = (unsigned int)avail;
        s->data     = (const signed char *)(sampbase + running);
        s->len      = bytes << 16;
        s->finetune = h[2];
        s->vol      = h[3];
        s->repstart = (unsigned int)BeU16(h + 4) << 17;
        s->replen   = BeU16(h + 6) < 2 ? 0 : (unsigned int)BeU16(h + 6) << 17;
        running    += bytes;
    }
}

/* jumpyball hss.dll hssSpeaker::playMusic 0x00103f1c */
/* jumpyball hss.dll hssMusic_setVolume 0x00106228 */
int Mod_Play(unsigned char *data, int size, int volume, int loop)
{
    int chans, songlen, numpat, i;
    unsigned char *sampbase;

    Mod_Stop();
    if (data == 0) return 0;
    if (size < 0x43c) { free(data); return 0; }
    chans = TranslateSign(data + 0x438);
    if (chans < 1 || chans > 0x20) { free(data); return 0; }
    songlen = data[0x3b6];
    if (songlen < 1 || songlen > 0x80) { free(data); return 0; }
    numpat = 0;
    for (i = 0; i < songlen; i++)
        if (data[MOD_ORDER_OFS + i] > numpat) numpat = data[MOD_ORDER_OFS + i];
    numpat++;
    sampbase = data + 0x43c + numpat * chans * 0x100;
    if (sampbase > data + size) { free(data); return 0; }

    memset(ff_M.tr, 0, sizeof ff_M.tr);
    memset(ff_M.smp, 0, sizeof ff_M.smp);
    memset(ff_M.loopcnt, 0, sizeof ff_M.loopcnt);
    memset(ff_M.loopstart, 0, sizeof ff_M.loopstart);
    LoadSamples(data, size, sampbase);

    ff_M.file      = data;
    ff_M.pat       = data + 0x43c;
    ff_M.chans     = chans;
    ff_M.songlen   = songlen;
    ff_M.restart   = data[0x3b7];
    if (ff_M.restart == 0x7f || ff_M.restart >= songlen) ff_M.restart = 0;
    ff_M.orderidx  = 0;
    ff_M.cellofs   = 0;
    ff_M.patno     = data[MOD_ORDER_OFS];
    ff_M.speed     = 6;
    ff_M.tick      = 6;
    ff_M.tickhz    = 0x32;
    ff_M.sctr      = 0;
    ff_M.jumporder = MOD_NOJUMP;
    ff_M.jumpcell  = MOD_NOJUMP;
    ff_M.patdelay  = 0;
    ff_M.loop      = loop;
    ff_M.trackvol  = volume > 0x40 ? 0x40 : (volume < 0 ? 0 : volume);
    ff_M.volbase   = (ff_M.mastervol * ff_M.trackvol) >> 6;
    Mod_SetTempo();
    ff_M.playing   = 1;
    return 1;
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
void Mod_Render(short *out, int frames)
{
    int i;

    for (i = 0; i < frames; i++) {
        int v;

        if (!ff_M.playing) { out[i] = 0; continue; }
        Tick();
        v = MixFrame();
        if (v < -0x8000) v = -0x8000;
        if (v > 0x7fff)  v = 0x7fff;
        out[i] = (short)v;
    }
}
