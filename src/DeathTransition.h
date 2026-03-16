#pragma once


#include <AEEngine.h>

namespace DeathTransition
{
    // Total duration of the full death animation in seconds
    static const float DEATH_DURATION = 4.5f;

    void Start();
    void Update(float dt);
    void Draw(AEGfxVertexList* playerMesh, AEGfxTexture* playerTex,
        float uvOffsetX, float uvOffsetY);
    bool IsActive();
    bool IsComplete();
    void Reset();
}