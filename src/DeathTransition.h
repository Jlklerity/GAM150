#pragma once
#include <AEEngine.h>

namespace DeathTransition
{
    // total duration of the full death animation in seconds
    static const float DEATH_DURATION = 4.5f;

    void start();
    void update(float dt);
    void draw(AEGfxVertexList* playerMesh, AEGfxTexture* playerTex,
        float uvOffsetX, float uvOffsetY);
    bool is_active();
    bool is_complete();
    void reset();
}