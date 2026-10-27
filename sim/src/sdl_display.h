#pragma once

#include "lvgl.h"

#include <SDL.h>
#include <stdbool.h>

lv_disp_t *SdlDisplay_Create(const char *title, lv_coord_t hor_res, lv_coord_t ver_res, int zoom, bool with_pointer);
void SdlDisplay_HandleEvent(const SDL_Event *event);
void SdlDisplay_Present(void);
