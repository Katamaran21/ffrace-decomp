#ifndef FF_TOUCH_SDL2_H
#define FF_TOUCH_SDL2_H

#include <SDL.h>

void Touch_Layout(int win_w, int win_h, int game_w, int game_h);

const SDL_Rect *Touch_GameRect(void);

void Touch_Handle(const SDL_Event *ev);

int Touch_Down(int key);

void Touch_Draw(SDL_Renderer *ren);

#endif /* FF_TOUCH_SDL2_H */
