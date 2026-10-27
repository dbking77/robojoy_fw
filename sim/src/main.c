#define _DEFAULT_SOURCE

#include "control_panel.h"
#include "hal_link.h"
#include "sdl_display.h"

#include "lvgl_main.h"
#include "joy_kb.h"

#include <stdio.h>
#include <unistd.h>

#define DEVICE_HOR 172
#define DEVICE_VER 320
#define DEVICE_ZOOM 2
#define PANEL_HOR 420
#define PANEL_VER 700

int main(void)
{
    lv_disp_t *device;
    lv_disp_t *panel;
    uint32_t last_tick;
    uint32_t last_joy;
    bool was_select = false;
    bool running = true;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    lv_init();
    JoyKb_Init();
    HalLink_Init();

    /* The device window is registered first and stays the LVGL default display. */
    device = SdlDisplay_Create("RoboJoy", DEVICE_HOR, DEVICE_VER, DEVICE_ZOOM, false);
    panel = SdlDisplay_Create("RoboJoy Hardware", PANEL_HOR, PANEL_VER, 1, true);
    if (device == NULL || panel == NULL) {
        SDL_Quit();
        return 1;
    }

    LvglMain_init();
    ControlPanel_Create(panel);

    last_tick = SDL_GetTicks();
    last_joy = last_tick;
    while (running) {
        SDL_Event event;
        uint32_t now;
        bool select;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) {
                running = false;
            }
            SdlDisplay_HandleEvent(&event);
        }

        now = SDL_GetTicks();
        lv_tick_inc(now - last_tick);
        last_tick = now;

        select = ControlPanel_IsSelectMode();
        if (select && !was_select) {
            JoyKb_Init();
        }
        was_select = select;

        if (now - last_joy >= 50) {
            last_joy = now;
            if (select) {
                int fwd;
                int right;
                int button;
                ControlPanel_GetJoy(&fwd, &right, &button);
                JoyKb_Update(fwd, right, button);
            }
        }

        lv_timer_handler();
        SdlDisplay_Present();
        usleep(5 * 1000);
    }

    SDL_Quit();
    return 0;
}
