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
#include "pch.h"
#include "PowerUp.h"


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
    constexpr float DROP_SPAWN_X_MARGIN = 80.0f;    // keep drops away from screen edges

    // default spawn interval range (seconds between new drops)
    constexpr float DROP_SPAWN_MIN_DEFAULT = 15.0f;
    constexpr float DROP_SPAWN_MAX_DEFAULT = 30.0f;

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
        float fall_speed = 0.0f;
        PowerUpSystem::PowerType type = PowerUpSystem::PowerType::LIGHTNING;
    };

    static Drop s_slots[MAX_SLOTS];

    // ----------------------------------------------------------------
    // Spawn timer
    // ----------------------------------------------------------------
    static float s_spawn_timer = 0.0f;
    static float s_spawn_min = DROP_SPAWN_MIN_DEFAULT;
    static float s_spawn_max = DROP_SPAWN_MAX_DEFAULT;
    static float s_next_spawn_in = 8.0f;

    // ----------------------------------------------------------------
    // Effect timers
    // ----------------------------------------------------------------
    static float s_shield_remaining = 0.0f;
    static float s_slow_remaining = 0.0f;

    // ----------------------------------------------------------------
    // Rendering resources
    // ----------------------------------------------------------------
    static AEGfxVertexList* s_mesh = nullptr;   // unit quad for sprite + fallback

    // One texture per power type (loaded in Init, may be nullptr if asset missing)
    static AEGfxTexture* s_tex_lightning = nullptr;
    static AEGfxTexture* s_tex_shield = nullptr;
    static AEGfxTexture* s_tex_slow = nullptr;

    // Last enemy area passed to Update, lets DebugSpawn use correct bounds
    static Data::EnemyArea s_last_enemy_area{};

    // ----------------------------------------------------------------
    // Lightning strike animation
    // ----------------------------------------------------------------
    constexpr int   STRIKE_FRAME_COUNT = 5;
    constexpr float STRIKE_FRAME_TIME = 0.16f;                        // seconds per frame (~12 fps)
    constexpr float STRIKE_UV_W = 1.0f / STRIKE_FRAME_COUNT;   // 0.2 per frame
    constexpr float STRIKE_DISPLAY_W = 400.0f;
    constexpr float STRIKE_DISPLAY_H = 800.0f;

    struct LightningStrike
    {
        bool  active = false;
        float x = 0.0f;
        float y = 0.0f;
        int   frame = 0;
        float frame_timer = 0.0f;
    };

    static LightningStrike   s_strikes[MAX_SLOTS];
    static AEGfxTexture* s_tex_strike = nullptr;
    static AEGfxVertexList* s_strike_mesh = nullptr;  // single-frame-width UV quad

    // ----------------------------------------------------------------
    // Shield / Slow activation spritesheet animation
    // ----------------------------------------------------------------
    constexpr int   SHIELD_ANIM_FRAME_COUNT = 2;
    constexpr float SHIELD_ANIM_FRAME_TIME = 0.12f;
    constexpr float SHIELD_ANIM_UV_W = 1.0f / SHIELD_ANIM_FRAME_COUNT;  // 0.5 per frame
    constexpr float SHIELD_ANIM_DISPLAY_W = 400.0f;
    constexpr float SHIELD_ANIM_DISPLAY_H = 400.0f;

    struct ScreenEffect
    {
        bool  active = false;
        int   frame = 0;
        float frame_timer = 0.0f;
    };

    static ScreenEffect      s_shield_effect{};
    static ScreenEffect      s_slow_effect{};
    static AEGfxTexture* s_tex_shield_anim = nullptr;   // shieldon.png (2-frame sheet)
    static AEGfxVertexList* s_shield_anim_mesh = nullptr;   // half-width UV quad
    static AEGfxTexture* s_tex_slow_anim = nullptr;   // slowon.png  (2-frame sheet)
    static AEGfxVertexList* s_slow_anim_mesh = nullptr;   // half-width UV quad

    // Running time counter used only for the pulsing text sine wave
    static float s_pulse_timer = 0.0f;

    // Short popup timers: text shows briefly on activation then disappears
    constexpr float POPUP_DURATION = 1.8f;
    constexpr float POPUP_FADE_TIME = 0.5f;
    static float s_shield_popup_timer = 0.0f;
    static float s_slow_popup_timer = 0.0f;

    // Spawn strikes at left, centre, and right of the screen
    static void spawn_three_strikes()
    {
        constexpr float LEFT_X = -500.0f;
        constexpr float CENTRE_X = 0.0f;
        constexpr float RIGHT_X = 500.0f;
        constexpr float STRIKE_Y = 0.0f;  // vertical centre of screen

        s_strikes[0] = { true, LEFT_X,   STRIKE_Y, 0, 0.0f };
        s_strikes[1] = { true, CENTRE_X, STRIKE_Y, 0, 0.0f };
        s_strikes[2] = { true, RIGHT_X,  STRIKE_Y, 0, 0.0f };
    }

    // ----------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------
    static float rand01()
    {
        return (float)std::rand() / (float)RAND_MAX;
    }

    static float rand_range(float a, float b)
    {
        return a + (b - a) * rand01();
    }

    static void world_to_ndc(float wx, float wy, float& nx, float& ny)
    {
        nx = wx / SCREEN_HALF_W;
        ny = wy / SCREEN_HALF_H;
    }

    /**
     * @brief Returns the sprite texture for a power type, or nullptr if not loaded.
     */
    static AEGfxTexture* texture_for_type(PowerUpSystem::PowerType t)
    {
        switch (t)
        {
        case PowerUpSystem::PowerType::LIGHTNING: return s_tex_lightning;
        case PowerUpSystem::PowerType::SHIELD:    return s_tex_shield;
        case PowerUpSystem::PowerType::TIME_SLOW: return s_tex_slow;
        }
        return nullptr;
    }

    /**
     * @brief Returns the fallback colour (r,g,b) for a power type.
     */
    static void color_for_type(PowerUpSystem::PowerType t, float& r, float& g, float& b)
    {
        switch (t)
        {
        case PowerUpSystem::PowerType::LIGHTNING: r = 1.0f; g = 0.85f; b = 0.1f; return; // gold
        case PowerUpSystem::PowerType::SHIELD:    r = 0.2f; g = 1.0f;  b = 0.4f; return; // green
        case PowerUpSystem::PowerType::TIME_SLOW: r = 0.6f; g = 0.2f;  b = 1.0f; return; // purple
        }
        r = g = b = 1.0f;
    }

    /**
     * @brief Picks a random power type.
     */
    static PowerUpSystem::PowerType random_type()
    {
        int v = (int)(rand_range(0.0f, 3.0f));
        if (v < 0) v = 0;
        if (v > 2) v = 2;
        return (PowerUpSystem::PowerType)v;
    }

    /**
     * @brief Tries to spawn into any free slot. Picks a random free slot so
     *        the slot order isn't always predictable.
     */
    static void try_spawn_drop(const Data::EnemyArea& area)
    {
        // Collect free slot indices
        int free_slots[MAX_SLOTS] = {};
        int free_count = 0;
        for (int i = 0; i < MAX_SLOTS; ++i)
            if (!s_slots[i].active) free_slots[free_count++] = i;

        if (free_count == 0) return; // all slots occupied

        // Pick a random free slot
        int chosen = free_slots[(int)(rand_range(0.0f, (float)free_count))];
        if (chosen < 0) chosen = 0;
        if (chosen >= free_count) chosen = free_count - 1;
        int slot = free_slots[chosen];

        float min_x = area.area_min_x + DROP_HALF + DROP_SPAWN_X_MARGIN;
        float max_x = area.area_max_x - DROP_HALF - DROP_SPAWN_X_MARGIN;
        if (min_x > max_x) { min_x = -400.0f; max_x = 400.0f; }

        s_slots[slot].active = true;
        s_slots[slot].type = random_type();
        s_slots[slot].x = rand_range(min_x, max_x);
        s_slots[slot].y = area.area_spawn_y + DROP_SIZE;  // just above spawn line
        s_slots[slot].fall_speed = rand_range(80.0f, 160.0f);
    }

    // ----------------------------------------------------------------
    // Effect activation
    // ----------------------------------------------------------------
    static void activate_lightning(std::vector<Data::Enemy>& pool, Data::Health& health)
    {
        for (Data::Enemy& e : pool)
            if (e.enemy_is_active && !e.enemy_is_dying)
                EnemySystem::enemy_killed(e, health);

        // Play three strike animations (left, centre, right)
        spawn_three_strikes();

        // Strong screen shake for impact
        particles_trigger_shake(0.5f, 20.0f);
    }

    static void activate(PowerUpSystem::PowerType type, std::vector<Data::Enemy>& pool, Data::Health& health)
    {
        switch (type)
        {
        case PowerUpSystem::PowerType::LIGHTNING:
            activate_lightning(pool, health);
            SoundManager::play_lightning_sfx();
            break;
        case PowerUpSystem::PowerType::SHIELD:
            s_shield_remaining = SHIELD_DURATION * UpgradeSystem::upgrade_get_power_up_duration_multiplier();
            s_shield_effect = { true, 0, 0.0f };
            s_shield_popup_timer = POPUP_DURATION;
            SoundManager::play_shield_sfx();
            break;
        case PowerUpSystem::PowerType::TIME_SLOW:
            s_slow_remaining = SLOW_DURATION * UpgradeSystem::upgrade_get_power_up_duration_multiplier();
            s_slow_effect = { true, 0, 0.0f };
            s_slow_popup_timer = POPUP_DURATION;
            SoundManager::play_slow_sfx();
            break;
        }
    }
}


