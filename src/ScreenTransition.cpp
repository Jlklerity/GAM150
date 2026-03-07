#include "ScreenTransition.h"

namespace ScreenTransitions {

    bool isTransitioning = false;
    float currTimer = 0.0f;
    const float TOTALTIMER = 0.7f;
    float result = 0.0f;
    float currPosY = 0.0f;
    float startPosY = 0.0f;
    float targetPosY = 0.0f;

    float EaseOutExpo(float x) //Dropping effect algorithm
    {
        if (x >= 1.0f) {
            return 1.0f;
        }
        return 1.0f - std::pow(2.0f, -10.0f * x);
    }

    void ScreenTransition_Init() { //Triggered the Transition
        isTransitioning = true;
        currTimer = 0.0f;
    }

    void ScreenTransition_Update(float dt) {
        if (isTransitioning) {
            currTimer += dt;
            result = currTimer / TOTALTIMER;
            if (result > TOTALTIMER) {
                result = TOTALTIMER;
            }
        }
    }
   
}