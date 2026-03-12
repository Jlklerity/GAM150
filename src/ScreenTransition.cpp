#include "ScreenTransition.h"

namespace ScreenTransitions {

    bool isTransitioning = false;
    float currTimer = 0.0f;
    float totalTimer = 0.6f;
    float result = 0.0f;

    void ScreenTransition_SetTime(float targetTime) {
        totalTimer = targetTime;
    }

    float EaseOutExpo(float x) //Dropping effect algorithm
    {
        if (x >= 1.0f) {
            return 1.0f;
        }
        return 1.0f - std::pow(1.0f - x, 5.0f);
    }

    void ScreenTransition_Init() { //Triggered the Transition
        isTransitioning = true;
        currTimer = 0.0f;
    }

    void ScreenTransition_Update(float dt) {
        if (isTransitioning) {
            currTimer += dt;
            result = currTimer / totalTimer;
            if (result > 1.0f) {
                result = 1.0f;
            }
        }
    }
   
}