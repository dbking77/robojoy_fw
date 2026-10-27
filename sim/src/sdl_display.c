#include "sdl_display.h"

#include <stdio.h>
#include <stdlib.h>

#define SDL_WINDOW_MAX 2

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    lv_disp_drv_t drv;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t *buf;
    int hor;
    int ver;
    int zoom;
    Uint32 window_id;
    bool dirty;
    bool pointer;
} SdlWindow;

static SdlWindow g_windows[SDL_WINDOW_MAX];
static int g_window_count;
static int g_next_x = 40;

static lv_indev_drv_t g_pointer_drv;
static int g_pointer_x;
static int g_pointer_y;
static bool g_pointer_pressed;
static Uint32 g_pointer_window;

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    SdlWindow *win = drv->user_data;
    SDL_Rect rect;

    rect.x = area->x1;
    rect.y = area->y1;
    rect.w = lv_area_get_width(area);
    rect.h = lv_area_get_height(area);
    if (rect.w > 0 && rect.h > 0) {
        SDL_UpdateTexture(win->texture, &rect, color_p, rect.w * (int)sizeof(lv_color_t));
        win->dirty = true;
    }
    lv_disp_flush_ready(drv);
}

static void pointer_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    data->point.x = (lv_coord_t)g_pointer_x;
    data->point.y = (lv_coord_t)g_pointer_y;
    data->state = g_pointer_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static SdlWindow *window_from_id(Uint32 window_id)
{
    for (int i = 0; i < g_window_count; ++i) {
        if (g_windows[i].window_id == window_id) {
            return &g_windows[i];
        }
    }
    return NULL;
}

lv_disp_t *SdlDisplay_Create(const char *title, lv_coord_t hor_res, lv_coord_t ver_res, int zoom, bool with_pointer)
{
    if (g_window_count >= SDL_WINDOW_MAX || zoom < 1) {
        return NULL;
    }

    SdlWindow *win = &g_windows[g_window_count];
    int window_w = hor_res * zoom;
    int window_h = ver_res * zoom;
    uint32_t buf_h = 40;
    if (buf_h > (uint32_t)ver_res) {
        buf_h = (uint32_t)ver_res;
    }

    win->hor = hor_res;
    win->ver = ver_res;
    win->zoom = zoom;
    win->pointer = with_pointer;
    win->buf = malloc((size_t)hor_res * buf_h * sizeof(lv_color_t));
    if (win->buf == NULL) {
        fprintf(stderr, "display buffer alloc failed\n");
        return NULL;
    }

    win->window = SDL_CreateWindow(title, g_next_x, 40, window_w, window_h, SDL_WINDOW_SHOWN);
    if (win->window == NULL) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        free(win->buf);
        return NULL;
    }
    g_next_x += window_w + 24;

    win->renderer = SDL_CreateRenderer(win->window, -1, SDL_RENDERER_ACCELERATED);
    if (win->renderer == NULL) {
        win->renderer = SDL_CreateRenderer(win->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (win->renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win->window);
        free(win->buf);
        return NULL;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    win->texture = SDL_CreateTexture(win->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, hor_res, ver_res);
    if (win->texture == NULL) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(win->renderer);
        SDL_DestroyWindow(win->window);
        free(win->buf);
        return NULL;
    }
    SDL_SetTextureBlendMode(win->texture, SDL_BLENDMODE_NONE);
    win->window_id = SDL_GetWindowID(win->window);

    lv_disp_draw_buf_init(&win->draw_buf, win->buf, NULL, (uint32_t)hor_res * buf_h);
    lv_disp_drv_init(&win->drv);
    win->drv.hor_res = hor_res;
    win->drv.ver_res = ver_res;
    win->drv.flush_cb = flush_cb;
    win->drv.draw_buf = &win->draw_buf;
    win->drv.user_data = win;

    lv_disp_t *disp = lv_disp_drv_register(&win->drv);
    if (disp == NULL) {
        fprintf(stderr, "lv_disp_drv_register failed\n");
        return NULL;
    }

    if (with_pointer) {
        g_pointer_window = win->window_id;
        lv_indev_drv_init(&g_pointer_drv);
        g_pointer_drv.type = LV_INDEV_TYPE_POINTER;
        g_pointer_drv.read_cb = pointer_read;
        g_pointer_drv.disp = disp;
        lv_indev_drv_register(&g_pointer_drv);
    }

    ++g_window_count;
    return disp;
}

void SdlDisplay_HandleEvent(const SDL_Event *event)
{
    Uint32 window_id = 0;
    int x = 0;
    int y = 0;
    bool update_pressed = false;
    bool pressed = false;

    /* Releasing outside the control window still lets the stick spring back. */
    if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT) {
        g_pointer_pressed = false;
    }

    if (event->type == SDL_MOUSEMOTION) {
        window_id = event->motion.windowID;
        x = event->motion.x;
        y = event->motion.y;
    } else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        window_id = event->button.windowID;
        x = event->button.x;
        y = event->button.y;
        update_pressed = true;
        pressed = true;
    } else if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_LEFT) {
        window_id = event->button.windowID;
        x = event->button.x;
        y = event->button.y;
        update_pressed = true;
        pressed = false;
    } else {
        return;
    }

    if (window_id != g_pointer_window) {
        return;
    }
    if (update_pressed) {
        g_pointer_pressed = pressed;
    }

    SdlWindow *win = window_from_id(window_id);
    if (win == NULL || win->zoom < 1) {
        return;
    }

    g_pointer_x = x / win->zoom;
    g_pointer_y = y / win->zoom;
    if (g_pointer_x < 0) {
        g_pointer_x = 0;
    } else if (g_pointer_x >= win->hor) {
        g_pointer_x = win->hor - 1;
    }
    if (g_pointer_y < 0) {
        g_pointer_y = 0;
    } else if (g_pointer_y >= win->ver) {
        g_pointer_y = win->ver - 1;
    }
}

void SdlDisplay_Present(void)
{
    for (int i = 0; i < g_window_count; ++i) {
        SdlWindow *win = &g_windows[i];
        if (!win->dirty) {
            continue;
        }
        SDL_RenderClear(win->renderer);
        SDL_RenderCopy(win->renderer, win->texture, NULL, NULL);
        SDL_RenderPresent(win->renderer);
        win->dirty = false;
    }
}
