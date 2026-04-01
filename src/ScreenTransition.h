#pragma once

#include "AEEngine.h"
#include "Enemy.h"
#include "MainMenu.h"
#include <cmath>

namespace ScreenTransitions
{
    extern bool  s_is_transitioning;
    extern float s_curr_timer;
    extern float result;

    void screen_transition_set_time(float target_time);
    float ease_out_expo(float x);
    void screen_transition_init();
    void screen_transition_update(float dt);
}