/*!
    @file           ButtonTween.h
    @brief          Lightweight per-button scale tweening for hover and click effects.
*//*___________________________________________________________________________*/

#pragma once
#include <cmath>

namespace ButtonTweenCfg
{
    constexpr float HOVER_SCALE = 1.10f;
    constexpr float CLICK_SCALE = 0.92f;
    constexpr float NORMAL_SCALE = 1.00f;
    constexpr float LERP_SPEED = 18.0f;
}

struct ButtonTween
{
    float scale = ButtonTweenCfg::NORMAL_SCALE;
    float target_scale = ButtonTweenCfg::NORMAL_SCALE;
    bool  was_clicked = false;
};

inline void ButtonTween_Reset(ButtonTween& btn)
{
    btn.scale = ButtonTweenCfg::NORMAL_SCALE;
    btn.target_scale = ButtonTweenCfg::NORMAL_SCALE;
    btn.was_clicked = false;
}

inline void ButtonTween_Update(ButtonTween& btn, bool hovered, bool clicked, float dt)
{
    using namespace ButtonTweenCfg;

    if (clicked && !btn.was_clicked)
    {
        btn.scale = CLICK_SCALE;
        btn.target_scale = hovered ? HOVER_SCALE : NORMAL_SCALE;
        btn.was_clicked = true;
    }
    else
    {
        btn.was_clicked = clicked;
        btn.target_scale = hovered ? HOVER_SCALE : NORMAL_SCALE;
    }

    float t = 1.0f - std::expf(-LERP_SPEED * dt);
    btn.scale = btn.scale + (btn.target_scale - btn.scale) * t;
}