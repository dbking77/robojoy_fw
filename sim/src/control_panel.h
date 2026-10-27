#pragma once

#include "lvgl.h"

#include <stdbool.h>

void ControlPanel_Create(lv_disp_t *disp);
bool ControlPanel_IsSelectMode(void);
void ControlPanel_GetJoy(int *fwd, int *right, int *button);
