#include "lvgl_robot_select.h"

#include "demos/lv_demos.h"

#include "LVGL_Driver.h"

#define MAX_ROBOT_SELECT 5

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

static lv_obj_t* robot_labels[MAX_ROBOT_SELECT];
static lv_obj_t* robot_buttons[MAX_ROBOT_SELECT];

void LvglRobotSelect_create(lv_obj_t * parent, lv_group_t * group)
{
    lv_obj_t * list1 = lv_list_create(parent);//lv_scr_act());
    lv_gridnav_add(list1, LV_GRIDNAV_CTRL_ROLLOVER);
    lv_obj_set_size(list1, lv_pct(90), lv_pct(90));
    lv_obj_align(list1, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_style_bg_color(list1, lv_palette_lighten(LV_PALETTE_BLUE, 5), LV_STATE_FOCUSED);
    lv_group_add_obj(group, list1);
    //lv_group_remove_obj(list1);

    char buf[32];
    for(int index = 0; index < MAX_ROBOT_SELECT; index++) {
        lv_snprintf(buf, sizeof(buf), "File %d", index + 1);
        lv_obj_t * item = lv_list_add_btn(list1, LV_SYMBOL_FILE, buf);
        robot_buttons[index] = item;
        lv_obj_set_style_bg_opa(item, 0, 0);
        lv_group_remove_obj(item);   /*Not needed, we use the gridnav instead*/
    }

    //lv_gridnav_set_focused(parent, robot_buttons[2], LV_ANIM_ON);
}   

void LvglRobotSelect_create2(lv_obj_t * parent)
{
    lv_obj_set_style_bg_color(parent, lv_palette_lighten(LV_PALETTE_BLUE, 5), LV_STATE_FOCUSED);

    char strbuf[32];
    for (int index = 0; index < MAX_ROBOT_SELECT; ++index)
    {
        lv_obj_t* label = lv_label_create(parent);
        lv_obj_add_flag(label, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_group_remove_obj(label);
        //lv_gridnav_add(label, LV_GRIDNAV_CTRL_NONE);
        robot_labels[index] = label;
        lv_obj_add_style(label, &style_text_small, 0);
        snprintf(strbuf, sizeof(strbuf), "Robot %d", index);
        strbuf[sizeof(strbuf)-1] = '\0';
        lv_label_set_text(label, strbuf);
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
    }
    //lv_group_focus_obj(robot_labels[2]);
    lv_gridnav_set_focused(parent, robot_buttons[2], LV_ANIM_ON);
}


void LvglRobotSelect_update()
{
}