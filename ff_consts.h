#ifndef FF_CONSTS_H
#define FF_CONSTS_H

/* FFRace.exe .data 0x000832a8 / 0x000832ac initialisers 0xf0 / 0x140. */
#define FF_VIEW_W 240
#define FF_VIEW_H 320

/* FFRace.exe Game_Init 0x000115b0, ExtEscape(0x20001) width 0xb0 branch:
   0x000832a8 = 0xb0, 0x000832ac = 0xdc, 0x000832a4 = 0xdb, 0x000832b0 = 0x58,
   layout marker 0x000a7630 = 0x14. */
#define FF_SMALL_VIEW_W        176
#define FF_SMALL_VIEW_H        220
#define FF_SMALL_VIEW_CENTER_X 88
#define FF_LAYOUT_240x320      0
#define FF_LAYOUT_176x220      0x14

/* FFRace.exe Game_Init 0x000115b0, height 0xf0 branch: 0x000832ac = 0xf0,
   0x000832a4 = 0xef. */
#define FF_SQUARE_VIEW_H 240

/* FFRace.exe .data 0x000832a4 initialiser 0x100, tested by the frame renderer
   0x00039b1c against every road row span + 1. */
#define FF_VIEW_BOTTOM 256

/* FFRace.exe .data 0x000832b0 initialiser 0x78, the centre x passed to
   Text_DrawCentered 0x00013460. */
#define FF_VIEW_CENTER_X 120

/* FFRace.exe WndProc 0x0004f6bc SetTimer(hwnd, 10, 5, 0). */
#define FF_TIMER_ID 10
#define FF_FRAME_MS 5

/* FFRace.exe Screen_Set 0x00014924 arms the counter 0x000a77f4 with 0x14. */
#define FF_TRANSITION_FRAMES 20

/* FFRace.exe 0x0004e0c0 calls Sleep(500) between its last Gfx_Present
   0x000132e4 and PostQuitMessage(0). */
#define FF_FAREWELL_MS 500

/* FFRace.exe 0x0004f6bc at 0x00050000 reaches Screen_Set(1,0), (3,0), (4,0) and
   (2,4) from menu parameters 0..3; Music_Select 0x00045b34 takes mainmenu.tkm
   for 0x00083488 in {0,1} and menucareer.tkm for {3,5}, and the other values
   compared there and in 0x00039b1c are 6, 7, 10, 0x2a, 0x2b, 0x33. */
#define FF_SCREEN_MAIN        0
#define FF_SCREEN_PRACTICE    1
#define FF_SCREEN_ABOUT       2
#define FF_SCREEN_CAREER_MENU 3
#define FF_SCREEN_SETTINGS    4
#define FF_SCREEN_CAREER_SUB  5
#define FF_SCREEN_SHIP        6
#define FF_SCREEN_GARAGE      7
#define FF_SCREEN_KEY         10
#define FF_SCREEN_SOUND       0x2a
#define FF_SCREEN_GAMEPLAY    0x2b
#define FF_SCREEN_TRACK       0x33

/* FFRace.exe Font_Load 0x00012f98: argument 1 takes LoadBitmapW 0x14a and
   scans row 0 for 0xff00ff pairs over 0x4ba pixels; any other argument takes
   0xfe and scans for a column packed 0 through all 14 rows of stride 0x4bc. */
#define FF_RES_FONT        330
#define FF_RES_FONT_COLUMN 254

/* FFRace.exe Text_DrawCentered 0x00013460 and Text_DrawGlyph 0x00013548 pass
   0xe as the Blit_Glyph height and 0 as the source row. */
#define FF_GLYPH_H 14

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW block: 0x000a4634 takes 0x110,
   0x000a4638 takes 0x111 and 0x000a463c takes 0x112. */
#define FF_RES_RESULTS   272
#define FF_RES_COUNTDOWN 273
#define FF_RES_GO        274

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW block: 0x000a4584 takes 0xe2,
   0x000a4588 takes 0xe1, 0x000a458c takes 0xe4 and 0x000a4590 takes 0xe3. */
#define FF_RES_DAMAGE_BACK 226
#define FF_RES_DAMAGE_FILL 225
#define FF_RES_SPEED_BACK  228
#define FF_RES_SPEED_FILL  227

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW block: 0x000a4684 takes 0x124,
   0x000a4688 takes 0x125 and 0x000a468c takes 0x126. */
#define FF_RES_STRIP        292
#define FF_RES_STRIP_DAMAGE 293
#define FF_RES_STRIP_SPEED  294

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW block: 0x000a4508 takes 299 and
   0x000a456c takes 0xd8; the table 0x000a44e0 takes 0xd3, 0xd2, 0xd4, 0xd5,
   0xd6 and 0x157 into slots 1 .. 6, and holds no other sprite. */
#define FF_RES_SHADOW    299
#define FF_RES_WRECK     216
#define FF_SHIP_SPRITES  6

/* FFRace.exe Game_Init 0x000115b0 LoadBitmapW block: 0x000a45e4 takes 0xfb,
   0x000a469c takes 300 and 0x000a46a8 takes 0x12f; 0x00039b1c reads the first
   on screen 0x33, the second on screens 6 and 7 and the third on 0x33. */
#define FF_RES_TRACK_PANEL 251
#define FF_RES_SHIP_PANEL  300
#define FF_RES_ARROWS      303

/* FFRace.exe 0x00016b78 returns its highest theme 9 for 0x000a77d0 in 0x1e ..
   0x24, and the sprite table 0x000a44e0 fills slots 1 .. 6, so these are the
   values 0x000a77d0 and 0x000833a4 hold once a career has been won out. */
#define FF_CAREER_RACE_ALL   0x1e
#define FF_SHIPS_OPEN_ALL    FF_SHIP_SPRITES

/* FFRace.exe .data 0x000832e4 initialiser 25; 0x0004f6bc compares 0x000a7728
   against it at the 0x000a4860 playSound and again at the chase gate that
   follows the 0x000a77c4 block, and 0x0004d3bc, 0x0004d404, 0x0004e024,
   0x0004e058 and 0x0004e08c offset it by 4, 0xc, 8, 6 and 4. */
#define FF_COPS_SEC 25

#endif /* FF_CONSTS_H */
