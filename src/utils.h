#pragma once
#include <AEEngine.h>
#include <math.h>

 
void HPBarPlus(float *HPBar, float *HPBarScale);
void HPBarMinus(float *HPBar, float *HPBarScale);
int AreCirclesIntersecting(float player_radius, float AOE_radius, float player_X, float player_Y, float AOE_X, float AOE_Y);