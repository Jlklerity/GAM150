#include "pch.h"
#include "ScreenTransition.h"

namespace ScreenTransitions
{
    bool  s_is_transitioning = false;
    float s_curr_timer = 0.0f;
    float s_total_timer = 0.6f;
    float result = 0.0f;

    void screen_transition_set_time(float target_time)
    {
        s_total_timer = target_time;
    }

    float ease_out_expo(float x)  // Dropping effect algorithm
    {
        if (x >= 1.0f) return 1.0f;
        return 1.0f - std::pow(1.0f - x, 5.0f);
    }

    void screen_transition_init()  // Trigger the transition
    {
        s_is_transitioning = true;
        s_curr_timer = 0.0f;
    }

    void screen_transition_update(float dt)
    {
        if (s_is_transitioning)
        {
            s_curr_timer += dt;
            result = s_curr_timer / s_total_timer;
            if (result > 1.0f) result = 1.0f;
        }
    }
}