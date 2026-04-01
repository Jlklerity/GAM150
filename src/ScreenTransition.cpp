/*!
@file           ScreenTransition.cpp
@author         Jeremy Lim
@brief          Handles global screen transition logic and mathematical easing functions for UI animations.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#include "pch.h"
#include "ScreenTransition.h"

/*!
@namespace      ScreenTransitions
@brief          Encapsulates variables and functions used to manage screen transition timings and easing animations across different game states.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace ScreenTransitions
{
    bool  s_is_transitioning = false;
    float s_curr_timer = 0.0f;
    float s_total_timer = 0.6f;
    float result = 0.0f;

    /*!
    @function       screen_transition_set_time
    @brief          Overrides the default total duration of the screen transition.
    @param          target_time (float) The new duration for the transition in seconds.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void screen_transition_set_time(float target_time)
    {
        s_total_timer = target_time;
    }

    /*!
    @function       ease_out_expo
    @brief          A mathematical easing function that creates a smooth deceleration (dropping/sliding) effect.
    @param          x (float) The current normalized progress of the transition (from 0.0 to 1.0).
    @return         A float representing the eased progress value.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    float ease_out_expo(float x)  // Dropping effect algorithm
    {
        if (x >= 1.0f) return 1.0f;
        return 1.0f - std::pow(1.0f - x, 5.0f);
    }

    /*!
    @function       screen_transition_init
    @brief          Triggers the start of a screen transition by resetting the current timer and setting the active flag.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void screen_transition_init()  // Trigger the transition
    {
        s_is_transitioning = true;
        s_curr_timer = 0.0f;
    }

    /*!
    @function       screen_transition_update
    @brief          Advances the transition timer by the delta time and calculates the normalized progress result.
    @param          dt (float) The delta time since the last frame.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
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