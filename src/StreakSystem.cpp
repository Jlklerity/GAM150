

#include "StreakSystem.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "AEEngine.h"
#include <cmath>
#include <string>
#include "Enemy.h"  // for Graphics::font_id

// Flame position - sits just to the right of the player sprite
static const float FLAME_X = -700.0f;
static const float FLAME_Y = -95.0f;

// Base flame size - scales up per tier
static const float FLAME_BASE_W = 30.0f;
static const float FLAME_BASE_H = 40.0f;

namespace
{
    int   s_streak      = 0;
    float s_bobTimer    = 0.0f;  // for gentle up/down bob animation
    float s_scaleTimer  = 0.0f;  // for pop animation on streak increment
    float s_popScale    = 1.0f;  // current pop scale

    AEGfxVertexList* s_flameMesh    = nullptr;
    AEGfxVertexList* s_counterMesh  = nullptr;

    // Returns tier 0-4 based on streak
    int GetTier()
    {
        if (s_streak >= 30) return 4;
        if (s_streak >= 20) return 3;
        if (s_streak >= 10) return 2;
        if (s_streak >= 5)  return 1;
        return 0;
    }

    // Flame color per tier: no flame, yellow, orange, orange-red, red
    void GetFlameColor(int tier, float& r, float& g, float& b)
    {
        switch (tier)
        {
        case 1: r = 1.0f; g = 0.95f; b = 0.1f; break; // yellow
        case 2: r = 1.0f; g = 0.55f; b = 0.0f; break; // orange
        case 3: r = 1.0f; g = 0.25f; b = 0.0f; break; // orange-red
        case 4: r = 1.0f; g = 0.05f; b = 0.0f; break; // deep red
        default: r = 1.0f; g = 1.0f; b = 1.0f; break;
        }
    }

    // Flame size multiplier per tier
    float GetSizeMultiplier(int tier)
    {
        switch (tier)
        {
        case 1: return 1.0f;
        case 2: return 1.5f;
        case 3: return 2.2f;
        case 4: return 3.0f;
        default: return 0.0f; // no flame at tier 0
        }
    }
}

namespace StreakSystem
{
    void Init()
    {
        s_streak     = 0;
        s_bobTimer   = 0.0f;
        s_scaleTimer = 0.0f;
        s_popScale   = 1.0f;

        if (!s_flameMesh)
            s_flameMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    }

    void Update(float dt)
    {
        s_bobTimer += dt * 3.0f; // bob speed
        if (s_bobTimer > 6.2832f) s_bobTimer -= 6.2832f; // wrap 2*PI

        // Pop scale decays back to 1.0 after streak increment
        if (s_popScale > 1.0f)
        {
            s_popScale -= dt * 4.0f;
            if (s_popScale < 1.0f) s_popScale = 1.0f;
        }
    }

    void Draw()
    {
        if (!s_flameMesh) return;

        int tier = GetTier();
        if (tier == 0) return; // no flame below streak 5

        float sizeMul = GetSizeMultiplier(tier) * s_popScale;
        float w = FLAME_BASE_W * sizeMul;
        float h = FLAME_BASE_H * sizeMul;

        // Gentle bob
        float bob = sinf(s_bobTimer) * 4.0f;

        float r, g, b;
        GetFlameColor(tier, r, g, b);

        // -------------------------------------------------------
        // PLACEHOLDER FLAME - replace with sprite draw later
        // Draw a simple tapered rect as flame placeholder
        // -------------------------------------------------------
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.9f);
        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        Mesh_Renderer(s_flameMesh, FLAME_X, FLAME_Y + bob, w, h, 0.0f);

        // Inner brighter core
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.8f, 1.0f);
        Mesh_Renderer(s_flameMesh, FLAME_X, FLAME_Y + bob - h * 0.1f, w * 0.4f, h * 0.5f, 0.0f);
        // -------------------------------------------------------
        // END PLACEHOLDER - sprite goes here when ready
        // -------------------------------------------------------

        // Draw streak counter above the flame
        if (Graphics::font_id >= 0 && s_streak > 0)
        {
            std::string streakStr = std::to_string(s_streak) + "x";
            float tw, th;
            AEGfxGetPrintSize(Graphics::font_id, streakStr.c_str(), 0.5f, &tw, &th);

            float winH = (float)AEGfxGetWindowHeight();
            float ndcX = (FLAME_X / (800.0f)) - tw * 0.5f;
            float ndcY = ((FLAME_Y + bob + h * 0.7f) / (winH * 0.5f));

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, streakStr.c_str(),
                       ndcX, ndcY, 0.5f, r, g * 0.8f, b, 1.0f);
        }
    }

    void Exit()
    {
        if (s_flameMesh)
        {
            AEGfxMeshFree(s_flameMesh);
            s_flameMesh = nullptr;
        }
        s_streak = 0;
    }

    void OnWordCorrect()
    {
        s_streak++;
        s_popScale = 1.4f; // pop animation on increment
    }

    void OnStreakReset()
    {
        s_streak   = 0;
        s_popScale = 1.0f;
    }

    int GetStreak()
    {
        return s_streak;
    }

    float GetMultiplier()
    {
        switch (GetTier())
        {
        case 1: return 1.5f;
        case 2: return 2.0f;
        case 3: return 3.0f;
        case 4: return 5.0f;
        default: return 1.0f;
        }
    }

    void OnHealthLost()
    {
        OnStreakReset();
    }
}
