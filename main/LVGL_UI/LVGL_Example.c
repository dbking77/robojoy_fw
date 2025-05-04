#include "LVGL_Example.h"


#include "lvgl_robot_control.h"
#include "lvgl_robot_select.h"
#include "joy_kb.h"

#include "src/extra/others/gridnav/lv_gridnav.h"
#include "src/core/lv_indev.h"

#include "esp_log.h"


/**********************
 *  STATIC PROTOTYPES
 **********************/
//static void Onboard_create(lv_obj_t * parent);

static void ta_event_cb(lv_event_t * e);
void example1_increase_lvgl_tick(lv_timer_t * t);
/**********************
 *  STATIC VARIABLES
 **********************/

lv_style_t style_text_muted;
lv_style_t style_title;
lv_style_t style_text_small;

static lv_obj_t * tv;
static lv_style_t style_icon;
static lv_style_t style_bullet;

static const lv_font_t * font_large;
static const lv_font_t * font_normal;
static lv_timer_t * auto_step_timer;
static lv_timer_t * meter2_timer;

static lv_indev_drv_t g_indev_drv;
static lv_indev_t * g_indev;

void IRAM_ATTR auto_switch(lv_timer_t * t)
{
  uint16_t page = lv_tabview_get_tab_act(tv);

  if (page == 0) {
    lv_tabview_set_act(tv, 1, LV_ANIM_ON);
  } else if (page == 3) {
    lv_tabview_set_act(tv, 2, LV_ANIM_ON);
  }
}

void Lvgl_update(lv_timer_t* timer)
{
  //LvglRobotControl_update();
  //LvglRobotSelect_update();
}

void keyboard_read(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
  JoyKey key = JoyKb_GetAndClearKey();
  if (key != JOY_KEY_NONE)
  {
    data->state = LV_INDEV_STATE_PRESSED;
    switch (key)
    {
      case JOY_KEY_DOWN: data->key = LV_KEY_DOWN; break;
      case JOY_KEY_UP: data->key = LV_KEY_UP; break;
      case JOY_KEY_RIGHT: data->key = LV_KEY_RIGHT; break;
      case JOY_KEY_LEFT: data->key = LV_KEY_LEFT; break;
      case JOY_KEY_PRESS: data->key = LV_KEY_ENTER; break;
      default:
        data->state = LV_INDEV_STATE_RELEASED;
    }
    ESP_LOGE("READ_KEY", "key %d state %d", (int)data->key, (int)data->state);
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void Lvgl_IndevInit()
{
  lv_indev_drv_init(&g_indev_drv);
  g_indev_drv.type = LV_INDEV_TYPE_KEYPAD;
  g_indev_drv.read_cb = keyboard_read;
  g_indev_drv.disp = lv_disp_get_default();
  g_indev = lv_indev_drv_register(&g_indev_drv);
}

void Lvgl_Example1(void)
{
  Lvgl_IndevInit();

  font_large = LV_FONT_DEFAULT;
  font_normal = LV_FONT_DEFAULT;

  lv_coord_t tab_h;
  tab_h = 45;
  #if LV_FONT_MONTSERRAT_18
    font_large     = &lv_font_montserrat_18;
  #else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
  #endif
  #if LV_FONT_MONTSERRAT_12
    font_normal    = &lv_font_montserrat_12;
  #else
    LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
  #endif

  lv_style_init(&style_text_small);
  lv_style_set_text_font(&style_text_small, &lv_font_montserrat_12);

  lv_style_init(&style_text_muted);
  lv_style_set_text_opa(&style_text_muted, LV_OPA_90);

  lv_style_init(&style_title);
  lv_style_set_text_font(&style_title, font_large);

  lv_style_init(&style_icon);
  lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
  lv_style_set_text_font(&style_icon, font_large);

  lv_style_init(&style_bullet);
  lv_style_set_border_width(&style_bullet, 0);
  lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);


  lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
  
  tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, tab_h);
  // lv_group_remove_obj(tv);
  lv_obj_set_style_bg_color(tv, lv_color_hex(0xFFFFFF), 0);

  //lv_obj_t * select_tab = lv_tabview_add_tab(tv, "Select");
  //lv_group_set_default(group);

  //lv_gridnav_set_focused(tv, select_tab, LV_ANIM_ON);

  //lv_obj_t * control_tab = lv_tabview_add_tab(tv, "Control");
  //lv_group_remove_obj(control_tab);
  //lv_obj_t * select_tab = lv_scr_act();

  //lv_group_add_obj(group, select_tab);
  lv_group_t * group = lv_group_create();
  lv_indev_set_group(g_indev, group);
  //lv_group_add_obj(group, select_tab);
  //v_gridnav_add(select_tab, LV_GRIDNAV_CTRL_NONE);
  //LvglRobotSelect_create(select_tab, group);

  //lv_group_t * top_group = lv_group_get_default(); //lv_group_create();  //
  //lv_group_add_obj(top_group, select_tab);

  lv_obj_t * control_tab = lv_tabview_add_tab(tv, "Control");
  LvglRobotControl_create(control_tab);

  // Setup timer to update display
  lv_timer_create(Lvgl_update, 500, NULL);
}



void Lvgl_Example1_close(void)
{
  /*Delete all animation*/
  lv_anim_del(NULL, NULL);

  lv_timer_del(meter2_timer);
  meter2_timer = NULL;

  lv_obj_clean(lv_scr_act());

  lv_style_reset(&style_text_muted);
  lv_style_reset(&style_title);
  lv_style_reset(&style_icon);
  lv_style_reset(&style_bullet);
}


static void ta_event_cb(lv_event_t * e)
{
}





