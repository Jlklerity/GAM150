/**
 * @file PowerUp.cpp
 * @author Azfar
 * @brief Power-up system - sky drops activated by number keys 1/2/3.
 *
 * Up to MAX_SLOTS (3) drops fall simultaneously, one per slot.
 * Each slot is tied to a number key: slot 0 = key 1, slot 1 = key 2, slot 2 = key 3.
 * Pressing the matching key while a drop is active in that slot activates its effect.
 * Drops that reach the enemy landing line disappear without activating.
 *
 * Rendering:
 * - Loads one sprite texture per power type (powerup_lightning / shield / slow).
 * - Falls back to a coloured quad if the texture is missing.
 * - Draws the slot number centred on the sprite so the player knows which key to press.
 *
 * Coordinates:
 * - World origin at centre (0,0), screen 1600x900.
 * - Text in normalised [-1,+1] NDC.
 */

#include "PowerUp.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Particles.h"

#include "AEEngine.h"
#include "AEGraphics.h"
#include "AEMath.h"

#include <cstdlib>
#include <ctime>
#include <cstdio>  // snprintf

namespace
{
    // ----------------------------------------------------------------
    // Screen / world constants
    // ----------------------------------------------------------------
    constexpr float SCREEN_HALF_W = 800.0f;
    constexpr float SCREEN_HALF_H = 450.0f;

    // ----------------------------------------------------------------
    // Drop tuning
    // ----------------------------------------------------------------
    constexpr int   MAX_SLOTS = 3;        // 3 simultaneous drops max
    constexpr float DROP_SIZE = 100.0f;   // matches ENEMY_SIZE_PX
    constexpr float DROP_HALF = DROP_SIZE * 0.5f;
    constexpr float DROP_SPAWN_X_MARGIN = 80.0f; // keep drops away from screen edges

    // default spawn interval range (seconds between new drops)
    constexpr float DROP_SPAWN_MIN_DEFAULT = 6.0f;
    constexpr float DROP_SPAWN_MAX_DEFAULT = 12.0f;

    // ----------------------------------------------------------------
    // Effect durations
    // ----------------------------------------------------------------
    constexpr float SHIELD_DURATION = 10.0f;
    constexpr float SLOW_DURATION = 5.0f;
    constexpr float SLOW_SCALE = 0.6f;  // 40% game speed during slow

    // ----------------------------------------------------------------
    // Per-slot drop state
    // ----------------------------------------------------------------
    struct Drop
    {
        bool  active = false;
        float x = 0.0f;
        float y = 0.0f;
        float fallSpeed = 0.0f;
        PowerUpSystem::PowerType type = PowerUpSystem::PowerType::Lightning;
    };

    static Drop  g_slots[MAX_SLOTS];

    // ----------------------------------------------------------------
    // Spawn timer
    // ----------------------------------------------------------------
    static float g_spawnTimer = 0.0f;
    static float g_spawnMin = DROP_SPAWN_MIN_DEFAULT;
    static float g_spawnMax = DROP_SPAWN_MAX_DEFAULT;
    static float g_nextSpawnIn = 8.0f;

    // ----------------------------------------------------------------
    // Effect timers
    // ----------------------------------------------------------------
    static float g_shieldRemaining = 0.0f;
    static float g_slowRemaining = 0.0f;

    // ----------------------------------------------------------------
    // Rendering resources
    // ----------------------------------------------------------------
    static AEGfxVertexList* g_mesh = nullptr;   // unit quad for sprite + fallback

    // One texture per power type (loaded in Init, may be nullptr if asset missing)
    static AEGfxTexture* g_texLightning = nullptr;
    static AEGfxTexture* g_texShield = nullptr;
    static AEGfxTexture* g_texSlow = nullptr;

    // Last enemy area passed to Update � lets DebugSpawn use correct bounds
    static Data::EnemyArea s_lastEnemyArea{};

