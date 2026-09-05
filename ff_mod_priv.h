#ifndef FF_MOD_PRIV_H
#define FF_MOD_PRIV_H

#include "ff_mod_table.h"

#define MOD_CLOCK     7159090
#define MOD_MAXPERIOD 0x6b0
#define MOD_STEPMAX   0x7ff
#define MOD_CHANS     32
#define MOD_NOJUMP    0xffffff
#define MOD_BASEFREQ  0xac44
#define MOD_NOTES_ALL (FF_MOD_FINETUNES * FF_MOD_NOTES)
#define MOD_ORDER_OFS 0x3b8

typedef struct {
    const signed char *data;
    unsigned int       len;
    unsigned int       repstart;
    unsigned int       replen;
    int                vol;
    int                finetune;
} mod_sample;

typedef struct {
    int          period, sample, vol, mixvol, noteidx;
    int          effect, param, parhi, parlo;
    unsigned int step, pos, sampleofs;
    int          volslide, portabase, portatarget, portaspeed;
    int          arpphase;
    unsigned int arpstep[3];
    int          vibpos, vibspeed, vibdepth;
    int          notedelay, notedelayperiod, notecut;
} mod_track;

typedef struct {
    unsigned char *file;
    unsigned char *pat;
    int            chans, songlen, restart;
    int            orderidx, patno, cellofs;
    int            speed, tick, tickhz, spt, sctr;
    unsigned int   jumporder, jumpcell;
    int            patdelay, loop, playing;
    int            trackvol, mastervol, volbase, outrate;
    unsigned int   step[MOD_STEPMAX + 1];
    short          note[0x1000];
    mod_sample     smp[256];
    mod_track      tr[MOD_CHANS + 1];
    unsigned char  loopcnt[MOD_CHANS];
    unsigned char  loopstart[MOD_CHANS];
} mod_state;

extern mod_state ff_M;

int          Mod_Period(int idx);
unsigned int Mod_Step(int period);
unsigned int Mod_Cell(int ch);
void         Mod_SetTempo(void);
void         Mod_ProcessRow(void);
void         Mod_AdvanceRow(void);
void         Mod_TickEffects(void);

#endif
