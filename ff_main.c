#include "ff_appassets.h"
#include "ff_assets.h"
#include "ff_audio.h"
#include "ff_consts.h"
#include "ff_ingame.h"
#include "ff_menu.h"
#include "ff_physics.h"
#include "ff_platform.h"
#include "ff_render.h"
#include "ff_screen.h"
#include "ff_text.h"

#include <SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FF_MSG_MAX 2048
#define FF_ACTIVATE_MAX 16

static int ParseActivate(const char *s, int *out)
{
    int n = 0;

    while (*s != '\0' && n < FF_ACTIVATE_MAX) {
        out[n++] = atoi(s);
        while (*s != '\0' && *s != ',')
            s++;
        if (*s == ',')
            s++;
    }
    return n;
}

int main(int argc, char **argv)
{
    ff_surface *back;
    const char *dump_path = 0;
    unsigned    frame_start, elapsed;
    long        frames = 0, dump_frame = 60;
    int         scale = 0;
    int         tap_x, tap_y;
    int         activate[FF_ACTIVATE_MAX];
    int         activate_n = 0, activate_i = 0;
    int         farewell = 0;
    int         i;

    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "--dump=", 7))
            dump_path = argv[i] + 7;
        else if (!strncmp(argv[i], "--dump-frame=", 13))
            dump_frame = atol(argv[i] + 13);
        else if (!strncmp(argv[i], "--activate=", 11))
            activate_n = ParseActivate(argv[i] + 11, activate);
        else if (!scale)
            scale = atoi(argv[i]);
    }
    if (scale < 1)
        scale = 2;

    if (!Platform_Init(FF_VIEW_W, FF_VIEW_H, scale, "FFRace port")) {
        char msg[FF_MSG_MAX];

        snprintf(msg, sizeof msg, "SDL could not open the window:\n\n%s",
                 Platform_LastError());
        Platform_ShowError("FFRace", msg);
        Platform_Shutdown();
        return 1;
    }
    back = Platform_BackBuffer();

    if (!Assets_Init()) {
        char msg[FF_MSG_MAX];

        snprintf(msg, sizeof msg,
                 "FFRace cannot find its assets.\n\nLooked for BITMAP/%d.bmp in:\n%s\n"
                 "Copy the BITMAP, Sounds and Musics folders next to ffrace.exe,\n"
                 "or set FFRACE_ASSETS to the folder that holds them.",
                 FF_RES_FONT, Assets_FailureText());
        Platform_ShowError("FFRace", msg);
        Platform_Shutdown();
        return 1;
    }

    /* FFRace.exe Game_Init 0x000115b0 builds the scale tables before its first
       Screen_Set 0x00014924. */
    Gfx_BuildScaleTables();
    Render_BuildTables();
    Audio_Init();

    if (!Menu_Init(back)) {
        char msg[FF_MSG_MAX];

        snprintf(msg, sizeof msg,
                 "Menu bitmaps missing or unreadable under:\n\n%sBITMAP/",
                 Assets_Root());
        Platform_ShowError("FFRace", msg);
        Font_Free();
        AppAssets_Free();
        Audio_Shutdown();
        Platform_Shutdown();
        return 1;
    }
    /* FFRace.exe gates the 0x33 cursor at 0x000500bc on 0x00016b78(0x000a77d0)
       and the 0x000832f8 steps at 0x00014924 on 0x000833a4; neither global has
       a writer in the image, so both progressions stay at their first entry. */
    Screen_SetCareerRace(FF_CAREER_RACE_ALL);
    Physics_SetShipsUnlocked(FF_SHIPS_OPEN_ALL);

    Screen_Set(FF_SCREEN_MAIN, 0);
    if (activate_i < activate_n)
        Menu_Activate(activate[activate_i++]);

    while (Platform_PollEvents()) {
        frame_start = Platform_Ticks();

        /* FFRace.exe WndProc 0x0004f6bc: the 0x201 arm reaching 0x00050a3c
           returns on 0x00083380 != 0, and the 0x202 arm before 0x00050ab0
           selects on 0x00083488, which has no case 3. */
        while (Platform_NextTap(&tap_x, &tap_y)) {
            if (Ingame_Active())
                continue;
            if (!Screen_Tap(tap_x, tap_y))
                Menu_Touch(tap_x, tap_y);
        }

        if (Platform_TakeBack() && !Screen_Back()
            && Screen_Id() == FF_SCREEN_MAIN)
            break;

        /* FFRace.exe WndProc 0x0004f6bc WM_TIMER: 0x00083380 == 0 runs the
           physics pass and 0x0004e3c8; otherwise it draws the menu. */
        if (Ingame_Active()) {
            Ingame_Tick();
            Ingame_Frame(back);
        } else {
            Menu_Frame(back);
            if (Menu_TakeActivation()) {
                if (Screen_Activate(Screen_Id(), Screen_Param()))
                    farewell = 1;
                if (activate_i < activate_n)
                    Menu_Activate(activate[activate_i++]);
            }
        }

        if (farewell)
            Screen_Farewell(back);

        Platform_Present();
        frames++;

        if (dump_path != 0 && frames >= dump_frame) {
            FILE *fp = fopen(dump_path, "wb");

            if (fp != NULL) {
                fwrite(back->pixels, sizeof(uint16_t),
                       (size_t)FF_VIEW_W * (size_t)FF_VIEW_H, fp);
                fclose(fp);
            }
            printf("dump: %s frame=%ld screen=%d ingame=%d\n",
                   dump_path, frames, Screen_Id(), Ingame_Active());
            fflush(stdout);
            break;
        }

        if (farewell) {
            Platform_Delay(FF_FAREWELL_MS);
            break;
        }

        /* FFRace.exe WndProc 0x0004f6bc SetTimer(hwnd, 10, 5, 0). */
        elapsed = Platform_Ticks() - frame_start;
        if (elapsed < (unsigned)FF_FRAME_MS)
            Platform_Delay((unsigned)FF_FRAME_MS - elapsed);
    }

    Font_Free();
    AppAssets_Free();
    Audio_Shutdown();
    Platform_Shutdown();
    return 0;
}