    // ----------------------------------------------------------------
    // Lightning strike animation
    // ----------------------------------------------------------------
    constexpr int   STRIKE_FRAME_COUNT = 5;
    constexpr float STRIKE_FRAME_TIME = 0.16f;              // seconds per frame (~12 fps)
    constexpr float STRIKE_UV_W = 1.0f / STRIKE_FRAME_COUNT; // 0.2 per frame
    constexpr float STRIKE_DISPLAY_W = 400.0f;
    constexpr float STRIKE_DISPLAY_H = 800.0f;

    struct LightningStrike
    {
        bool  active = false;
        float x = 0.0f;
        float y = 0.0f;
        int   frame = 0;
        float frameTimer = 0.0f;
    };

    static LightningStrike  g_strikes[MAX_SLOTS];
    static AEGfxTexture* g_texStrike = nullptr;
    static AEGfxVertexList* g_strikeMesh = nullptr;  // single-frame-width UV quad

    // Spawn strikes at left, centre, and right of the screen
    static void SpawnThreeStrikes()
    {
        constexpr float LEFT_X = -500.0f;
        constexpr float CENTRE_X = 0.0f;
        constexpr float RIGHT_X = 500.0f;
        constexpr float STRIKE_Y = 0.0f;  // vertical centre of screen

        // We need 3 slots - expand pool if needed by reusing from index 0
        // g_strikes has MAX_SLOTS entries; spawn each directly by index
        g_strikes[0] = { true, LEFT_X,   STRIKE_Y, 0, 0.0f };
        g_strikes[1] = { true, CENTRE_X, STRIKE_Y, 0, 0.0f };
        g_strikes[2] = { true, RIGHT_X,  STRIKE_Y, 0, 0.0f };
    }

    // ----------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------
    static float Rand01()
    {
        return (float)std::rand() / (float)RAND_MAX;
    }

    static float RandRange(float a, float b)
    {
        return a + (b - a) * Rand01();
    }

    static void WorldToNDC(float wx, float wy, float& nx, float& ny)
    {
        nx = wx / SCREEN_HALF_W;
        ny = wy / SCREEN_HALF_H;
    }

    /**
     * @brief Returns the sprite texture for a power type, or nullptr if not loaded.
     */
    static AEGfxTexture* TextureForType(PowerUpSystem::PowerType t)
    {
        switch (t)
        {
        case PowerUpSystem::PowerType::Lightning: return g_texLightning;
        case PowerUpSystem::PowerType::Shield:    return g_texShield;
        case PowerUpSystem::PowerType::TimeSlow:  return g_texSlow;
        }
        return nullptr;
    }

    /**
     * @brief Returns the fallback colour (r,g,b) for a power type.
     */
    static void ColorForType(PowerUpSystem::PowerType t, float& r, float& g, float& b)
    {
        switch (t)
        {
        case PowerUpSystem::PowerType::Lightning: r = 1.0f; g = 0.85f; b = 0.1f; return; // gold
        case PowerUpSystem::PowerType::Shield:    r = 0.2f; g = 1.0f;  b = 0.4f; return; // green
        case PowerUpSystem::PowerType::TimeSlow:  r = 0.6f; g = 0.2f;  b = 1.0f; return; // purple
        }
        r = g = b = 1.0f;
    }

    /**
     * @brief Picks a random power type.
     */
    static PowerUpSystem::PowerType RandomType()
    {
        int v = (int)(RandRange(0.0f, 3.0f));
        if (v < 0) v = 0;
        if (v > 2) v = 2;
        return (PowerUpSystem::PowerType)v;
    }

    /**
     * @brief Tries to spawn into any free slot. Picks a random free slot so
     *        the slot order isn't always predictable.
     */
    static void TrySpawnDrop(const Data::EnemyArea& area)
    {
        // Collect free slot indices
        int free[MAX_SLOTS] = {};
        int freeCount = 0;
        for (int i = 0; i < MAX_SLOTS; ++i)
            if (!g_slots[i].active) free[freeCount++] = i;

        if (freeCount == 0) return; // all slots occupied

        // Pick a random free slot
        int chosen = free[(int)(RandRange(0.0f, (float)freeCount))];
        if (chosen < 0) chosen = 0;
        if (chosen >= freeCount) chosen = freeCount - 1;
        int slot = free[chosen];

        float minX = area.area_min_x + DROP_HALF + DROP_SPAWN_X_MARGIN;
        float maxX = area.area_max_x - DROP_HALF - DROP_SPAWN_X_MARGIN;
        if (minX > maxX) { minX = -400.0f; maxX = 400.0f; }

        g_slots[slot].active = true;
        g_slots[slot].type = RandomType();
        g_slots[slot].x = RandRange(minX, maxX);
        g_slots[slot].y = area.area_spawn_y + DROP_SIZE;  // just above spawn line
        g_slots[slot].fallSpeed = RandRange(80.0f, 160.0f);
    }

