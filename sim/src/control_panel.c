#include "control_panel.h"

#include "hal_link.h"
#include "lvgl_main.h"

/* 9.0 V is empty and 12.6 V is full. The device screen receives percent only. */
#define BATT_TENTH_EMPTY 90
#define BATT_TENTH_FULL 126

static lv_obj_t *g_pad;
static lv_obj_t *g_knob;
static lv_obj_t *g_axis_label;
static lv_obj_t *g_mode_label;
static lv_obj_t *g_rssi_slider;
static lv_obj_t *g_rssi_label;
static lv_obj_t *g_rtt_slider;
static lv_obj_t *g_rtt_label;
static lv_obj_t *g_conn_slider;
static lv_obj_t *g_conn_label;
static lv_obj_t *g_rate_slider;
static lv_obj_t *g_rate_label;
static lv_obj_t *g_batt_slider;
static lv_obj_t *g_batt_label;
static lv_obj_t *g_link;
static lv_obj_t *g_name_label;

static int g_joy_fwd;
static int g_joy_right;
static int g_joy_button = 1;
static bool g_select_mode;

static int battery_percent_from_tenths(int tenths)
{
    if (tenths <= BATT_TENTH_EMPTY) {
        return 0;
    }
    if (tenths >= BATT_TENTH_FULL) {
        return 100;
    }
    return (tenths - BATT_TENTH_EMPTY) * 100 / (BATT_TENTH_FULL - BATT_TENTH_EMPTY);
}

static int battery_tenths_from_percent(int percent)
{
    if (percent < 0) {
        percent = 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    return BATT_TENTH_EMPTY + (BATT_TENTH_FULL - BATT_TENTH_EMPTY) * percent / 100;
}

static void refresh_axis_label(void)
{
    lv_label_set_text_fmt(g_axis_label, "fwd %d    right %d", g_joy_fwd, g_joy_right);
}

static void joy_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSING) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t point;
        lv_area_t coords;
        int radius;
        int dx;
        int dy;

        if (indev == NULL) {
            return;
        }
        lv_indev_get_point(indev, &point);
        lv_obj_get_coords(g_pad, &coords);
        radius = lv_obj_get_width(g_pad) / 2;
        if (radius < 1) {
            radius = 1;
        }
        dx = point.x - (coords.x1 + coords.x2) / 2;
        dy = point.y - (coords.y1 + coords.y2) / 2;
        if (dx > radius) {
            dx = radius;
        } else if (dx < -radius) {
            dx = -radius;
        }
        if (dy > radius) {
            dy = radius;
        } else if (dy < -radius) {
            dy = -radius;
        }
        g_joy_right = dx * 100 / radius;
        g_joy_fwd = -dy * 100 / radius;
        lv_obj_align(g_knob, LV_ALIGN_CENTER, dx, dy);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        g_joy_fwd = 0;
        g_joy_right = 0;
        lv_obj_align(g_knob, LV_ALIGN_CENTER, 0, 0);
    } else {
        return;
    }

    refresh_axis_label();
}

static void click_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_PRESSED) {
        g_joy_button = 0;
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        g_joy_button = 1;
    }
}

static void mode_event_cb(lv_event_t *event)
{
    g_select_mode = lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED);
    LvglMain_setSelectMode(g_select_mode);
    lv_label_set_text(g_mode_label, g_select_mode ? "Slider: Select" : "Slider: Control");
}

static void rssi_event_cb(lv_event_t *event)
{
    int rssi = (int)lv_slider_get_value(lv_event_get_target(event));
    HalLink_SetRssi(rssi);
    lv_label_set_text_fmt(g_rssi_label, "RSSI %d dBm", rssi);
}

static void rtt_event_cb(lv_event_t *event)
{
    int rtt = (int)lv_slider_get_value(lv_event_get_target(event));
    HalLink_SetLatencyMs((uint32_t)rtt);
    lv_label_set_text_fmt(g_rtt_label, "RTT %d ms", rtt);
}

static void conn_event_cb(lv_event_t *event)
{
    int count = (int)lv_slider_get_value(lv_event_get_target(event));
    HalLink_SetConnectionCount((unsigned)count);
    lv_label_set_text_fmt(g_conn_label, "Connections %d", count);
}

static void rate_event_cb(lv_event_t *event)
{
    int shown = (int)lv_slider_get_value(lv_event_get_target(event)) / 10 * 10;
    HalLink_SetPacketRate(shown);
    lv_label_set_text_fmt(g_rate_label, "Rate %d pkt/s", shown);
}

static void apply_battery(void)
{
    int tenths = (int)lv_slider_get_value(g_batt_slider);
    int percent = battery_percent_from_tenths(tenths);
    bool link = lv_obj_has_state(g_link, LV_STATE_CHECKED);

    HalLink_SetBatteryPercent(link ? percent : -1);
    if (link) {
        lv_label_set_text_fmt(g_batt_label, "Battery %d.%d V    %d%%", tenths / 10, tenths % 10, percent);
    } else {
        lv_label_set_text_fmt(g_batt_label, "Battery %d.%d V    no response", tenths / 10, tenths % 10);
    }
}