namespace PowerUpSystem
{
    // ----------------------------------------------------------------
    void init()
    {
        std::srand((unsigned)std::time(nullptr));

        // Create unit quad mesh
        if (s_mesh) { AEGfxMeshFree(s_mesh); s_mesh = nullptr; }
        s_mesh = Sprite_Mesh(1.0f, 1.0f);  // full UV (no atlas slicing needed)

        // Load sprite textures, gracefully handle missing assets
        s_tex_lightning = AEGfxTextureLoad("Assets/lightningpowerup.png");
        s_tex_shield = AEGfxTextureLoad("Assets/shieldpowerup.png");
        s_tex_slow = AEGfxTextureLoad("Assets/slowpowerup.png");
        s_tex_strike = AEGfxTextureLoad("Assets/lightningstrike.png");
        s_tex_shield_anim = AEGfxTextureLoad("Assets/shieldon.png");
        s_tex_slow_anim = AEGfxTextureLoad("Assets/slowon.png");

        // Half-width UV quad for the 2-frame shield spritesheet
        if (s_shield_anim_mesh) { AEGfxMeshFree(s_shield_anim_mesh); s_shield_anim_mesh = nullptr; }
        s_shield_anim_mesh = Sprite_Mesh(SHIELD_ANIM_UV_W, 1.0f);

        // Half-width UV quad for the 2-frame slow spritesheet
        if (s_slow_anim_mesh) { AEGfxMeshFree(s_slow_anim_mesh); s_slow_anim_mesh = nullptr; }
        s_slow_anim_mesh = Sprite_Mesh(SHIELD_ANIM_UV_W, 1.0f);

        // Single-frame UV quad: UV width = 1/5 of the sheet, full height
        if (s_strike_mesh) { AEGfxMeshFree(s_strike_mesh); s_strike_mesh = nullptr; }
        s_strike_mesh = Sprite_Mesh(STRIKE_UV_W, 1.0f);

        // Reset slots
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            s_slots[i].active = false;
            s_strikes[i].active = false;
        }