    // ----------------------------------------------------------------
    // Effect activation
    // ----------------------------------------------------------------
    static void ActivateLightning(std::vector<Data::Enemy>& pool, Data::Health& health)
    {
        for (Data::Enemy& e : pool)
            if (e.enemy_is_active && !e.enemy_is_dying)
                EnemySystem::Enemy_Killed(e, health);

        // Play three strike animations (left, centre, right)
        SpawnThreeStrikes();

        // Strong screen shake for impact
        Particles_TriggerShake(0.5f, 20.0f);
    }

    static void Activate(PowerUpSystem::PowerType type, std::vector<Data::Enemy>& pool, Data::Health& health)
    {
        switch (type)
        {
        case PowerUpSystem::PowerType::Lightning: ActivateLightning(pool, health); break;
        case PowerUpSystem::PowerType::Shield:    g_shieldRemaining = SHIELD_DURATION; break;
        case PowerUpSystem::PowerType::TimeSlow:  g_slowRemaining = SLOW_DURATION;   break;
        }
    }
}


namespace PowerUpSystem
{
    // ----------------------------------------------------------------
    void Init()
    {
        std::srand((unsigned)std::time(nullptr));

        // Create unit quad mesh
        if (g_mesh) { AEGfxMeshFree(g_mesh); g_mesh = nullptr; }
        g_mesh = Sprite_Mesh(1.0f, 1.0f);  // full UV (no atlas slicing needed)

        // Load sprite textures — gracefully handle missing assets
        g_texLightning = AEGfxTextureLoad("Assets/lightningpowerup.png");
        g_texShield = AEGfxTextureLoad("Assets/shieldpowerup.png");
        g_texSlow = AEGfxTextureLoad("Assets/slowpowerup.png");
        g_texStrike = AEGfxTextureLoad("Assets/lightningstrike.png");

        // Single-frame UV quad: UV width = 1/5 of the sheet, full height
        if (g_strikeMesh) { AEGfxMeshFree(g_strikeMesh); g_strikeMesh = nullptr; }
        g_strikeMesh = Sprite_Mesh(STRIKE_UV_W, 1.0f);

        // Reset slots
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            g_slots[i].active = false;
            g_strikes[i].active = false;
        }

