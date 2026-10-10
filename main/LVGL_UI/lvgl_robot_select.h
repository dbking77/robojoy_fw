#pragma once

#include "lvgl.h"

void LvglRobotSelect_create(lv_obj_t * parent, lv_group_t * group);
void LvglRobotSelect_update();
int LvglRobotSelect_GetSelectedIndex(void);
void LvglRobotSelect_SetSelectedIndex(int index);
const char* LvglRobotSelect_GetSelectedName(void);