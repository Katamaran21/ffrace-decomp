#ifndef FF_MOD_H
#define FF_MOD_H

/* jumpyball hss.dll hssSpeaker::createTables 0x00103664 */
void Mod_Init(int out_rate);

/* jumpyball hss.dll hssSpeaker::playMusic 0x00103f1c,
   hssMusic_setVolume 0x00106228 clamps volume to 0x40 */
int  Mod_Play(unsigned char *data, int size, int volume, int loop);

/* jumpyball hss.dll hssSpeaker::stopMusics 0x001042f0 */
void Mod_Stop(void);

/* jumpyball hss.dll hssSpeaker::volumeMusics 0x0010567c */
void Mod_MasterVolume(int volume);

/* jumpyball hss.dll hssSpeaker::updateModSFX 0x00104370 */
void Mod_Render(short *out, int frames);

#endif /* FF_MOD_H */