        // Reset timers
        g_spawnTimer = 0.0f;
        g_nextSpawnIn = RandRange(g_spawnMin, g_spawnMax);
        g_shieldRemaining = 0.0f;
        g_slowRemaining = 0.0f;
    }

    // ----------------------------------------------------------------
    void Free()
    {
        if (g_mesh) { AEGfxMeshFree(g_mesh);              g_mesh = nullptr; }
        if (g_strikeMesh) { AEGfxMeshFree(g_strikeMesh);        g_strikeMesh = nullptr; }
        if (g_texLightning) { AEGfxTextureUnload(g_texLightning); g_texLightning = nullptr; }
        if (g_texShield) { AEGfxTextureUnload(g_texShield);    g_texShield = nullptr; }
        if (g_texSlow) { AEGfxTextureUnload(g_texSlow);      g_texSlow = nullptr; }
        if (g_texStrike) { AEGfxTextureUnload(g_texStrike);    g_texStrike = nullptr; }
    }

    // ----------------------------------------------------------------
    void SetDropSpawnInterval(float minSeconds, float maxSeconds)
    {
        if (minSeconds < 0.2f)        minSeconds = 0.2f;
        if (maxSeconds < minSeconds)  maxSeconds = minSeconds;
        g_spawnMin = minSeconds;
        g_spawnMax = maxSeconds;
        g_spawnTimer = 0.0f;
        g_nextSpawnIn = RandRange(g_spawnMin, g_spawnMax);
    }

    // ----------------------------------------------------------------
    bool IsShieldActive()
    {
        return g_shieldRemaining > 0.0f;
    }

    // ----------------------------------------------------------------
    float GetTimeScale()
    {
        return (g_slowRemaining > 0.0f) ? SLOW_SCALE : 1.0f;
    }

    // ----------------------------------------------------------------
    void Update(float dt, std::vector<Data::Enemy>& enemy_pool,
        const Data::EnemyArea& enemy_area, Data::Health& health)
    {
        s_lastEnemyArea = enemy_area;  // cache for DebugSpawn
        // ---- Decay effect timers ----
        if (g_shieldRemaining > 0.0f) { g_shieldRemaining -= dt; if (g_shieldRemaining < 0.0f) g_shieldRemaining = 0.0f; }
        if (g_slowRemaining > 0.0f) { g_slowRemaining -= dt; if (g_slowRemaining < 0.0f) g_slowRemaining = 0.0f; }

        // ---- Spawn timer ----
        g_spawnTimer += dt;
        if (g_spawnTimer >= g_nextSpawnIn)
        {
            g_spawnTimer = 0.0f;
            g_nextSpawnIn = RandRange(g_spawnMin, g_spawnMax);
            TrySpawnDrop(enemy_area);
        }

        // ---- Advance lightning strike animations ----
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            LightningStrike& s = g_strikes[i];
            if (!s.active) continue;

            s.frameTimer += dt;
            if (s.frameTimer >= STRIKE_FRAME_TIME)
            {
                s.frameTimer -= STRIKE_FRAME_TIME;
                ++s.frame;
                if (s.frame >= STRIKE_FRAME_COUNT)
                    s.active = false;  // animation finished
            }
        }

        // ---- Move drops + key input + floor check ----
        // Key codes for 1, 2, 3
        const int keys[MAX_SLOTS] = { AEVK_1, AEVK_2, AEVK_3 };

        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            Drop& d = g_slots[i];
            if (!d.active) continue;

            // Fall
            d.y -= d.fallSpeed * dt;

            // Floor reached � disappear without activating
            if (d.y - DROP_HALF <= enemy_area.area_landing_y)
            {
                d.active = false;
                continue;
            }

            // Number key press � activate and consume the drop
            if (AEInputCheckTriggered(static_cast<u8>(keys[i])))
            {
                d.active = false;
                Activate(d.type, enemy_pool, health);
            }
        }
    }

    // ----------------------------------------------------------------
    void OnMouseClick(float /*mouseX*/, float /*mouseY*/, std::vector<Data::Enemy>& /*enemy_pool*/)
    {
        // No-op: power-ups are now activated by number keys, not mouse clicks.
    }

    // ----------------------------------------------------------------
    void DebugSpawn(PowerType type)
    {
        // Find a free slot
        int slot = -1;
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            if (!g_slots[i].active) { slot = i; break; }
        }
        if (slot < 0) return;  // all slots occupied

        float minX = s_lastEnemyArea.area_min_x + DROP_HALF + DROP_SPAWN_X_MARGIN;
        float maxX = s_lastEnemyArea.area_max_x - DROP_HALF - DROP_SPAWN_X_MARGIN;
        if (minX > maxX) { minX = -400.0f; maxX = 400.0f; }

        g_slots[slot].active = true;
        g_slots[slot].type = type;
        g_slots[slot].x = RandRange(minX, maxX);
        g_slots[slot].y = s_lastEnemyArea.area_spawn_y + DROP_SIZE;
        g_slots[slot].fallSpeed = RandRange(80.0f, 160.0f);
    }

    // ----------------------------------------------------------------
    void Draw()
    {
        if (!g_mesh) return;

        // ---- Lightning strike animations ----
        if (g_strikeMesh && g_texStrike)
        {
            for (int i = 0; i < MAX_SLOTS; ++i)
            {
                const LightningStrike& s = g_strikes[i];
                if (!s.active) continue;

                float uOffset = s.frame * STRIKE_UV_W;

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxTextureSet(g_texStrike, uOffset, 0.0f);
                Mesh_Renderer(g_strikeMesh, s.x, s.y, STRIKE_DISPLAY_W, STRIKE_DISPLAY_H, 0.0f);
            }
        }

        // ---- Falling drops ----
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            const Drop& d = g_slots[i];
            if (!d.active) continue;

            AEGfxTexture* tex = TextureForType(d.type);

            if (tex)
            {
                // ---- Textured sprite ----
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxTextureSet(tex, 0.0f, 0.0f);
                Mesh_Renderer(g_mesh, d.x, d.y, DROP_SIZE, DROP_SIZE, 0.0f);
            }
            else
            {
                // ---- Fallback: coloured quad ----
                float r, g, b;
                ColorForType(d.type, r, g, b);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(r, g, b, 1.0f);
                Mesh_Renderer(g_mesh, d.x, d.y, DROP_SIZE, DROP_SIZE, 0.0f);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            }

            // ---- Slot number label above the sprite ----
            if (Graphics::font_id >= 0)
            {
                char numBuf[4];
                std::snprintf(numBuf, sizeof(numBuf), "%d", i + 1);

                float nx, ny;
                // Offset world Y to the top edge of the sprite, then a small gap above it
                WorldToNDC(d.x, d.y + DROP_HALF + 10.0f, nx, ny);

                float tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(Graphics::font_id, numBuf, 0.6f, &tw, &th);

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxPrint(Graphics::font_id, numBuf,
                    nx - tw * 0.5f,
                    ny - th * 0.5f,
                    0.6f,
                    1.0f, 1.0f, 1.0f, // white text
                    1.0f);
            }
        }

        // ---- Active effect indicator icons (bottom-left corner) ----
        if (g_shieldRemaining > 0.0f || g_slowRemaining > 0.0f)
        {
            float iconX = -SCREEN_HALF_W + 40.0f;  // anchored to left edge
            float iconY = -SCREEN_HALF_H + 40.0f;  // anchored to bottom edge
            float iconSize = 48.0f;
            float gap = 56.0f;

            // Helper lambda to draw one active-effect icon with sprite + timer text
            auto DrawIcon = [&](AEGfxTexture* tex, float r, float gb, float b, float remaining)
                {
                    // Fade out as effect nears expiry
                    float alpha = (remaining < 1.5f) ? (0.4f + 0.6f * (remaining / 1.5f)) : 1.0f;

                    if (tex)
                    {
                        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetTransparency(alpha);
                        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                        AEGfxTextureSet(tex, 0.0f, 0.0f);
                        Mesh_Renderer(g_mesh, iconX, iconY, iconSize, iconSize, 0.0f);
                    }
                    else
                    {
                        // Fallback coloured quad if texture is missing
                        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetTransparency(alpha);
                        AEGfxSetColorToMultiply(r, gb, b, 1.0f);
                        Mesh_Renderer(g_mesh, iconX, iconY, iconSize, iconSize, 0.0f);
                    }

                    // Draw remaining time below the icon
                    if (Graphics::font_id >= 0)
                    {
                        char timeBuf[8];
                        std::snprintf(timeBuf, sizeof(timeBuf), "%.1fs", remaining);

                        float nx, ny;
                        WorldToNDC(iconX, iconY - iconSize * 0.5f - 8.0f, nx, ny);

                        float tw = 0.0f, th = 0.0f;
                        AEGfxGetPrintSize(Graphics::font_id, timeBuf, 0.4f, &tw, &th);

                        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                        AEGfxPrint(Graphics::font_id, timeBuf,
                            nx - tw * 0.5f,
                            ny - th * 0.5f,
                            0.4f,
                            1.0f, 1.0f, 1.0f,
                            alpha);
                    }

                    iconX += gap;
                };

            if (g_shieldRemaining > 0.0f)
                DrawIcon(g_texShield, 0.2f, 1.0f, 0.4f, g_shieldRemaining);

            if (g_slowRemaining > 0.0f)
                DrawIcon(g_texSlow, 0.6f, 0.2f, 1.0f, g_slowRemaining);

            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}