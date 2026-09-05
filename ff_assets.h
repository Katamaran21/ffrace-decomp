#ifndef FF_ASSETS_H
#define FF_ASSETS_H

int Assets_Init(void);

const char *Assets_Root(void);
const char *Assets_FailureText(void);

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW(0x000a4854, res) resource ids. */
const char *Assets_Bitmap(int res);

/* FFRace.exe 0x000136d8: GetModuleFileNameW, wcsrchr for 0x00083538, then
   wcscat of 0x00083514 "\Sounds\sblam.wav" and its eight siblings. */
const char *Assets_Sound(const char *name);

/* FFRace.exe Music_Select 0x00045b34: the same GetModuleFileNameW and wcsrchr
   pair, then wcscat of 0x0008455c "\Musics\mainmenu.tkm". */
const char *Assets_Music(const char *name);

#endif /* FF_ASSETS_H */