static void battery_event_cb(lv_event_t *event)
{
    (void)event;
    apply_battery();
}

static void name_timer_cb(lv_timer_t *timer)
{
    const char *name = HalLink_GetRemoteName();
    (void)timer;
    lv_label_set_text_fmt(g_name_label, "Remote %s", name[0] != '\0' ? name : "(none)");
}

static lv_obj_t *make_slider(lv_obj_t *parent, int min, int max, int value, lv_event_cb_t callback)
{
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, lv_pct(100));
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, callback, LV_EVENT_VALUE_CHANGED, NULL);
    return slider;
}

static void stop_scroll(lv_obj_t *obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN);
}

void ControlPanel_Create(lv_disp_t *disp)
{
    lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_obj_t *title;
    lv_obj_t *row;
    lv_obj_t *click;
    lv_obj_t *click_label;
    lv_obj_t *hint;
    int batt_tenths = battery_tenths_from_percent(HAL_LINK_BATT_DEFAULT);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 16, 0);
    lv_obj_set_style_pad_row(scr, 8, 0);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF4F4F4), 0);

    title = lv_label_create(scr);
    lv_label_set_text(title, "Hardware");

    row = lv_obj_create(scr);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 180);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    stop_scroll(row);

    g_pad = lv_obj_create(row);
    lv_obj_set_size(g_pad, 160, 160);
    lv_obj_set_style_radius(g_pad, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g_pad, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_style_border_width(g_pad, 2, 0);
    stop_scroll(g_pad);
    lv_obj_add_event_cb(g_pad, joy_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(g_pad, joy_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(g_pad, joy_event_cb, LV_EVENT_PRESS_LOST, NULL);

    g_knob = lv_obj_create(g_pad);
    lv_obj_set_size(g_knob, 48, 48);
    lv_obj_set_style_radius(g_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(g_knob, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(g_knob, 0, 0);
    lv_obj_align(g_knob, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(g_knob, LV_OBJ_FLAG_CLICKABLE);
    stop_scroll(g_knob);

    click = lv_btn_create(row);
    lv_obj_set_size(click, 88, 48);
    lv_obj_add_event_cb(click, click_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(click, click_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(click, click_event_cb, LV_EVENT_PRESS_LOST, NULL);
    click_label = lv_label_create(click);
    lv_label_set_text(click_label, "Click");
    lv_obj_center(click_label);

    g_axis_label = lv_label_create(scr);
    refresh_axis_label();

    g_mode_label = lv_label_create(scr);
    lv_label_set_text(g_mode_label, "Slider: Control");
    lv_obj_t *mode = lv_switch_create(scr);
    lv_obj_add_event_cb(mode, mode_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    hint = lv_label_create(scr);
    lv_label_set_text(hint, "Stick moves the list only while Select is on.");

    lv_label_set_text(lv_label_create(scr), "Bluetooth");
    g_rssi_label = lv_label_create(scr);
    g_rssi_slider = make_slider(scr, -120, -20, HAL_LINK_RSSI_DEFAULT, rssi_event_cb);
    g_rtt_label = lv_label_create(scr);
    g_rtt_slider = make_slider(scr, 0, 300, (int)HAL_LINK_RTT_DEFAULT, rtt_event_cb);
    g_conn_label = lv_label_create(scr);
    g_conn_slider = make_slider(scr, 0, 10, (int)HAL_LINK_CONN_DEFAULT, conn_event_cb);
    g_rate_label = lv_label_create(scr);
    g_rate_slider = make_slider(scr, 0, 50, HAL_LINK_RATE_DEFAULT, rate_event_cb);

    lv_label_set_text(lv_label_create(scr), "Robot");
    g_batt_label = lv_label_create(scr);
    g_batt_slider = make_slider(scr, BATT_TENTH_EMPTY, BATT_TENTH_FULL, batt_tenths, battery_event_cb);
    g_link = lv_checkbox_create(scr);
    lv_checkbox_set_text(g_link, "Link up");
    lv_obj_add_state(g_link, LV_STATE_CHECKED);
    lv_obj_add_event_cb(g_link, battery_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    g_name_label = lv_label_create(scr);
    name_timer_cb(NULL);
    lv_timer_create(name_timer_cb, 300, NULL);

    lv_event_send(g_rssi_slider, LV_EVENT_VALUE_CHANGED, NULL);
    lv_event_send(g_rtt_slider, LV_EVENT_VALUE_CHANGED, NULL);
    lv_event_send(g_conn_slider, LV_EVENT_VALUE_CHANGED, NULL);
    lv_event_send(g_rate_slider, LV_EVENT_VALUE_CHANGED, NULL);
    apply_battery();
}

bool ControlPanel_IsSelectMode(void)
{
    return g_select_mode;
}

void ControlPanel_GetJoy(int *fwd, int *right, int *button)
{
    *fwd = g_joy_fwd;
    *right = g_joy_right;
    *button = g_joy_button;
}
