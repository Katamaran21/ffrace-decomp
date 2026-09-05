#ifndef FF_MENU_H
#define FF_MENU_H

#include "ff_gfx.h"

/* FFRace.exe 0x00039b1c walks five plates from 0x000a45e0 and picks the caption
   pair with 0x000a7708. */
#define FF_MENU_ITEMS 5

/* FFRace.exe 0x00039b1c draws .data 0x0008434c "Exit" as the fifth label and
   pairs it with 0x0008429c "Return to" / 0x0008428c "Windows Mobile". */
#define FF_MENU_EXIT 4

int  Menu_Init(const ff_surface *dst);

void Menu_Frame(const ff_surface *dst);

/* FFRace.exe 0x00039b1c reads the pending touch from 0x000a77e0 / 0x000a77e4
   and the direct activation code from 0x000a7750, which it matches against
   item + 2. */
void Menu_Touch(int x, int y);
void Menu_Activate(int item);

/* FFRace.exe 0x000a7744 activation flag. */
int  Menu_TakeActivation(void);

/* FFRace.exe Game_Init 0x000115b0 layout marker 0x000a7630 and centre
   0x000832b0. */
int  Menu_Small(void);
int  Menu_CentreX(void);

/* FFRace.exe Game_Init 0x000115b0 colour key 0x000a7800. */
uint16_t Menu_Key(void);

/* FFRace.exe 0x00039b1c Text_DrawCentered y bases 0x4c 0x6d 0x8e 0xaf 0xd0 and
   243.0 / 258.0. */
int  Menu_LabelY(int i);
int  Menu_CaptionY(double base);

#endif /* FF_MENU_H */
