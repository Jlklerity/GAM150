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
    float s_bob_timer = 0.0f;
    float s_scale_timer = 0.0f;
    float s_pop_scale = 1.0f;

    // Floating +1 popup at kill position
    float s_float_timer = 0.0f;
    float s_float_x = 0.0f;
    float s_float_y = 0.0f;
    bool  s_float_active = false;
    static const float FLOAT_DURATION = 0.8f;
    static const float FLOAT_SPEED = 70.0f;

    AEGfxVertexList* s_flame_mesh = nullptr;
    AEGfxTexture* s_flame_texture = nullptr;

    // maps the current streak count to a tier (0-4), tier 0 means no flame is shown,
    // thresholds are 5, 10, 20, 30 - each tier unlocks a bigger and more intensely coloured flame
    int GetTier()
    {
        if (s_streak >= 30) return 4;
        if (s_streak >= 20) return 3;
        if (s_streak >= 10) return 2;
        if (s_streak >= 5)  return 1;
        return 0;
    }

    // outputs an rgb tint colour for the flame sprite based on the current tier,
    // tier 1 is natural fire (no tint), higher tiers shift toward deep orange then blazing red,
    // applied as a multiplicative colour so it blends with the sprite's own colours
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

    // returns a size scale multiplier for the flame sprite based on tier,
    // tier 0 returns 0 (no flame), tier 4 returns 3.0 (three times the base size),
    // this is then further multiplied by s_pop_scale for the hit pop animation
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
    // resets all streak state and loads the flame texture and mesh if not already loaded,
    // safe to call on game restart since it only builds assets if the pointers are null
    void Init()
    {
        s_streak = 0;
        s_bob_timer = 0.0f;
        s_scale_timer = 0.0f;
        s_pop_scale = 1.0f;
        s_float_timer = 0.0f;
        s_float_x = 0.0f;
        s_float_y = 0.0f;
        s_float_active = false;

        if (!s_flame_mesh)
            s_flame_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

        if (!s_flame_texture)
            s_flame_texture = AEGfxTextureLoad("Assets/fire.png");
    }

    // advances the bob animation timer, decays the pop scale back to 1.0 after a kill,
    // and moves the floating streak counter upward while fading it out, param is dt in seconds
    void Update(float dt)
    {
        s_bob_timer += dt * 3.0f;
        if (s_bob_timer > 6.2832f) s_bob_timer -= 6.2832f;

        // Pop scale decays back to 1.0
        if (s_pop_scale > 1.0f)
        {
            s_pop_scale -= dt * 4.0f;
            if (s_pop_scale < 1.0f) s_pop_scale = 1.0f;
        }

        // Floating text moves up and fades
        if (s_float_active)
        {
            s_float_timer += dt;
            s_float_y += FLOAT_SPEED * dt;
            if (s_float_timer >= FLOAT_DURATION)
                s_float_active = false;
        }
    }

    // draws the flame sprite above the player (only if tier > 0) with bob animation, tint and pop scale,
    // also draws the streak count number above the flame, and the floating streak popup at the kill position
    void Draw()
    {
        if (!s_flame_mesh) return;

        int tier = GetTier();

        // Draw fire sprite if streak is active (tier > 0)
        if (tier > 0 && s_flame_texture)
        {
            float size_mul = GetSizeMultiplier(tier) * s_pop_scale;
            float w = FLAME_BASE_W * size_mul;
            float h = FLAME_BASE_H * size_mul;
            float bob = sinf(s_bob_timer) * 4.0f;

            // Bottom-anchor: centre Y = anchor + half height, so flame grows upward
            float flame_centre_y = FLAME_ANCHOR_Y + h * 0.5f + bob;

            float r, g, b;
            GetFlameColor(tier, r, g, b);

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(r, g, b, 1.0f);
            AEGfxTextureSet(s_flame_texture, 0.0f, 0.0f);
            Mesh_Renderer(s_flame_mesh, FLAME_X, flame_centre_y, w, h, 0.0f);

            // Reset tint after drawing
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Streak counter above flame top
            if (Graphics::font_id >= 0 && s_streak > 0)
            {
                std::string streak_str = std::to_string(s_streak) + "x";
                float tw, th;
                AEGfxGetPrintSize(Graphics::font_id, streak_str.c_str(), 0.5f, &tw, &th);

                float win_h = (float)AEGfxGetWindowHeight();
                float ndc_x = (FLAME_X / 800.0f) - tw * 0.5f;
                float ndc_y = ((flame_centre_y + h * 0.5f + 10.0f) / (win_h * 0.5f));

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::font_id, streak_str.c_str(),
                    ndc_x, ndc_y, 0.5f, r, g * 0.8f, b, 1.0f);
            }
        }

        // Floating "+1" popup at kill position
        if (s_float_active && Graphics::font_id >= 0)
        {
            float alpha = 1.0f - (s_float_timer / FLOAT_DURATION);
            float win_w = (float)AEGfxGetWindowWidth();
            float win_h = (float)AEGfxGetWindowHeight();

            std::string pop_str_str = "x" + std::to_string(s_streak);
            const char* pop_str = pop_str_str.c_str();
            float tw, th;
            AEGfxGetPrintSize(Graphics::font_id, pop_str, 0.6f, &tw, &th);

            float ndc_x = (s_float_x / (win_w * 0.5f)) - tw * 0.5f;
            float ndc_y = (s_float_y / (win_h * 0.5f));

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, pop_str,
                ndc_x, ndc_y, 0.6f, 1.0f, 0.9f, 0.2f, alpha);
        }
    }

    // frees the flame mesh and texture and resets the streak counter, call in Game_Exit()
    void Exit()
    {
        if (s_flame_mesh)
        {
            AEGfxMeshFree(s_flame_mesh);
            s_flame_mesh = nullptr;
        }
        if (s_flame_texture)
        {
            AEGfxTextureUnload(s_flame_texture);
            s_flame_texture = nullptr;
        }
        s_streak = 0;
    }

    // increments the streak and triggers the pop scale animation, call when an enemy is killed
    // without a known position - use OnWordCorrectAt if you have the kill position for the float popup
    void OnWordCorrect()
    {
        s_streak++;
        s_pop_scale = 1.6f;
    }

    // increments the streak and triggers the pop scale animation AND spawns a floating streak
    // counter at the given world position, call this version when you have the kill position
    void OnWordCorrectAt(float x, float y)
    {
        s_streak++;
        s_pop_scale = 1.6f;
        s_float_active = true;
        s_float_timer = 0.0f;
        s_float_x = x;
        s_float_y = y + 20.0f;
    }

    // resets the streak to zero and cancels the float popup, call when an enemy lands
    // or the player types a wrong word - anything that should break the combo
    void OnStreakReset()
    {
        s_streak = 0;
        s_pop_scale = 1.0f;
        s_float_active = false;
    }

    // returns the current streak count, used by the tutorial to check if the player hit the target streak
    int GetStreak()
    {
        return s_streak;
    }

    // returns the score multiplier for the current tier (1.0x at tier 0, up to 5.0x at tier 4),
    // apply this to the base score when an enemy is killed to reward high streaks
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

    // resets the streak when the player takes damage, wraps OnStreakReset so callers
    // don't need to know the internal reset function exists
    void OnHealthLost()
    {
        OnStreakReset();
    }
}