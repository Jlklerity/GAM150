#pragma once

#include "AEEngine.h"
#include "Enemy.h"
#include "MainMenu.h"
#include <cmath>

namespace ScreenTransitions {

    extern bool isTransitioning;
    extern float currTimer;
    extern float result;

    void ScreenTransition_SetTime(float targetTime);

	float EaseOutExpo(float x);

	void ScreenTransition_Init();

	void ScreenTransition_Update(float dt);

}