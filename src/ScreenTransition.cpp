#include "pch.h"
#include "ScreenTransition.h"

namespace ScreenTransitions
{
    bool  s_is_transitioning = false;
    float s_curr_timer = 0.0f;
    float s_total_timer = 0.6f;
    float result = 0.0f;

    void ScreenTransition_SetTime(float target_time)
    {
        s_total_timer = target_time;
    }

    float EaseOutExpo(float x)  // Dropping effect algorithm
    {
        if (x >= 1.0f) return 1.0f;
        return 1.0f - std::pow(1.0f - x, 5.0f);
    }

    void ScreenTransition_Init()  // Trigger the transition
    {
        s_is_transitioning = true;
        s_curr_timer = 0.0f;
    }

    void ScreenTransition_Update(float dt)
    {
        if (s_is_transitioning)
        {
            s_curr_timer += dt;
            result = s_curr_timer / s_total_timer;
            if (result > 1.0f) result = 1.0f;
        }
    }
}