#include "lvgl_robot_control.h"
#include "robodrive_comm_client.h"
#include "demos/lv_demos.h"

#include "LVGL_Driver.h"
//#include "SD_SPI.h"

/**********************
*   Extern variable
**********************/

extern lv_style_t style_text_muted;
extern lv_style_t style_title;
extern lv_style_t style_text_small;
extern int g_rssi;

/**********************
*   STATIC variable
**********************/

static lv_obj_t * robot_battery_icon;
static lv_obj_t* robot_battery_bar;
static lv_obj_t * robot_battery_label;

static lv_obj_t* signal_label = NULL;
static lv_obj_t* signal_bar = NULL;
static lv_obj_t* signal_icon = NULL;

static lv_obj_t* rate_label = NULL;
static lv_obj_t* rate_bar = NULL;

LV_IMG_DECLARE(img_signal3);
LV_IMG_DECLARE(img_signal2);
LV_IMG_DECLARE(img_signal1);

LV_IMG_DECLARE(img_battery3);
LV_IMG_DECLARE(img_battery2);
LV_IMG_DECLARE(img_battery1);


void LvglRobotControl_create(lv_obj_t * parent)
{
    ////////////////////////////////////
    // RoboDrive BATTERY
    ////////////////////////////////////
    int robot_battery_level_percent = -1;
    //float battery_voltage = 12.1f;

    robot_battery_icon = lv_img_create(parent);
    lv_img_set_src(robot_battery_icon, &img_battery1);
    lv_obj_align(robot_battery_icon, LV_ALIGN_TOP_LEFT, 5, 5);

    robot_battery_bar = lv_bar_create(parent);
    lv_obj_set_size(robot_battery_bar, 100, 10);
    lv_bar_set_value(robot_battery_bar, robot_battery_level_percent, LV_ANIM_OFF);
    lv_obj_align_to(robot_battery_bar, robot_battery_icon, LV_ALIGN_OUT_RIGHT_TOP, 10, 5);

    robot_battery_label = lv_label_create(parent);
    lv_obj_add_style(robot_battery_label, &style_text_small, 0);
    char strbuf[32];
    //snprintf(strbuf, sizeof(strbuf), "Batt  (%.1fv)  %d%%", battery_voltage, battery_level_percent);
    snprintf(strbuf, sizeof(strbuf), "Robot Batt %d%%", robot_battery_level_percent);
    strbuf[sizeof(strbuf)-1] = '\0';
    lv_label_set_text(robot_battery_label, strbuf);
    lv_obj_align_to(robot_battery_label, robot_battery_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    ////////////////////////////////////
    // SIGNAL
    ////////////////////////////////////
    signal_icon = lv_img_create(parent);
    lv_img_set_src(signal_icon, &img_signal1);
    lv_obj_align_to(signal_icon, robot_battery_icon, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    int signal_strength_percent = 0;
    signal_bar = lv_bar_create(parent);
    lv_obj_set_size(signal_bar, 100, 10);
    lv_bar_set_value(signal_bar, signal_strength_percent, LV_ANIM_OFF);
    lv_obj_align_to(signal_bar, signal_icon, LV_ALIGN_OUT_RIGHT_TOP, 10, 5);

    signal_label = lv_label_create(parent);
    lv_obj_add_style(signal_label, &style_text_small, 0);
    snprintf(strbuf, sizeof(strbuf), "RSSI X dB : X%%");
    strbuf[sizeof(strbuf)-1] = '\0';
    lv_label_set_text(signal_label, strbuf);
    lv_obj_align_to(signal_label, signal_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    ////////////////////////////////////
    // Packet Rate & RTT
    ////////////////////////////////////
    int rate_percent = 0;
    rate_bar = lv_bar_create(parent);
    lv_obj_set_size(rate_bar, 100, 10);
    lv_bar_set_value(rate_bar, rate_percent, LV_ANIM_OFF);
    lv_obj_align_to(rate_bar, signal_icon, LV_ALIGN_OUT_BOTTOM_LEFT, 10+32, 5);

    rate_label = lv_label_create(parent);
    lv_obj_add_style(rate_label, &style_text_small, 0);
    snprintf(strbuf, sizeof(strbuf), "Rate X pkt/s : RTT X ms");
    strbuf[sizeof(strbuf)-1] = '\0';
    lv_label_set_text(rate_label, strbuf);
    lv_obj_align_to(rate_label, rate_bar, LV_ALIGN_OUT_BOTTOM_MID, -32, 0);


}

void LvglRobotControl_update()
{
  char strbuf[32];

  // Robot Battery
  int robot_battery_level_percent = RoboDriveCommClient_GetBatteryPercent();
  snprintf(strbuf, sizeof(strbuf), "Robot Batt %d%%", robot_battery_level_percent);
  strbuf[sizeof(strbuf)-1] = '\0';
  lv_label_set_text(robot_battery_label, strbuf);
  lv_bar_set_value(robot_battery_bar, robot_battery_level_percent, LV_ANIM_OFF);
  const lv_img_dsc_t* img_robot_battery;
  if (robot_battery_level_percent > 66)
  {
    img_robot_battery = &img_battery3;
  }
  else if (robot_battery_level_percent > 33)
  {
    img_robot_battery = &img_battery2;
  }
  else
  {
    img_robot_battery = &img_battery1;
  }
  lv_img_set_src(robot_battery_icon, img_robot_battery);


  // RSS 100%%
  int min_rssi = -100;
  int max_rssi = -20;
  int signal_strength_percent = (100 * (g_rssi - min_rssi)) / (max_rssi - min_rssi);
  if (signal_strength_percent > 100)
  {
    signal_strength_percent = 100;
  }
  else if (signal_strength_percent < 0)
  {
    signal_strength_percent = 0;
  }
  snprintf(strbuf, sizeof(strbuf), "RSSI %ddB : %d%%", g_rssi, signal_strength_percent);
  strbuf[sizeof(strbuf)-1] = '\0';
  lv_label_set_text(signal_label, strbuf);
  lv_bar_set_value(signal_bar, signal_strength_percent, LV_ANIM_OFF);
  const lv_img_dsc_t* img_signal;
  if (signal_strength_percent > 66)
  {
    img_signal = &img_signal3;
  }
  else if (signal_strength_percent > 33)
  {
    img_signal = &img_signal2;
  }
  else
  {
    img_signal = &img_signal1;
  }
  lv_img_set_src(signal_icon, img_signal);


  // Latency and round trip time
  int response_rate = RoboDriveCommClient_GetAndResetResponseCounter() * 10;  // x10 = 100ms update 
  int latency_ms = RoboDriveCommClient_GetLatencyMilliseconds();

  int max_rate = 50;
  int rate_percent = (100.0f * response_rate) / max_rate;

  snprintf(strbuf, sizeof(strbuf), "%dpkt/s : RTT %dms", response_rate, latency_ms);
  strbuf[sizeof(strbuf)-1] = '\0';
  lv_label_set_text(rate_label, strbuf);
  lv_bar_set_value(rate_bar, rate_percent, LV_ANIM_OFF);
}