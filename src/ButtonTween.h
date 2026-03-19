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
    float targetScale = ButtonTweenCfg::NORMAL_SCALE;
    bool  wasClicked = false;
};

inline void ButtonTween_Reset(ButtonTween& btn)
{
    btn.scale = ButtonTweenCfg::NORMAL_SCALE;
    btn.targetScale = ButtonTweenCfg::NORMAL_SCALE;
    btn.wasClicked = false;
}

inline void ButtonTween_Update(ButtonTween& btn, bool hovered, bool clicked, float dt)
{
    using namespace ButtonTweenCfg;

    if (clicked && !btn.wasClicked)
    {
        btn.scale = CLICK_SCALE;
        btn.targetScale = hovered ? HOVER_SCALE : NORMAL_SCALE;
        btn.wasClicked = true;
    }
    else
    {
        btn.wasClicked = clicked;
        btn.targetScale = hovered ? HOVER_SCALE : NORMAL_SCALE;
    }

    float t = 1.0f - std::expf(-LERP_SPEED * dt);
    btn.scale = btn.scale + (btn.targetScale - btn.scale) * t;
}