#include "ff_mod_priv.h"

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void VolSlide(mod_track *t)
{
    t->vol += t->volslide;
    if (t->vol < 0) t->vol = 0;
    if (t->vol > 0x40) t->vol = 0x40;
    t->mixvol = (t->vol * ff_M.volbase) >> 6;
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void RowLoop(int ch, mod_track *t)
{
    int b = ff_M.loopcnt[ch], p = t->parlo, hi = 0xf0;

    if ((b & 0xf0) == 0) {
        if (p == 0) {
            ff_M.loopstart[ch] = (unsigned char)ff_M.cellofs;
            return;
        }
    } else {
        if (p == 0) return;
        p = (b & 0xf) - 1;
    }
    if (p == 0) {
        hi = 0;
    } else {
        ff_M.jumporder = (unsigned int)ff_M.orderidx;
        ff_M.jumpcell  = ff_M.loopstart[ch];
    }
    ff_M.loopcnt[ch] = (unsigned char)(p | hi);
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void RowEffectE(int ch, mod_track *t, int cellperiod)
{
    switch (t->parhi) {
    case 0x6:
        RowLoop(ch, t);
        break;
    case 0xa:
        t->vol += t->parlo;
        if (t->vol > 0x40) t->vol = 0x40;
        t->mixvol = (t->vol * ff_M.volbase) >> 6;
        break;
    case 0xb:
        t->vol -= t->parlo;
        if (t->vol < 0) t->vol = 0;
        t->mixvol = (t->vol * ff_M.volbase) >> 6;
        break;
    case 0xc:
        t->notecut = t->parlo;
        break;
    case 0xd:
        t->notedelayperiod = cellperiod;
        t->notedelay       = t->parlo - 1;
        break;
    case 0xe:
        ff_M.patdelay = t->parlo;
        break;
    }
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void RowEffectHi(int ch, mod_track *t, int cellperiod)
{
    int param = t->param;

    switch (t->effect) {
    case 0xb:
        if (!ff_M.loop && (unsigned int)param <= (unsigned int)ff_M.orderidx) {
            ff_M.jumporder = (unsigned int)ff_M.songlen;
        } else {
            ff_M.jumporder = (unsigned int)param;
            ff_M.jumpcell  = 0;
        }
        break;
    case 0xc:
        t->vol    = param;
        t->mixvol = (param * ff_M.volbase) >> 6;
        break;
    case 0xd:
        if ((unsigned int)(ff_M.orderidx + 1) < (unsigned int)ff_M.songlen) {
            ff_M.jumporder = (unsigned int)(ff_M.orderidx + 1);
            ff_M.jumpcell  = (unsigned int)(t->parlo + t->parhi * 10) * 4;
        } else if (!ff_M.loop) {
            ff_M.jumporder = (unsigned int)ff_M.songlen;
        } else {
            ff_M.jumporder = (unsigned int)ff_M.restart;
            ff_M.jumpcell  = (unsigned int)(t->parlo + t->parhi * 10) * 4;
        }
        break;
    case 0xe:
        RowEffectE(ch, t, cellperiod);
        break;
    case 0xf:
        if (param > 0x20) {
            ff_M.tickhz = (param * 2) / 5;
            Mod_SetTempo();
        } else if (param != 0) {
            ff_M.speed = param;
        }
        break;
    }
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void RowEffect(int ch, mod_track *t, int cellperiod)
{
    int param = t->param;

    switch (t->effect) {
    case 0x0:
        if (param != 0) {
            t->arpstep[0] = t->step;
            t->arpphase   = 0;
            t->arpstep[1] = Mod_Step(Mod_Period(t->parhi + t->noteidx));
            t->arpstep[2] = Mod_Step(Mod_Period(t->parlo + t->noteidx));
        }
        break;
    case 0x1:
    case 0x2:
        t->portabase = t->period;
        break;
    case 0x3:
        if (cellperiod == 0) {
            if (t->portatarget == 0) t->portatarget = Mod_Period(t->noteidx);
        } else {
            t->portatarget = Mod_Period(ff_M.smp[t->sample].finetune * 0x54 +
                                        ff_M.note[cellperiod]);
        }
        if (param != 0) t->portaspeed = param;
        break;
    case 0x4:
        if (param != 0) {
            t->vibspeed = t->parhi;
            t->vibpos   = 0;
            t->vibdepth = t->parlo;
        }
        break;
    case 0x5:
    case 0x6:
    case 0xa:
        if (param == 0)         t->volslide = 0;
        else if (t->parhi == 0) t->volslide = -t->parlo;
        else                    t->volslide = t->parhi;
        break;
    case 0x9:
        if (param != 0)
            t->sampleofs = (unsigned int)(t->parlo + t->parhi * 0x10) * 0x1000000u;
        t->pos = t->sampleofs;
        break;
    default:
        RowEffectHi(ch, t, cellperiod);
        break;
    }
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
void Mod_ProcessRow(void)
{
    int ch;

    for (ch = 0; ch < ff_M.chans; ch++) {
        mod_track   *t = &ff_M.tr[ch];
        unsigned int w = Mod_Cell(ch);
        int sample     = (int)((w & 0xf0) | ((w & 0xffffff) >> 20));
        int cellperiod = (int)(((w & 0xffff) >> 8) | ((w & 0xf) << 8));

        t->param  = (int)(w >> 24);
        t->effect = (int)((w & 0xfffff) >> 16);
        t->parhi  = (int)(w >> 28);
        t->parlo  = t->param & 0xf;

        if (sample != 0) {
            t->sample = sample;
            t->vol    = ff_M.smp[sample].vol;
            t->mixvol = (t->vol * ff_M.volbase) >> 6;
        }
        if (cellperiod != 0 && t->effect != 3 && t->effect != 5 &&
            t->parhi + t->effect * 0x10 != 0xed) {
            t->pos     = 0;
            t->noteidx = ff_M.smp[t->sample].finetune * 0x54 +
                         ff_M.note[cellperiod];
            t->period  = Mod_Period(t->noteidx);
        }
        if (t->sample != 0) t->step = Mod_Step(t->period);
        RowEffect(ch, t, cellperiod);
    }
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
void Mod_AdvanceRow(void)
{
    if (ff_M.jumporder == MOD_NOJUMP) {
        ff_M.cellofs += ff_M.chans;
        if (ff_M.cellofs >= ff_M.chans * 0x40) {
            ff_M.cellofs = 0;
            ff_M.orderidx++;
            if (ff_M.orderidx >= ff_M.songlen) {
                if (!ff_M.loop) {
                    ff_M.playing = 0;
                    return;
                }
                ff_M.orderidx = ff_M.restart;
            }
            ff_M.patno = ff_M.file[MOD_ORDER_OFS + ff_M.orderidx];
        }
        return;
    }
    if (ff_M.jumporder >= (unsigned int)ff_M.songlen) {
        ff_M.playing = 0;
        return;
    }
    ff_M.orderidx  = (int)ff_M.jumporder;
    ff_M.patno     = ff_M.file[MOD_ORDER_OFS + ff_M.orderidx];
    ff_M.cellofs   = (int)ff_M.jumpcell;
    ff_M.jumporder = MOD_NOJUMP;
    if (ff_M.cellofs >= ff_M.chans * 0x40) ff_M.cellofs = 0;
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void TickEffectE(mod_track *t)
{
    if (t->parhi == 0xc) {
        if (t->notecut != 0) t->notecut--;
        if (t->notecut == 0) t->mixvol = 0;
    } else if (t->parhi == 0xd) {
        if (t->notedelay == -1) {
            t->notedelay = (int)MOD_NOJUMP;
            t->period    = t->notedelayperiod;
            t->step      = Mod_Step(t->notedelayperiod);
        } else if (t->notedelay != (int)MOD_NOJUMP) {
            t->notedelay--;
        }
    }
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void TickPorta(mod_track *t)
{
    unsigned int tgt = (unsigned int)t->portatarget, np;

    if (tgt < (unsigned int)t->period) {
        np        = (unsigned int)(t->period - t->portaspeed);
        t->period = (int)np;
        if (np < tgt || (int)np < 0) t->period = (int)tgt;
    }
    if ((unsigned int)t->period < tgt) {
        np        = (unsigned int)(t->portaspeed + t->period);
        t->period = (int)np;
        if (tgt < np) t->period = (int)tgt;
    }
    t->step = Mod_Step(t->period);
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
static void TickVibrato(mod_track *t)
{
    int vp = t->vibpos, per;

    if (vp < 0x20)
        per = t->period + ((ff_mod_vibrato[vp] * t->vibdepth) >> 7);
    else
        per = t->period - ((ff_mod_vibrato[vp - 0x20] * t->vibdepth) >> 7);
    vp += t->vibspeed;
    t->step = Mod_Step(per);
    if (vp > 0x3f) vp -= 0x40;
    t->vibpos = vp;
}

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
void Mod_TickEffects(void)
{
    int ch;

    for (ch = 0; ch <= ff_M.chans; ch++) {
        mod_track *t = &ff_M.tr[ch];

        switch (t->effect) {
        case 0x0:
            if (t->param != 0) {
                if (++t->arpphase == 3) t->arpphase = 0;
                t->step = t->arpstep[t->arpphase];
            }
            break;
        case 0x1:
            t->portabase -= t->param;
            if (t->portabase < 0xd) t->portabase = 0xd;
            t->period = t->portabase;
            t->step   = Mod_Step(t->period);
            break;
        case 0x2:
            t->portabase += t->param;
            if ((unsigned int)t->portabase > MOD_MAXPERIOD)
                t->portabase = MOD_MAXPERIOD;
            t->period = t->portabase;
            t->step   = Mod_Step(t->period);
            break;
        case 0x3:
            TickPorta(t);
            break;
        case 0x5:
            TickPorta(t);
            VolSlide(t);
            break;
        case 0x4:
            TickVibrato(t);
            break;
        case 0x6:
            TickVibrato(t);
            VolSlide(t);
            break;
        case 0xa:
            VolSlide(t);
            break;
        case 0xe:
            TickEffectE(t);
            break;
        }
    }
}