        // Reset timers — always restore the default spawn interval so a restart after
        // a rainbow event does not carry over the near-zero interval it left behind.
        s_spawn_min = DROP_SPAWN_MIN_DEFAULT;
        s_spawn_max = DROP_SPAWN_MAX_DEFAULT;
        s_spawn_timer = 0.0f;
        s_next_spawn_in = rand_range(s_spawn_min, s_spawn_max);
        s_shield_remaining = 0.0f;
        s_slow_remaining = 0.0f;
        s_shield_effect = {};
        s_slow_effect = {};
        s_pulse_timer = 0.0f;
        s_shield_popup_timer = 0.0f;
        s_slow_popup_timer = 0.0f;
    }

    // ----------------------------------------------------------------
    void free()
    {
        if (s_mesh) { AEGfxMeshFree(s_mesh);                  s_mesh = nullptr; }
        if (s_strike_mesh) { AEGfxMeshFree(s_strike_mesh);           s_strike_mesh = nullptr; }
        if (s_shield_anim_mesh) { AEGfxMeshFree(s_shield_anim_mesh);      s_shield_anim_mesh = nullptr; }
        if (s_slow_anim_mesh) { AEGfxMeshFree(s_slow_anim_mesh);        s_slow_anim_mesh = nullptr; }
        if (s_tex_lightning) { AEGfxTextureUnload(s_tex_lightning);     s_tex_lightning = nullptr; }
        if (s_tex_shield) { AEGfxTextureUnload(s_tex_shield);        s_tex_shield = nullptr; }
        if (s_tex_slow) { AEGfxTextureUnload(s_tex_slow);          s_tex_slow = nullptr; }
        if (s_tex_strike) { AEGfxTextureUnload(s_tex_strike);        s_tex_strike = nullptr; }
        if (s_tex_shield_anim) { AEGfxTextureUnload(s_tex_shield_anim);   s_tex_shield_anim = nullptr; }
        if (s_tex_slow_anim) { AEGfxTextureUnload(s_tex_slow_anim);     s_tex_slow_anim = nullptr; }
    }

    // ----------------------------------------------------------------
    void set_drop_spawn_interval(float min_seconds, float max_seconds)
    {
        if (min_seconds < 0.2f)          min_seconds = 0.2f;
        if (max_seconds < min_seconds)   max_seconds = min_seconds;
        s_spawn_min = min_seconds;
        s_spawn_max = max_seconds;
        s_spawn_timer = 0.0f;
        s_next_spawn_in = rand_range(s_spawn_min, s_spawn_max);
    }

    // ----------------------------------------------------------------
    bool is_shield_active()
    {
        return s_shield_remaining > 0.0f;
    }

    // ----------------------------------------------------------------
    float get_time_scale()
    {
        return (s_slow_remaining > 0.0f) ? SLOW_SCALE : 1.0f;
    }

    // ----------------------------------------------------------------
    void update(float dt, std::vector<Data::Enemy>& enemy_pool,
        const Data::EnemyArea& enemy_area, Data::Health& health)
    {
        s_last_enemy_area = enemy_area;  // cache for DebugSpawn

        // ---- Decay effect timers ----
        if (s_shield_remaining > 0.0f) { s_shield_remaining -= dt; if (s_shield_remaining < 0.0f) s_shield_remaining = 0.0f; }
        if (s_slow_remaining > 0.0f) { s_slow_remaining -= dt; if (s_slow_remaining < 0.0f) s_slow_remaining = 0.0f; }
        if (s_shield_popup_timer > 0.0f) { s_shield_popup_timer -= dt; if (s_shield_popup_timer < 0.0f) s_shield_popup_timer = 0.0f; }
        if (s_slow_popup_timer > 0.0f) { s_slow_popup_timer -= dt; if (s_slow_popup_timer < 0.0f) s_slow_popup_timer = 0.0f; }

        // ---- Advance shield / slow activation spritesheet animations ----
        auto advance_effect = [&](ScreenEffect& fx)
            {
                if (!fx.active) return;
                fx.frame_timer += dt;
                if (fx.frame_timer >= SHIELD_ANIM_FRAME_TIME)
                {
                    fx.frame_timer -= SHIELD_ANIM_FRAME_TIME;
                    ++fx.frame;
                    if (fx.frame >= SHIELD_ANIM_FRAME_COUNT)
                    {
                        fx.frame = SHIELD_ANIM_FRAME_COUNT - 1;  // hold last frame
                        fx.active = false;                          // intro done
                    }
                }
            };
        advance_effect(s_shield_effect);
        advance_effect(s_slow_effect);

        // Deactivate held-frame when effect expires
        if (s_shield_remaining <= 0.0f) s_shield_effect.active = false;
        if (s_slow_remaining <= 0.0f) s_slow_effect.active = false;

        // ---- Global pulse timer ----
        s_pulse_timer += dt;

        // ---- Spawn timer ----
        s_spawn_timer += dt;
        if (s_spawn_timer >= s_next_spawn_in)
        {
            s_spawn_timer = 0.0f;
            // Apply upgrade spawn-rate reduction: subtract the accumulated bonus seconds and clamp to 0.5s minimum
            float reduction = UpgradeSystem::upgrade_get_power_up_spawn_interval_reduction();
            float reduced_min = s_spawn_min - reduction;
            float reduced_max = s_spawn_max - reduction;
            if (reduced_min < 0.5f)          reduced_min = 0.5f;          // never let the interval go below 0.5 seconds
            if (reduced_max < reduced_min)   reduced_max = reduced_min;   // keep max >= min after reduction
            s_next_spawn_in = rand_range(reduced_min, reduced_max);
            try_spawn_drop(enemy_area);
        }

        // ---- Advance lightning strike animations ----
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            LightningStrike& s = s_strikes[i];
            if (!s.active) continue;

            s.frame_timer += dt;
            if (s.frame_timer >= STRIKE_FRAME_TIME)
            {
                s.frame_timer -= STRIKE_FRAME_TIME;
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
            Drop& d = s_slots[i];
            if (!d.active) continue;

            // Fall
            d.y -= d.fall_speed * dt;

            // Floor reached, disappear without activating
            if (d.y - DROP_HALF <= enemy_area.area_landing_y)
            {
                d.active = false;
                continue;
            }

            // Number key press, activate and consume the drop
            if (AEInputCheckTriggered(static_cast<u8>(keys[i])))
            {
                d.active = false;
                activate(d.type, enemy_pool, health);
            }
        }
    }

    // ----------------------------------------------------------------
    void on_mouse_click(float /*mouse_x*/, float /*mouse_y*/, std::vector<Data::Enemy>& /*enemy_pool*/)
    {
        // No-op: power-ups are now activated by number keys, not mouse clicks.
    }

    // ----------------------------------------------------------------
    void debug_spawn(PowerType type)
    {
        // Find a free slot
        int slot = -1;
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            if (!s_slots[i].active) { slot = i; break; }
        }
        if (slot < 0) return;  // all slots occupied

        float min_x = s_last_enemy_area.area_min_x + DROP_HALF + DROP_SPAWN_X_MARGIN;
        float max_x = s_last_enemy_area.area_max_x - DROP_HALF - DROP_SPAWN_X_MARGIN;
        if (min_x > max_x) { min_x = -400.0f; max_x = 400.0f; }

        s_slots[slot].active = true;
        s_slots[slot].type = type;
        s_slots[slot].x = rand_range(min_x, max_x);
        s_slots[slot].y = s_last_enemy_area.area_spawn_y + DROP_SIZE;
        s_slots[slot].fall_speed = rand_range(80.0f, 160.0f);
    }

    // ----------------------------------------------------------------
    void draw()
    {
        if (!s_mesh) return;

        // ---- Shield / Slow activation spritesheet + pulsing text ----
        {
            // Helper: draw one frame of a 2-frame spritesheet centred on screen
            auto draw_screen_anim = [&](const ScreenEffect& fx, AEGfxTexture* tex,
                AEGfxVertexList* mesh, float disp_w, float disp_h, float alpha)
                {
                    if (!fx.active || !tex || !mesh) return;
                    float u_offset = fx.frame * SHIELD_ANIM_UV_W;
                    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                    AEGfxSetTransparency(alpha);
                    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                    AEGfxTextureSet(tex, u_offset, 0.0f);
                    mesh_renderer(mesh, 0.0f, 0.0f, disp_w, disp_h, 0.0f);
                    AEGfxSetTransparency(1.0f);
                };

            // Helper: draw pulsing centred text while popup timer is active
            auto draw_pulsing_text = [&](const char* label, float popup_timer, float ndc_y,
                float r, float g, float b)
                {
                    if (popup_timer <= 0.0f || Graphics::g_font_id < 0) return;

                    constexpr float PULSE_SPEED = 4.0f;
                    constexpr float BASE_SCALE = 0.9f;
                    constexpr float PULSE_AMP = 0.2f;
                    float pulse = BASE_SCALE + PULSE_AMP * (0.5f + 0.5f * sinf(s_pulse_timer * PULSE_SPEED));
                    float alpha = (popup_timer < POPUP_FADE_TIME) ? (popup_timer / POPUP_FADE_TIME) : 1.0f;
                    float tw = 0.0f, th = 0.0f;
                    AEGfxGetPrintSize(Graphics::g_font_id, label, pulse, &tw, &th);
                    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                    AEGfxPrint(Graphics::g_font_id, label,
                        -tw * 0.5f, ndc_y - th * 0.5f,
                        pulse, r, g, b, alpha);
                };

            // Stack labels vertically so they never overlap
            float row_h = 0.0f, dummy = 0.0f;
            if (Graphics::g_font_id >= 0)
                AEGfxGetPrintSize(Graphics::g_font_id, "SHIELDED!", 0.9f, &dummy, &row_h);
            constexpr float ROW_GAP = 0.06f;
            float row_spacing = row_h + ROW_GAP;
            int active_count = (s_shield_popup_timer > 0.0f ? 1 : 0)
                + (s_slow_popup_timer > 0.0f ? 1 : 0);
            float cur_y = (active_count - 1) * row_spacing * 0.5f + 0.15f;

            // Shield: shieldon.png spritesheet on activation, then pulsing text
            if (s_shield_effect.active)
                draw_screen_anim(s_shield_effect, s_tex_shield_anim, s_shield_anim_mesh,
                    SHIELD_ANIM_DISPLAY_W, SHIELD_ANIM_DISPLAY_H, 0.85f);
            if (s_shield_popup_timer > 0.0f)
            {
                draw_pulsing_text("SHIELDED!", s_shield_popup_timer, cur_y, 0.4f, 1.0f, 0.6f);
                cur_y -= row_spacing;
            }

            // Slow: slowon.png spritesheet on activation, then pulsing text
            if (s_slow_effect.active)
                draw_screen_anim(s_slow_effect, s_tex_slow_anim, s_slow_anim_mesh,
                    SHIELD_ANIM_DISPLAY_W, SHIELD_ANIM_DISPLAY_H, 0.75f);
            if (s_slow_popup_timer > 0.0f)
                draw_pulsing_text("SLOWED!", s_slow_popup_timer, cur_y, 0.7f, 0.4f, 1.0f);
        }

        // ---- Lightning strike animations ----
        if (s_strike_mesh && s_tex_strike)
        {
            for (int i = 0; i < MAX_SLOTS; ++i)
            {
                const LightningStrike& s = s_strikes[i];
                if (!s.active) continue;

                float u_offset = s.frame * STRIKE_UV_W;

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxTextureSet(s_tex_strike, u_offset, 0.0f);
                mesh_renderer(s_strike_mesh, s.x, s.y, STRIKE_DISPLAY_W, STRIKE_DISPLAY_H, 0.0f);
            }
        }

        // ---- Falling drops ----
        for (int i = 0; i < MAX_SLOTS; ++i)
        {
            const Drop& d = s_slots[i];
            if (!d.active) continue;

            AEGfxTexture* tex = texture_for_type(d.type);

            if (tex)
            {
                // ---- Textured sprite ----
                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxTextureSet(tex, 0.0f, 0.0f);
                mesh_renderer(s_mesh, d.x, d.y, DROP_SIZE, DROP_SIZE, 0.0f);
            }
            else
            {
                // ---- Fallback: coloured quad ----
                float r, g, b;
                color_for_type(d.type, r, g, b);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(r, g, b, 1.0f);
                mesh_renderer(s_mesh, d.x, d.y, DROP_SIZE, DROP_SIZE, 0.0f);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            }

            // ---- Slot number label above the sprite ----
            if (Graphics::g_font_id >= 0)
            {
                char num_buf[4];
                std::snprintf(num_buf, sizeof(num_buf), "%d", i + 1);

                float nx, ny;
                // Offset world Y to the top edge of the sprite, then a small gap above it
                world_to_ndc(d.x, d.y + DROP_HALF + 10.0f, nx, ny);

                float tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(Graphics::g_font_id, num_buf, 0.6f, &tw, &th);

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxPrint(Graphics::g_font_id, num_buf,
                    nx - tw * 0.5f,
                    ny - th * 0.5f,
                    0.6f,
                    1.0f, 1.0f, 1.0f, // white text
                    1.0f);
            }
        }

        // ---- Active effect indicator icons (bottom-left corner) ----
        if (s_shield_remaining > 0.0f || s_slow_remaining > 0.0f)
        {
            float icon_x = -SCREEN_HALF_W + 40.0f;  // anchored to left edge
            float icon_y = -SCREEN_HALF_H + 40.0f;  // anchored to bottom edge
            float icon_size = 48.0f;
            float gap = 56.0f;

            // Helper lambda to draw one active-effect icon with sprite + timer text
            auto draw_icon = [&](AEGfxTexture* tex, float r, float gb, float b, float remaining)
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
                        mesh_renderer(s_mesh, icon_x, icon_y, icon_size, icon_size, 0.0f);
                    }
                    else
                    {
                        // Fallback coloured quad if texture is missing
                        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetTransparency(alpha);
                        AEGfxSetColorToMultiply(r, gb, b, 1.0f);
                        mesh_renderer(s_mesh, icon_x, icon_y, icon_size, icon_size, 0.0f);
                    }

                    // Draw remaining time below the icon
                    if (Graphics::g_font_id >= 0)
                    {
                        char time_buf[8];
                        std::snprintf(time_buf, sizeof(time_buf), "%.1fs", remaining);

                        float nx, ny;
                        world_to_ndc(icon_x, icon_y - icon_size * 0.5f - 8.0f, nx, ny);

                        float tw = 0.0f, th = 0.0f;
                        AEGfxGetPrintSize(Graphics::g_font_id, time_buf, 0.4f, &tw, &th);

                        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                        AEGfxPrint(Graphics::g_font_id, time_buf,
                            nx - tw * 0.5f,
                            ny - th * 0.5f,
                            0.4f,
                            1.0f, 1.0f, 1.0f,
                            alpha);
                    }

                    icon_x += gap;
                };

            if (s_shield_remaining > 0.0f)
                draw_icon(s_tex_shield, 0.2f, 1.0f, 0.4f, s_shield_remaining);

            if (s_slow_remaining > 0.0f)
                draw_icon(s_tex_slow, 0.6f, 0.2f, 1.0f, s_slow_remaining);

            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}