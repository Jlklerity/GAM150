/**
 * @file PowerUp.cpp
 * @author Azfar
 * @brief Power-up system implementation (button + falling drops + effects).
 *
 * Rendering:
 * - Uses a simple 1x1 quad mesh in AE_GFX_RM_COLOR as a placeholder for the button and drop.
 * - Uses Alpha Engine text rendering (AEGfxCreateFont/AEGfxPrint/AEGfxGetPrintSize).
 *
 * Coordinates:
 * - Game world uses origin at center (0,0), screen size 1600x900.
 * - Text rendering uses normalized coordinates in [-1, +1].
 *
 * Effects:
 * - Lightning: kills all active enemies using EnemySystem::Enemy_Killed().
 * - Shield: sets a timer (damage immunity requires later health integration).
 * - TimeSlow: sets a timer and exposes GetTimeScale() for game dt scaling.
 */

#include "PowerUp.h"
#include "Enemy.h"

#include <cstdlib>
#include <ctime>

#include "AEEngine.h"
#include "AEGraphics.h"
#include "AEMath.h"

namespace
{
    // ------------------------------
    // Screen constants (1600 x 900)
    // ------------------------------
    constexpr float SCREEN_HALF_W = 800.0f;
    constexpr float SCREEN_HALF_H = 450.0f;
    constexpr float UI_MARGIN = 80.0f;

    // ------------------------------
    // Button and effect tuning
    // ------------------------------
    constexpr float BUTTON_COOLDOWN_SECONDS = 30.0f;

    constexpr float SHIELD_DURATION = 5.0f;
    constexpr float SLOW_DURATION = 5.0f;
    constexpr float SLOW_SCALE = 0.4f; // 40% speed while slow is active

    // Default drop spawn range (seconds)
    constexpr float DROP_SPAWN_MIN_DEFAULT = 6.0f;
    constexpr float DROP_SPAWN_MAX_DEFAULT = 12.0f;

    // ------------------------------
    // UI button state
    // ------------------------------
    float g_buttonX = -SCREEN_HALF_W + UI_MARGIN; // -720
    float g_buttonY = -SCREEN_HALF_H + UI_MARGIN; // -370
    float g_buttonRadius = 40.0f;
    float g_buttonCooldownRemaining = 0.0f;

    // ------------------------------
    // Falling drop state (single drop MVP)
    // ------------------------------
    bool  g_dropActive = false;
    float g_dropX = 0.0f;
    float g_dropY = 0.0f;
    float g_dropRadius = 28.0f;
    float g_dropFallSpeed = 160.0f;

    float g_dropSpawnTimer = 0.0f;
    float g_dropSpawnMin = DROP_SPAWN_MIN_DEFAULT;
    float g_dropSpawnMax = DROP_SPAWN_MAX_DEFAULT;
    float g_nextDropSpawnIn = 8.0f;

    PowerUpSystem::PowerType g_dropType = PowerUpSystem::PowerType::Lightning;
    PowerUpSystem::PowerType g_buttonType = PowerUpSystem::PowerType::Lightning;

    // ------------------------------
    // Effect timers
    // ------------------------------
    float g_shieldRemaining = 0.0f;
    float g_slowRemaining = 0.0f;

    // ------------------------------
    // Rendering resources
    // ------------------------------
    AEGfxVertexList* g_quadMesh = nullptr;


    // ------------------------------
    // Utility helpers
    // ------------------------------
    float Rand01() { return (float)std::rand() / (float)RAND_MAX; }
    float RandRange(float a, float b) { return a + (b - a) * Rand01(); }

    /**
     * @brief Checks whether a point lies within a circular hit area.
     */
    bool PointInCircle(float px, float py, float cx, float cy, float r)
    {
        float dx = px - cx, dy = py - cy;
        return (dx * dx + dy * dy) <= (r * r);
    }

    /**
     * @brief Converts world coordinates (center origin) to normalized coordinates in [-1,+1].
     * @param wx World X in pixels.
     * @param wy World Y in pixels.
     * @param nx Output normalized X.
     * @param ny Output normalized Y.
     */
    void WorldToNDC(float wx, float wy, float& nx, float& ny)
    {
        nx = wx / SCREEN_HALF_W;
        ny = wy / SCREEN_HALF_H;
    }

    /**
     * @brief Maps a power type to a short label to display on screen.
     */
    const char* PowerTypeToText(PowerUpSystem::PowerType type)
    {
        switch (type)
        {
        case PowerUpSystem::PowerType::Lightning: return "LGT";
        case PowerUpSystem::PowerType::Shield:    return "SHD";
        case PowerUpSystem::PowerType::TimeSlow:  return "SLOW";
        default: return "?";
        }
    }

