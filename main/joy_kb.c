#include "joy_kb.h"

#define JOY_KB_LOCKOUT 10

#define JOY_UPPER_LIMIT 60
#define JOY_LOWER_LIMIT 40

struct JoyKbState
{
    int lockout;
    JoyKey key;
};

static struct JoyKbState g_joy_kb_state;

void JoyKb_Init()
{
    struct JoyKbState* state = &g_joy_kb_state;
    state->key = JOY_KEY_NONE;
    state->lockout = 0;
}

JoyKey JoyKb_GetAndClearKey()
{
    struct JoyKbState* state = &g_joy_kb_state;
    JoyKey key = state->key;
    state->key = JOY_KEY_NONE;
    return key;
}

int abs(int value)
{
    return value > 0 ? value : -value;
}

void JoyKb_Update(int joy_fwd, int joy_right, int button)
{
    struct JoyKbState* state = &g_joy_kb_state;
    JoyKey next_key = JOY_KEY_NONE;
    
    if (button == 0)
    {
        next_key = JOY_KEY_PRESS;
    }
    else if (joy_fwd > JOY_UPPER_LIMIT)
    {
        next_key = JOY_KEY_UP;
    }
    else if (joy_fwd < -JOY_UPPER_LIMIT)
    {
        next_key = JOY_KEY_DOWN;
    }
    else if (joy_right > JOY_UPPER_LIMIT)
    {
        next_key = JOY_KEY_RIGHT;
    }
    else if (joy_right < -JOY_UPPER_LIMIT)
    {
        next_key = JOY_KEY_LEFT;
    }

    // Going back to center resets lockout
    if (next_key == JOY_KEY_NONE && abs(joy_right) < JOY_LOWER_LIMIT && abs(joy_fwd) < JOY_LOWER_LIMIT)
    {
        state->lockout = 0;
    }

    if (state->lockout > 0)
    {
        --state->lockout;
    }
    else if ((state->key == JOY_KEY_NONE) && (next_key != JOY_KEY_NONE))
    {
        // Lockout is zero, previous key was read, nad next key is pressed
        state->key = next_key;
        state->lockout = JOY_KB_LOCKOUT;
    }
}
