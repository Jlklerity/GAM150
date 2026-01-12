#pragma once
#include <AEEngine.h>

AEGfxVertexList* CircleMesh(int slices, float radius, float angleStep, u32 color);
AEGfxVertexList* RectMesh(float width, float height, u32 color);
void HPBarPlus(float *HPBar, float *HPBarScale);
void HPBarMinus(float *HPBar, float *HPBarScale);
int AreCirclesIntersecting(float player_radius, float AOE_radius, float player_X, float player_Y, float AOE_X, float AOE_Y);