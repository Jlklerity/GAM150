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

    void ScreenTransition_SetTime(float target_time);
    float EaseOutExpo(float x);
    void ScreenTransition_Init();
    void ScreenTransition_Update(float dt);
}