#pragma once
#include "Game.h"
#include "AEEngine.h"

AEGfxVertexList* Circle_Mesh(int slices, float radius, float angleStep, u32 color);
AEGfxVertexList* Rect_Mesh(float width, float height, u32 color);
AEGfxVertexList* Sprite_Mesh(const f32 sprite_uv_width, const f32 sprite_uv_height);

