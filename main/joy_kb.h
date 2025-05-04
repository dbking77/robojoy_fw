#pragma once

typedef enum
{
    JOY_KEY_NONE = 0,
    JOY_KEY_UP = 1,
    JOY_KEY_DOWN = 2,
    JOY_KEY_RIGHT = 3,
    JOY_KEY_LEFT = 4,
    JOY_KEY_PRESS = 5
} JoyKey;

void JoyKb_Init();

JoyKey JoyKb_GetAndClearKey();

void JoyKb_Update(int joy_fwd, int joy_right, int button);