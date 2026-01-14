#include "utils.h"

void HPBarPlus(float* HPBar, float* HPBarScale) {
    *HPBar += 5.0f;
    *HPBarScale += 1.666666f; //HP bar scale-translation ratio 3:1, (width x scale) / 2  

}
void HPBarMinus(float* HPBar, float* HPBarScale) {
    *HPBar -= 5.0f;
    *HPBarScale -= 1.666666f; //HP bar scale-translation ratio 3:1, (width x scale) / 2  
}

int AreCirclesIntersecting(float player_radius, float AOE_radius,
    float player_X, float player_Y, float AOE_X, float AOE_Y)
{


    float distance = (float)sqrt((player_X - AOE_X) * (player_X - AOE_X) + (player_Y - AOE_Y) * (player_Y - AOE_Y));
    if (distance < player_radius + AOE_radius) {
        return 1;
    }
    return 0;
}
