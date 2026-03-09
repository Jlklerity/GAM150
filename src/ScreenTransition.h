#pragma once

#include "AEEngine.h"
#include "Enemy.h"
#include "MainMenu.h"
#include <cmath>

namespace ScreenTransitions {

    extern bool isTransitioning;
    extern float currTimer;
    extern const float TOTALTIMER;
    extern float result;
    extern float currPosY;
    extern float startPosY;
    extern float targetPosY;

	float EaseOutExpo(float x);

	void ScreenTransition_Init();

	void ScreenTransition_Update(float dt);

}