#include "pch.h"
#include "StreakSystem.h"

// Flame position - bottom edge of the flame anchored here, always above player's head
static const float FLAME_X = -700.0f;
static const float FLAME_ANCHOR_Y = -120.0f;  // bottom of flame, raise this if still too low

// Base flame size at tier 1 - scales up per tier
static const float FLAME_BASE_W = 80.0f;
static const float FLAME_BASE_H = 80.0f;

namespace
{
    int   s_streak = 0;
    float s_bobTimer = 0.0f;
    float s_scaleTimer = 0.0f;
    float s_popScale = 1.0f;

    // Floating +1 popup at kill position
    float s_floatTimer = 0.0f;
    float s_floatX = 0.0f;
    float s_floatY = 0.0f;
    bool  s_floatActive = false;
    static const float FLOAT_DURATION = 0.8f;
    static const float FLOAT_SPEED = 70.0f;

    AEGfxVertexList* s_flameMesh = nullptr;
    AEGfxTexture* s_flameTexture = nullptr;

    // Returns tier 0-4 based on streak
    int GetTier()
    {
        if (s_streak >= 30) return 4;
        if (s_streak >= 20) return 3;
        if (s_streak >= 10) return 2;
        if (s_streak >= 5)  return 1;
        return 0;
    }

    // Tint color applied on top of the sprite per tier (multiplicative)
    // Tier 1 = no tint (natural fire colours), higher tiers shift hue
    void GetFlameColor(int tier, float& r, float& g, float& b)
    {
        switch (tier)
        {
        case 1: r = 1.0f;  g = 1.0f;  b = 1.0f;  break; // natural
        case 2: r = 1.0f;  g = 0.85f; b = 0.3f;  break; // warm orange tint
        case 3: r = 1.0f;  g = 0.5f;  b = 0.1f;  break; // deep orange-red
        case 4: r = 1.0f;  g = 0.15f; b = 0.15f; break; // blazing red
        default: r = 1.0f; g = 1.0f;  b = 1.0f;  break;
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
        default: return 0.0f;
        }
    }
}

namespace StreakSystem
{
    void Init()
    {
        s_streak = 0;
        s_bobTimer = 0.0f;
        s_scaleTimer = 0.0f;
        s_popScale = 1.0f;
        s_floatTimer = 0.0f;
        s_floatX = 0.0f;
        s_floatY = 0.0f;
        s_floatActive = false;

        if (!s_flameMesh)
            s_flameMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

        if (!s_flameTexture)
            s_flameTexture = AEGfxTextureLoad("Assets/fire.png");
    }

    void Update(float dt)
    {
        s_bobTimer += dt * 3.0f;
        if (s_bobTimer > 6.2832f) s_bobTimer -= 6.2832f;

        // Pop scale decays back to 1.0
        if (s_popScale > 1.0f)
        {
            s_popScale -= dt * 4.0f;
            if (s_popScale < 1.0f) s_popScale = 1.0f;
        }

        // Floating text moves up and fades
        if (s_floatActive)
        {
            s_floatTimer += dt;
            s_floatY += FLOAT_SPEED * dt;
            if (s_floatTimer >= FLOAT_DURATION)
                s_floatActive = false;
        }
    }

    void Draw()
    {
        if (!s_flameMesh) return;

        int tier = GetTier();

        // Draw fire sprite if streak is active (tier > 0)
        if (tier > 0 && s_flameTexture)
        {
            float sizeMul = GetSizeMultiplier(tier) * s_popScale;
            float w = FLAME_BASE_W * sizeMul;
            float h = FLAME_BASE_H * sizeMul;
            float bob = sinf(s_bobTimer) * 4.0f;

            // Bottom-anchor: centre Y = anchor + half height, so flame grows upward
            float flameCentreY = FLAME_ANCHOR_Y + h * 0.5f + bob;

            float r, g, b;
            GetFlameColor(tier, r, g, b);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(r, g, b, 1.0f);
            AEGfxTextureSet(s_flameTexture, 0.0f, 0.0f);
            Mesh_Renderer(s_flameMesh, FLAME_X, flameCentreY, w, h, 0.0f);

            // Reset tint after drawing
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Streak counter above flame top
            if (Graphics::font_id >= 0 && s_streak > 0)
            {
                std::string streakStr = std::to_string(s_streak) + "x";
                float tw, th;
                AEGfxGetPrintSize(Graphics::font_id, streakStr.c_str(), 0.5f, &tw, &th);

                float winH = (float)AEGfxGetWindowHeight();
                float ndcX = (FLAME_X / 800.0f) - tw * 0.5f;
                float ndcY = ((flameCentreY + h * 0.5f + 10.0f) / (winH * 0.5f));

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::font_id, streakStr.c_str(),
                    ndcX, ndcY, 0.5f, r, g * 0.8f, b, 1.0f);
            }
        }

        // Floating "+1" popup at kill position
        if (s_floatActive && Graphics::font_id >= 0)
        {
            float alpha = 1.0f - (s_floatTimer / FLOAT_DURATION);
            float winW = (float)AEGfxGetWindowWidth();
            float winH = (float)AEGfxGetWindowHeight();

            std::string popStrStr = "x" + std::to_string(s_streak);
            const char* popStr = popStrStr.c_str();
            float tw, th;
            AEGfxGetPrintSize(Graphics::font_id, popStr, 0.6f, &tw, &th);

            float ndcX = (s_floatX / (winW * 0.5f)) - tw * 0.5f;
            float ndcY = (s_floatY / (winH * 0.5f));

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, popStr,
                ndcX, ndcY, 0.6f, 1.0f, 0.9f, 0.2f, alpha);
        }
    }

    void Exit()
    {
        if (s_flameMesh)
        {
            AEGfxMeshFree(s_flameMesh);
            s_flameMesh = nullptr;
        }
        if (s_flameTexture)
        {
            AEGfxTextureUnload(s_flameTexture);
            s_flameTexture = nullptr;
        }
        s_streak = 0;
    }

    void OnWordCorrect()
    {
        s_streak++;
        s_popScale = 1.6f;
    }

    void OnWordCorrectAt(float x, float y)
    {
        s_streak++;
        s_popScale = 1.6f;
        s_floatActive = true;
        s_floatTimer = 0.0f;
        s_floatX = x;
        s_floatY = y + 20.0f;
    }

    void OnStreakReset()
    {
        s_streak = 0;
        s_popScale = 1.0f;
        s_floatActive = false;
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