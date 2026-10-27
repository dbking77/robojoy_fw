#include "lvgl_robot_select.h"

#include "gatt_client.h"
#include "demos/lv_demos.h"

#include "LVGL_Driver.h"

#include <stdint.h>

static const char* const robot_names[] = {
    "BOO_FRIENDS",
    "ROBODRIVE_2025",
    "AUSTIN_OCT2026",
    "BOX_BOX_BOX",
};
#define ROBOT_NAME_COUNT ((int)(sizeof(robot_names) / sizeof(robot_names[0])))

/**********************
*   Extern variable
**********************/

extern lv_style_t style_text_muted;
extern lv_style_t style_title;
extern lv_style_t style_text_small;

/**********************
*   STATIC variable
**********************/

static lv_obj_t* robot_labels[ROBOT_NAME_COUNT];
static lv_obj_t* robot_buttons[ROBOT_NAME_COUNT];
static int g_selected_robot_index;

static void OnRobotFocused(lv_event_t* event)
{
    g_selected_robot_index = (int)(intptr_t)lv_event_get_user_data(event);
    GattClient_SetRemoteName(LvglRobotSelect_GetSelectedName());
}

void LvglRobotSelect_create(lv_obj_t * parent, lv_group_t * group)
{
    lv_obj_t * list1 = lv_list_create(parent);//lv_scr_act());
    lv_gridnav_add(list1, LV_GRIDNAV_CTRL_ROLLOVER);
    lv_obj_set_size(list1, lv_pct(100), lv_pct(90));
    lv_obj_align(list1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(list1, lv_palette_lighten(LV_PALETTE_BLUE, 5), LV_STATE_FOCUSED);
    lv_group_add_obj(group, list1);
    //lv_group_remove_obj(list1);

    for(int index = 0; index < ROBOT_NAME_COUNT; index++) {
        lv_obj_t * item = lv_list_add_btn(list1, NULL, robot_names[index]);
        lv_obj_set_style_text_font(item, &lv_font_montserrat_10, 0);
        robot_buttons[index] = item;
        lv_obj_set_style_bg_opa(item, 0, 0);
        lv_group_remove_obj(item);   /*Not needed, we use the gridnav instead*/
        lv_obj_add_event_cb(item, OnRobotFocused, LV_EVENT_FOCUSED, (void*)(intptr_t)index);
        lv_obj_add_event_cb(item, OnRobotFocused, LV_EVENT_CLICKED, (void*)(intptr_t)index);
    }

    int index = LvglRobotSelect_GetSelectedIndex();
    lv_gridnav_set_focused(list1, robot_buttons[index], LV_ANIM_OFF);
    GattClient_SetRemoteName(LvglRobotSelect_GetSelectedName());
}   

void LvglRobotSelect_create2(lv_obj_t * parent)
{
    lv_obj_set_style_bg_color(parent, lv_palette_lighten(LV_PALETTE_BLUE, 5), LV_STATE_FOCUSED);

    for (int index = 0; index < ROBOT_NAME_COUNT; ++index)
    {
        lv_obj_t* label = lv_label_create(parent);
        lv_obj_add_flag(label, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_group_remove_obj(label);
        //lv_gridnav_add(label, LV_GRIDNAV_CTRL_NONE);
        robot_labels[index] = label;
        lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
        lv_label_set_text(label, robot_names[index]);
        if (index == 0)
        {
            lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 5);
        }
        else
        {
            lv_obj_align_to(label, robot_labels[index-1], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 40);
        }

        lv_obj_t * checkbox = lv_btn_create(parent);
        robot_buttons[index] = checkbox;
        lv_obj_set_size(checkbox, 40, 40);
        lv_obj_add_flag(checkbox, LV_OBJ_FLAG_CHECKABLE);
        lv_group_remove_obj(checkbox);   /*Not needed, we use the gridnav instead*/
        lv_obj_align_to(checkbox, label, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        lv_obj_add_event_cb(checkbox, OnRobotFocused, LV_EVENT_FOCUSED, (void*)(intptr_t)index);
        lv_obj_add_event_cb(checkbox, OnRobotFocused, LV_EVENT_CLICKED, (void*)(intptr_t)index);
    }
    //lv_group_focus_obj(robot_labels[2]);
    g_selected_robot_index = ROBOT_NAME_COUNT - 1;
    GattClient_SetRemoteName(LvglRobotSelect_GetSelectedName());
    lv_gridnav_set_focused(parent, robot_buttons[g_selected_robot_index], LV_ANIM_ON);
}


void LvglRobotSelect_update()
{
}

int LvglRobotSelect_GetSelectedIndex(void)
{
    if (g_selected_robot_index < 0 || g_selected_robot_index >= ROBOT_NAME_COUNT)
    {
        return ROBOT_NAME_COUNT - 1;
    }
    return g_selected_robot_index;
}

void LvglRobotSelect_SetSelectedIndex(int index)
{
    if (index < 0 || index >= ROBOT_NAME_COUNT)
    {
        index = ROBOT_NAME_COUNT - 1;
    }
    g_selected_robot_index = index;
}

const char* LvglRobotSelect_GetSelectedName(void)
{
    return robot_names[LvglRobotSelect_GetSelectedIndex()];
}