    /**
     * @brief Creates a 1x1 quad mesh centered at origin (used as a sprite placeholder).
     * The mesh is created once and reused.
     */
    void CreateQuadMeshIfNeeded()
    {
        if (g_quadMesh) return;

        AEGfxMeshStart();

        // 1x1 quad (2 triangles), centered at origin
        AEGfxTriAdd(
            -0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
            0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

        AEGfxTriAdd(
            0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
            0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
            -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);

        g_quadMesh = AEGfxMeshEnd();
    }

    /**
     * @brief Draws a colored quad at a world position, scaled to size.
     */
    void DrawQuadColor(float x, float y, float sx, float sy, float r, float g, float b, float a)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, a);
        AEGfxSetColorToAdd(0, 0, 0, 0);

        AEMtx33 scale, trans, transform;
        AEMtx33Scale(&scale, sx, sy);
        AEMtx33Trans(&trans, x, y);

        // transform = trans * scale
        AEMtx33Concat(&transform, &trans, &scale);

        AEGfxSetTransform(transform.m);
        AEGfxMeshDraw(g_quadMesh, AE_GFX_MDM_TRIANGLES);
    }

    /**
     * @brief Draws centered text at a given world coordinate by converting to normalized coords.
     *
     * Alpha Engine text is rendered in normalized coordinates and anchored at bottom-left.
     * We use AEGfxGetPrintSize to center the label.
     */
    void DrawCenteredLabel(const char* text, float worldX, float worldY, float scale,
        float r, float g, float b, float a)
    {
        float nx, ny;
        WorldToNDC(worldX, worldY, nx, ny);

        float w = 0.0f, h = 0.0f;
        AEGfxGetPrintSize(Graphics::font_id, text, scale, &w, &h);

        AEGfxPrint(Graphics::font_id, text, nx - w * 0.5f, ny - h * 0.5f, scale, r, g, b, a);
    }

    // ------------------------------
    // Activation logic
    // ------------------------------

    /**
     * @brief Lightning effect: kill all active enemies via official enemy pipeline.
     */
    void ActivateLightning(std::vector<Data::Enemy>& enemy_pool)
    {
        for (Data::Enemy& e : enemy_pool)
        {
            if (e.enemy_is_active && !e.enemy_is_dying)
                EnemySystem::Enemy_Killed(e);
        }
    }

    /**
     * @brief Shield effect: enable shield timer (damage immunity requires later health integration).
     */
    void ActivateShield()
    {
        g_shieldRemaining = SHIELD_DURATION;
    }

    /**
     * @brief TimeSlow effect: enable slow timer. Game should scale dt using GetTimeScale().
     */
    void ActivateTimeSlow()
    {
        g_slowRemaining = SLOW_DURATION;
    }

    /**
     * @brief Activates a power-up type.
     */
    void Activate(PowerUpSystem::PowerType type, std::vector<Data::Enemy>& enemy_pool)
    {
        switch (type)
        {
        case PowerUpSystem::PowerType::Lightning: ActivateLightning(enemy_pool); break;
        case PowerUpSystem::PowerType::Shield:    ActivateShield(); break;
        case PowerUpSystem::PowerType::TimeSlow:  ActivateTimeSlow(); break;
        }
    }

    /**
     * @brief Returns a random power-up type (uniform).
     */
    PowerUpSystem::PowerType RandomType()
    {
        int v = (int)(RandRange(0.0f, 3.0f)); // 0..2
        if (v < 0) v = 0;
        if (v > 2) v = 2;
        return (PowerUpSystem::PowerType)v;
    }

    /**
     * @brief Spawns a falling drop slightly above the top of the screen.
     */
    void SpawnDrop()
    {
        g_dropActive = true;
        g_dropType = RandomType();

        float minX = -SCREEN_HALF_W + UI_MARGIN;
        float maxX = SCREEN_HALF_W - UI_MARGIN;
        float topY = SCREEN_HALF_H + 60.0f;

        g_dropX = RandRange(minX, maxX);
        g_dropY = topY;
        g_dropFallSpeed = RandRange(140.0f, 240.0f);
    }
}

namespace PowerUpSystem
{
    void Init()
    {
        std::srand((unsigned)std::time(nullptr));
        CreateQuadMeshIfNeeded();

        // NOTE: Replace this path with your project font file.
        // Example:
        // g_powerupFont = AEGfxCreateFont("Assets/<yourfont>.otf", 72);

        g_buttonCooldownRemaining = 0.0f;

        g_dropActive = false;
        g_dropSpawnTimer = 0.0f;
        g_nextDropSpawnIn = RandRange(g_dropSpawnMin, g_dropSpawnMax);

        g_shieldRemaining = 0.0f;
        g_slowRemaining = 0.0f;

        g_buttonType = PowerType::Lightning;
    }

    void Free()
    {
        if (g_quadMesh)
        {
            AEGfxMeshFree(g_quadMesh);
            g_quadMesh = nullptr;
        }
    }

    void SetDropSpawnInterval(float minSeconds, float maxSeconds)
    {
        if (minSeconds < 0.2f) minSeconds = 0.2f;
        if (maxSeconds < minSeconds) maxSeconds = minSeconds;

        g_dropSpawnMin = minSeconds;
        g_dropSpawnMax = maxSeconds;

        // Restart timer so tuning is immediately noticeable.
        g_dropSpawnTimer = 0.0f;
        g_nextDropSpawnIn = RandRange(g_dropSpawnMin, g_dropSpawnMax);
    }

    bool IsShieldActive()
    {
        return g_shieldRemaining > 0.0f;
    }

    float GetTimeScale()
    {
        return (g_slowRemaining > 0.0f) ? SLOW_SCALE : 1.0f;
    }

    void Update(float dt, std::vector<Data::Enemy>& enemy_pool)
    {
        (void)enemy_pool;

        // Effect timers (real-time)
        if (g_shieldRemaining > 0.0f)
        {
            g_shieldRemaining -= dt;
            if (g_shieldRemaining < 0.0f) g_shieldRemaining = 0.0f;
        }
        if (g_slowRemaining > 0.0f)
        {
            g_slowRemaining -= dt;
            if (g_slowRemaining < 0.0f) g_slowRemaining = 0.0f;
        }

        // Button cooldown (real-time)
        if (g_buttonCooldownRemaining > 0.0f)
        {
            g_buttonCooldownRemaining -= dt;
            if (g_buttonCooldownRemaining < 0.0f)
                g_buttonCooldownRemaining = 0.0f;
        }

        /**
         * Drop movement/spawn can be slowed locally by time-scale.
         * If you already scale dt globally in game.cpp using GetTimeScale(),
         * then you should set scaledDt = dt (to avoid double-slowing drops).
         */
        float scaledDt = dt * GetTimeScale();

        // Random drop spawn timer
        g_dropSpawnTimer += scaledDt;
        if (!g_dropActive && g_dropSpawnTimer >= g_nextDropSpawnIn)
        {
            g_dropSpawnTimer = 0.0f;
            g_nextDropSpawnIn = RandRange(g_dropSpawnMin, g_dropSpawnMax);
            SpawnDrop();
        }

        // Falling movement
        if (g_dropActive)
        {
            g_dropY -= g_dropFallSpeed * scaledDt;

            float bottomKillY = -SCREEN_HALF_H - 60.0f;
            if (g_dropY < bottomKillY)
                g_dropActive = false;
        }
    }

    void OnMouseClick(float mouseX, float mouseY, std::vector<Data::Enemy>& enemy_pool)
    {
        // Click falling drop
        if (g_dropActive && PointInCircle(mouseX, mouseY, g_dropX, g_dropY, g_dropRadius))
        {
            g_dropActive = false;
            Activate(g_dropType, enemy_pool);
            return;
        }

        // Click UI button (cooldown gated)
        if (PointInCircle(mouseX, mouseY, g_buttonX, g_buttonY, g_buttonRadius))
        {
            if (g_buttonCooldownRemaining <= 0.0f)
            {
                Activate(g_buttonType, enemy_pool);
                g_buttonCooldownRemaining = BUTTON_COOLDOWN_SECONDS;
            }
        }
    }

    void Draw()
    {
        CreateQuadMeshIfNeeded();

        // Button visuals: dim while cooling down
        float mul = (g_buttonCooldownRemaining <= 0.0f) ? 1.0f : 0.35f;

        // Color by power type
        float br = 0.2f, bg = 0.8f, bb = 1.0f; // Lightning
        if (g_buttonType == PowerType::Shield) { br = 0.2f; bg = 1.0f; bb = 0.4f; }
        if (g_buttonType == PowerType::TimeSlow) { br = 0.7f; bg = 0.3f; bb = 1.0f; }

        DrawQuadColor(g_buttonX, g_buttonY,
            g_buttonRadius * 2.0f, g_buttonRadius * 2.0f,
            br * mul, bg * mul, bb * mul, 1.0f);

        // Button label (centered)
        DrawCenteredLabel(PowerTypeToText(g_buttonType), g_buttonX, g_buttonY, 0.5f,
            1.f, 1.f, 1.f, 1.f);

        // Drop visuals
        if (g_dropActive)
        {
            float dr = 1.0f, dg = 0.9f, db = 0.2f; // Lightning-ish gold
            if (g_dropType == PowerType::Shield) { dr = 0.2f; dg = 1.0f; db = 0.4f; }
            if (g_dropType == PowerType::TimeSlow) { dr = 0.7f; dg = 0.3f; db = 1.0f; }

            DrawQuadColor(g_dropX, g_dropY,
                g_dropRadius * 2.0f, g_dropRadius * 2.0f,
                dr, dg, db, 1.0f);

            // Drop label (centered)
            DrawCenteredLabel(PowerTypeToText(g_dropType), g_dropX, g_dropY, 0.5f,
                0.f, 0.f, 0.f, 1.f);
        }

        // Optional tiny indicators while effects are active
        if (g_shieldRemaining > 0.0f)
            DrawQuadColor(g_buttonX + 70.0f, g_buttonY, 18.0f, 18.0f, 0.2f, 1.0f, 0.4f, 1.0f);

        if (g_slowRemaining > 0.0f)
            DrawQuadColor(g_buttonX + 95.0f, g_buttonY, 18.0f, 18.0f, 0.7f, 0.3f, 1.0f, 1.0f);
    }
}
