/*!
@file           Boss.cpp
@author         (implementer)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

#include "Boss.h"
#include "AEEngine.h"   // Alpha Engine API

/*
@namespace      BossConfig
@brief          Central location for all boss-related tuning values.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossConfig
{
    const float BOSS_SPAWN_INTERVAL_SECONDS = 60.0f;    // seconds between boss waves
    const float BOSS_MOVE_SPEED = 120.0f;   // px/s lateral movement
    const int   BOSS_WORDS_TO_DEFEAT = 30;       // words required to defeat boss
    const int   BOSS_WORD_MIN_LEN = 3;        // min kill-word / minion word length
    const int   BOSS_WORD_MAX_LEN = 8;        // max kill-word / minion word length
    const float BOSS_SPRITE_SCALE = 1.2f;     // 1.2x normal enemy size
    const float BOSS_SPRITE_W_RATIO = 1.5f;     // width > height (landscape)
    const float BOSS_HIT_TINT_DURATION_SECONDS = 0.18f;    // red tint duration per hit
    const float BOSS_FLASH_INTERVAL_MIN_SECONDS = 8.0f;     // min gap between flash attacks
    const float BOSS_FLASH_INTERVAL_MAX_SECONDS = 18.0f;    // max gap between flash attacks
    const float BOSS_FLASH_FADE_DURATION_SECONDS = 3.0f;     // white overlay fade time
    const int   BOSS_MINION_CAP = 10;       // max live minions
    const float BOSS_MINION_SPAWN_INTERVAL_SECONDS = 20.0f;    // 20 second gap between minion waves
    const int   BOSS_MINION_WAVE_SIZE = 5;        // minions spawned immediately per wave
    const float BOSS_CUTSCENE_DURATION_SECONDS = 5.0f;     // spawn cutscene length (longer entrance)
    const float BOSS_POSITION_Y = 120.0f;   // boss y — well inside the visible play area
} // BossConfig namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
All state private to the boss system.  Nothing in this block is visible outside Boss.cpp.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace
{
    // ------------------------------------------------------------------ state machine
    enum class BossState
    {
        INACTIVE,   // boss not yet triggered or already fully dead and cleaned up
        WAITING,    // 60s mark hit; draining the pool before spawning
        CUTSCENE,   // spawned; playing entrance animation; gameplay frozen
        ACTIVE,     // boss moving, minions spawning, kill-word in pool
        DEAD        // defeat sequence running; boss cleared from pool this frame
    };

    BossState   boss_state = BossState::INACTIVE;

    // ------------------------------------------------------------------ timing
    float       boss_cycle_timer_seconds = 0.0f;     // counts up toward BOSS_SPAWN_INTERVAL_SECONDS
    float       boss_cutscene_timer_seconds = 0.0f;     // countdown during cutscene
    float       boss_minion_spawn_timer = 0.0f;     // time since last minion spawned

    // ------------------------------------------------------------------ movement
    float       boss_position_x = 0.0f;     // world x of boss sprite
    float       boss_dir = 1.0f;     // +1 = moving right, -1 = moving left

    // ------------------------------------------------------------------ defeat counter
    int         boss_words_typed = 0;        // successful hit-word submissions

    // ------------------------------------------------------------------ kill-word pool slot
    int         boss_pool_slot = -1;       // index in enemy_pool that holds the kill-word enemy
    std::string boss_current_word;                      // the current kill-word text

    // ------------------------------------------------------------------ targeting lock
    // While minions are alive the kill-word enemy is deactivated (not visible / not targetable).
    // We track whether it is currently injected so we can re-inject on the frame the last minion dies.
    bool        boss_kill_word_in_pool = false;

    // ------------------------------------------------------------------ cutscene
    float       boss_cutscene_y = 0.0f;     // animated Y during entrance cutscene
    float       boss_hit_tint_timer = 0.0f;     // counts down from BOSS_HIT_TINT_DURATION_SECONDS

    // ------------------------------------------------------------------ sprite row
    int         boss_anim_row = 0;        // 0 = alive row, 1 = death row

    // ------------------------------------------------------------------ flash attack
    float       boss_flash_cooldown_timer = 0.0f;     // time until next flash trigger
    float       boss_flash_active_timer = 0.0f;     // counts down while flash overlay is visible

    // ------------------------------------------------------------------ assets
    AEGfxTexture* boss_texture = nullptr;
    AEGfxVertexList* boss_sprite_mesh = nullptr;  // separate mesh at boss UV scale
    AEGfxVertexList* boss_flash_mesh = nullptr;  // unit rect for full-screen overlay

    // ------------------------------------------------------------------ helpers

    /*
    @function       Boss_CountActiveMinions
    @brief          Returns how many pool enemies are active and alive (excluding the boss kill-word slot).
    Counts MINION-type enemies spawned by the boss, plus any stray NORMAL enemies still on screen.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Boss_CountActiveMinions(const std::vector<Data::Enemy>& pool)
    {
        int count = 0;
        for (int i = 0; i < (int)pool.size(); ++i)
        {
            if (!pool[(size_t)i].enemy_is_active) continue;            // skip inactive
            if (boss_pool_slot >= 0 && i == boss_pool_slot) continue;  // skip the kill-word slot itself
            if (pool[(size_t)i].enemy_is_dying) continue;              // dying enemies do not block
            count++;
        }
        return count;
    }

    /*
    @function       Boss_PoolIsClear
    @brief          Returns true when all regular (non-boss) enemies are inactive or in the dying state.
    We wait for this before spawning the boss so the screen is clear for the cutscene.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Boss_PoolIsClear(const std::vector<Data::Enemy>& pool)
    {
        for (int i = 0; i < (int)pool.size(); ++i)
        {
            if (boss_pool_slot >= 0 && i == boss_pool_slot) continue;  // ignore boss kill-word slot
            const Data::Enemy& e = pool[(size_t)i];
            if (e.enemy_is_active && !e.enemy_is_dying) return false;  // a live enemy is still present
        }
        return true;
    }

    /*
    @function       Boss_PickFlashCooldown
    @brief          Returns a random float in [BOSS_FLASH_INTERVAL_MIN, BOSS_FLASH_INTERVAL_MAX].
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Boss_PickFlashCooldown()
    {
        int min_ms = (int)(BossConfig::BOSS_FLASH_INTERVAL_MIN_SECONDS * 100.0f);
        int max_ms = (int)(BossConfig::BOSS_FLASH_INTERVAL_MAX_SECONDS * 100.0f);
        int picked = Utility::Utility_RandInt(min_ms, max_ms);
        return (float)picked * 0.01f;
    }

    /*
    @function       Boss_InjectKillWord
    @brief          Reveals the kill-word by restoring enemy_word_text so Typing.cpp can target it.
    enemy_is_active stays true at all times while the boss owns the slot.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_InjectKillWord(std::vector<Data::Enemy>& pool)
    {
        if (boss_pool_slot < 0 || boss_pool_slot >= (int)pool.size()) return;
        if (boss_kill_word_in_pool) return;                             // already injected

        // Reveal the kill-word by restoring the word text.
        // enemy_is_active stays TRUE at all times — toggling it would let game.cpp
        // recycle this slot the frame it goes inactive, stealing it from the boss.
        pool[(size_t)boss_pool_slot].enemy_word_text = boss_current_word;
        pool[(size_t)boss_pool_slot].enemy_word_length = (int)boss_current_word.size();
        boss_kill_word_in_pool = true;
    }

    /*
    @function       Boss_RemoveKillWord
    @brief          Deactivates the kill-word enemy so Typing.cpp cannot see or target it.
    Does NOT push the slot back into free_slots — we hold it exclusively for the boss.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_RemoveKillWord(std::vector<Data::Enemy>& pool)
    {
        if (boss_pool_slot < 0 || boss_pool_slot >= (int)pool.size()) return;
        if (!boss_kill_word_in_pool) return;                            // already removed

        // Hide the kill-word by clearing the word text.
        // Typing.cpp skips enemies with empty word text, so it becomes untargetable.
        // enemy_is_active stays TRUE — keeping the slot owned by the boss and invisible
        // to game.cpp's slot-recycling logic.
        pool[(size_t)boss_pool_slot].enemy_word_text = "";
        pool[(size_t)boss_pool_slot].enemy_word_length = 0;
        boss_kill_word_in_pool = false;
    }

    /*
    @function       Boss_SwapKillWord
    @brief          Picks a new unique word and updates the kill-word slot — mirrors Enemy_BreakShield.

    Modelled directly on Enemy_BreakShield: swap the word text on the live slot and update the length.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_SwapKillWord(std::vector<Data::Enemy>& pool, const Data::WordBank& bank)
    {
        std::string new_word = EnemySystem::Enemy_PickUniqueWordRandomLength(
            BossConfig::BOSS_WORD_MIN_LEN,
            BossConfig::BOSS_WORD_MAX_LEN,
            bank, pool, true,
            EnemyConfig::MAX_WORD_TRIES);

        if (new_word.empty()) new_word = boss_current_word;             // fallback: keep old word

        boss_current_word = new_word;

        if (boss_kill_word_in_pool && boss_pool_slot >= 0 && boss_pool_slot < (int)pool.size())
        {
            pool[(size_t)boss_pool_slot].enemy_word_text = boss_current_word;
            pool[(size_t)boss_pool_slot].enemy_word_length = (int)boss_current_word.size();
        }
    }

} // anonymous namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      BossSystem
@brief          Public boss API implementation.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossSystem
{
    /*
    @function       Boss_Init
    @params         meshes (Data::Meshes&), bank (const Data::WordBank&)
    @brief          Loads boss texture and mesh; resets all boss state.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Init(Data::Meshes& /*meshes*/, const Data::WordBank& /*bank*/)
    {
        // Load boss texture
        boss_texture = AEGfxTextureLoad("Assets/bossEnemy.png");
        AE_ASSERT_MESG(boss_texture, "Failed to load: Assets/bossEnemy.png");

        // Build boss sprite mesh: 1 row, 6 columns (same animation layout as normal enemies)
        {
            Data::SpriteAnim boss_anim{};
            boss_anim.animation_columns = 6;
            boss_anim.animation_rows = 2;                            // row 0 = alive, row 1 = death
            boss_sprite_mesh = Graphics::Mesh_BuildSpriteQuad(
                boss_anim.Anim_UvWidth(),
                boss_anim.Anim_UvHeight(),
                0xFFFFFFFF);
        }

        // Build full-screen flash overlay mesh (unit rect, tinted at draw time)
        boss_flash_mesh = Graphics::Mesh_BuildUnitRect(0xFFFFFFFF);

        // Reset all state
        boss_state = BossState::INACTIVE;
        boss_cycle_timer_seconds = 0.0f;
        boss_cutscene_timer_seconds = 0.0f;
        boss_minion_spawn_timer = 0.0f;
        boss_position_x = 0.0f;
        boss_dir = 1.0f;
        boss_words_typed = 0;
        boss_pool_slot = -1;
        boss_current_word.clear();
        boss_kill_word_in_pool = false;
        boss_hit_tint_timer = 0.0f;
        boss_flash_cooldown_timer = Boss_PickFlashCooldown();
        boss_flash_active_timer = 0.0f;
        boss_anim_row = 0;
    }

    /*
    @function       Boss_Free
    @params         meshes (Data::Meshes&), free_slots (std::deque<int>&)
    @brief          Unloads boss texture and mesh; returns reserved pool slot if boss was active.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Free(Data::Meshes& /*meshes*/, std::deque<int>& free_slots, std::vector<Data::Enemy>& pool)
    {
        if (boss_sprite_mesh)
        {
            AEGfxMeshFree(boss_sprite_mesh);
            boss_sprite_mesh = nullptr;
        }

        if (boss_flash_mesh)
        {
            AEGfxMeshFree(boss_flash_mesh);
            boss_flash_mesh = nullptr;
        }

        if (boss_texture)
        {
            AEGfxTextureUnload(boss_texture);
            boss_texture = nullptr;
        }

        // Return the reserved kill-word slot so it isn't lost on restart/exit.
        // Mark it inactive first so no system sees an active slot with no owner.
        if (boss_pool_slot >= 0)
        {
            pool[(size_t)boss_pool_slot].enemy_is_active = false;
            free_slots.push_back(boss_pool_slot);
            boss_pool_slot = -1;
        }

        boss_state = BossState::INACTIVE;
        boss_kill_word_in_pool = false;
        boss_current_word.clear();
    }

    /*
    @function       Boss_Update
    @params         pool, free_slots, area, bank, dt, boss_spawning_blocked
    @brief          Full per-frame boss tick.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Update(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots,
        const Data::EnemyArea& area, const Data::WordBank& bank,
        float dt, bool& boss_spawning_blocked)
    {
        // ----------------------------------------------------------------
        // Flash attack timer — runs at all times while boss is alive so it
        // fires independently of game-logic pauses handled by the caller.
        // (The caller already gates this inside !isPaused && !isDead.)
        // ----------------------------------------------------------------
        if (boss_state == BossState::ACTIVE)
        {
            boss_flash_cooldown_timer -= dt;                            // count down to next flash
            if (boss_flash_cooldown_timer <= 0.0f)
            {
                boss_flash_active_timer = BossConfig::BOSS_FLASH_FADE_DURATION_SECONDS; // trigger flash
                boss_flash_cooldown_timer = Boss_PickFlashCooldown();  // reset cooldown
            }
        }

        if (boss_flash_active_timer > 0.0f)
            boss_flash_active_timer -= dt;                              // decay overlay alpha

        // ----------------------------------------------------------------
        // Hit-tint decay
        // ----------------------------------------------------------------
        if (boss_hit_tint_timer > 0.0f)
            boss_hit_tint_timer -= dt;

        // ----------------------------------------------------------------
        // State machine
        // ----------------------------------------------------------------
        switch (boss_state)
        {
            // ----------------------------------------------------------------
        case BossState::INACTIVE:
        {
            boss_cycle_timer_seconds += dt;                             // advance boss-cycle clock

            if (boss_cycle_timer_seconds >= BossConfig::BOSS_SPAWN_INTERVAL_SECONDS)
            {
                // 60-second mark reached — block regular spawning immediately
                boss_spawning_blocked = true;
                boss_state = BossState::WAITING;
            }
            break;
        }

        // ----------------------------------------------------------------
        case BossState::WAITING:
        {
            boss_spawning_blocked = true;                               // hold block while waiting

            if (Boss_PoolIsClear(pool))                                 // wait for all enemies to die/leave
            {
                // --- Reserve a pool slot for the boss kill-word enemy ---
                if (free_slots.empty()) break;                          // no slot yet — keep waiting

                boss_pool_slot = free_slots.front();
                free_slots.pop_front();

                // Pick an initial kill-word
                boss_current_word = EnemySystem::Enemy_PickUniqueWordRandomLength(
                    BossConfig::BOSS_WORD_MIN_LEN,
                    BossConfig::BOSS_WORD_MAX_LEN,
                    bank, pool, true,
                    EnemyConfig::MAX_WORD_TRIES);

                if (boss_current_word.empty()) boss_current_word = "BOSS"; // fallback

                // Create the kill-word enemy in the slot.
                // Set enemy_has_landed = true and enemy_did_damage = true so Enemy_Update
                // never moves it or applies landing damage (per Known Constraints).
                Data::Enemy kill_word_enemy{};
                kill_word_enemy.enemy_is_active = true;  // stays active permanently — game.cpp must not recycle this slot
                kill_word_enemy.enemy_has_landed = true;  // prevent fall logic in Enemy_Update
                kill_word_enemy.enemy_did_damage = true;  // prevent damage on land
                kill_word_enemy.enemy_is_dying = false;
                kill_word_enemy.enemy_type = Data::EnemyType::NORMAL;
                kill_word_enemy.enemy_word_text = "";    // hidden until Boss_InjectKillWord reveals it
                kill_word_enemy.enemy_word_length = 0;
                kill_word_enemy.enemy_word_text_alt.clear();
                kill_word_enemy.enemy_shield_intact = false;
                kill_word_enemy.enemy_fall_speed_units_per_second = 0.0f;
                kill_word_enemy.enemy_lane_id = -1;
                kill_word_enemy.enemy_position_x = 0.0f;
                kill_word_enemy.enemy_position_y = BossConfig::BOSS_POSITION_Y;
                kill_word_enemy.enemy_animation.animation_row_index = 0;
                kill_word_enemy.enemy_animation.animation_frame_index = 0;
                kill_word_enemy.enemy_animation.animation_timer_seconds = 0.0f;
                kill_word_enemy.enemy_animation.animation_uv_x = 0.0f;
                kill_word_enemy.enemy_animation.animation_uv_y = 0.0f;
                kill_word_enemy.enemy_death_row_index = 1;

                pool[(size_t)boss_pool_slot] = kill_word_enemy;
                boss_kill_word_in_pool = false;                         // not yet visible

                // Enter cutscene — boss slides in from above the screen
                boss_cutscene_timer_seconds = BossConfig::BOSS_CUTSCENE_DURATION_SECONDS;
                boss_cutscene_y = 600.0f;                  // start well above the visible area
                boss_position_x = 0.0f;                    // start centred
                boss_dir = 1.0f;
                boss_words_typed = 0;
                boss_minion_spawn_timer = 0.0f;
                boss_hit_tint_timer = 0.0f;
                boss_flash_cooldown_timer = Boss_PickFlashCooldown();
                boss_flash_active_timer = 0.0f;

                boss_state = BossState::CUTSCENE;
            }
            break;
        }

        // ----------------------------------------------------------------
        case BossState::CUTSCENE:
        {
            boss_spawning_blocked = true;                               // freeze regular spawns
            // Gameplay is frozen by the caller (Boss_IsActive() check gates the update block)

            boss_cutscene_timer_seconds -= dt;

            // Slide boss from off-screen top down to BOSS_POSITION_Y over the cutscene duration
            float progress = 1.0f - (boss_cutscene_timer_seconds / BossConfig::BOSS_CUTSCENE_DURATION_SECONDS);
            progress = Utility::Utility_Clamp(progress, 0.0f, 1.0f);
            boss_cutscene_y = Utility::Utility_Lerp(600.0f, BossConfig::BOSS_POSITION_Y, progress);

            if (boss_cutscene_timer_seconds <= 0.0f)
            {
                // Cutscene over — snap to final position and go live
                boss_cutscene_y = BossConfig::BOSS_POSITION_Y;
                Boss_InjectKillWord(pool);                              // make kill-word targetable
                boss_state = BossState::ACTIVE;
            }
            break;
        }

        // ----------------------------------------------------------------
        case BossState::ACTIVE:
        {
            boss_spawning_blocked = true;                               // hold block while boss alive

            // --- Check if the kill-word slot was just submitted by Typing.cpp ---
            // Typing.cpp calls Enemy_Killed on a matched enemy.  When that happens the slot
            // transitions to enemy_is_dying = true.  We detect that here.
            if (boss_pool_slot >= 0 && boss_pool_slot < (int)pool.size())
            {
                Data::Enemy& kw = pool[(size_t)boss_pool_slot];
                if (kw.enemy_is_dying)
                {
                    // A hit was registered.
                    // Cancel the dying state immediately — if Enemy_Update runs on this slot
                    // before Boss_Update (or if update order ever changes), enemy_is_dying=true
                    // would start a death-drop that moves the slot away from BOSS_POSITION_Y.
                    kw.enemy_is_dying = false;
                    kw.enemy_has_landed = true;     // restore the guard that keeps Enemy_Update inert

                    boss_words_typed++;
                    boss_hit_tint_timer = BossConfig::BOSS_HIT_TINT_DURATION_SECONDS;

                    if (boss_words_typed >= BossConfig::BOSS_WORDS_TO_DEFEAT)
                    {
                        // Boss defeated — deactivate slot and return it to the free pool
                        kw.enemy_is_active = false;
                        boss_kill_word_in_pool = false;

                        // Push the slot back to free pool
                        free_slots.push_back(boss_pool_slot);
                        boss_pool_slot = -1;

                        boss_cycle_timer_seconds = 0.0f;               // reset cycle clock for next wave
                        boss_spawning_blocked = false;               // regular spawning can resume
                        boss_anim_row = 1;                   // switch to death row sprite sheet
                        boss_state = BossState::DEAD;
                        break;
                    }
                    else
                    {
                        // Not defeated yet — swap in a new word
                        kw.enemy_word_text = "";   // hide via empty text, not enemy_is_active
                        kw.enemy_word_length = 0;
                        boss_kill_word_in_pool = false;

                        Boss_SwapKillWord(pool, bank);

                        // Re-inject immediately if no minions are alive
                        if (Boss_CountActiveMinions(pool) == 0)
                            Boss_InjectKillWord(pool);
                    }
                }
            }

            // --- Targeting lock: inject / remove kill-word based on minion count ---
            int minion_count = Boss_CountActiveMinions(pool);

            if (minion_count == 0 && !boss_kill_word_in_pool)
                Boss_InjectKillWord(pool);                              // last minion died this frame
            else if (minion_count > 0 && boss_kill_word_in_pool)
                Boss_RemoveKillWord(pool);                              // minions alive — hide kill-word

            // --- Update kill-word enemy position to follow boss ---
            if (boss_pool_slot >= 0 && boss_pool_slot < (int)pool.size())
            {
                pool[(size_t)boss_pool_slot].enemy_position_x = boss_position_x;
                pool[(size_t)boss_pool_slot].enemy_position_y = BossConfig::BOSS_POSITION_Y;
            }

            // --- Boss movement: left/right bounce within area bounds ---
            boss_position_x += boss_dir * BossConfig::BOSS_MOVE_SPEED * dt;

            float boss_half = (EnemyConfig::ENEMY_SIZE_PX * BossConfig::BOSS_SPRITE_SCALE *
                BossConfig::BOSS_SPRITE_W_RATIO) * 0.5f;

            if (boss_position_x + boss_half >= area.area_max_x)
            {
                boss_position_x = area.area_max_x - boss_half;
                boss_dir = -1.0f;
            }
            else if (boss_position_x - boss_half <= area.area_min_x)
            {
                boss_position_x = area.area_min_x + boss_half;
                boss_dir = 1.0f;
            }

            // --- Minion spawning: fire a wave every BOSS_MINION_SPAWN_INTERVAL_SECONDS ---
            if (minion_count < BossConfig::BOSS_MINION_CAP)
            {
                boss_minion_spawn_timer += dt;
                if (boss_minion_spawn_timer >= BossConfig::BOSS_MINION_SPAWN_INTERVAL_SECONDS)
                {
                    boss_minion_spawn_timer = 0.0f;

                    int wave = BossConfig::BOSS_MINION_WAVE_SIZE;
                    int current = Boss_CountActiveMinions(pool);
                    for (int w = 0; w < wave && current < BossConfig::BOSS_MINION_CAP; ++w)
                    {
                        int len = Utility::Utility_RandInt(BossConfig::BOSS_WORD_MIN_LEN, BossConfig::BOSS_WORD_MAX_LEN);
                        int before_spawn = Boss_CountActiveMinions(pool);
                        EnemySystem::Enemy_SpawnFromPool(pool, free_slots, area, bank, len);
                        if (Boss_CountActiveMinions(pool) > before_spawn)
                            ++current;  // only advance cap counter when a real spawn happened
                    }
                }
            }

            break;
        }

        // ----------------------------------------------------------------
        case BossState::DEAD:
        {
            // Brief dead state — one frame to let systems settle, then go INACTIVE
            boss_spawning_blocked = false;
            boss_anim_row = 0;                                      // reset for the next boss wave
            boss_state = BossState::INACTIVE;
            break;
        }

        } // end switch
    }

    /*
    @function       Boss_Draw
    @params         meshes (const Data::Meshes&)
    @brief          Renders the boss sprite, kill-word label, BOSS X / 50 counter, and cutscene overlay.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Draw(const Data::Meshes& meshes)
    {
        if (boss_state != BossState::CUTSCENE &&
            boss_state != BossState::ACTIVE &&
            boss_state != BossState::DEAD)
            return;                                                     // nothing to draw

        if (!boss_texture || !boss_sprite_mesh) return;

        // ----------------------------------------------------------------
        // Boss sprite — drawn first so overlay + banner always appear on top
        // Uses boss_cutscene_y during cutscene, BOSS_POSITION_Y when active/dead
        // ----------------------------------------------------------------
        float draw_y = (boss_state == BossState::CUTSCENE) ? boss_cutscene_y : BossConfig::BOSS_POSITION_Y;

        float sprite_base = EnemyConfig::ENEMY_SIZE_PX * BossConfig::BOSS_SPRITE_SCALE;
        float sprite_w = sprite_base * BossConfig::BOSS_SPRITE_W_RATIO;
        float sprite_h = sprite_base;

        Graphics::Graphics_SetSpriteState();

        // Apply hit tint: lerp from red back to white over the tint duration
        if (boss_hit_tint_timer > 0.0f)
        {
            float t = Utility::Utility_Clamp(boss_hit_tint_timer / BossConfig::BOSS_HIT_TINT_DURATION_SECONDS, 0.0f, 1.0f);
            float g = Utility::Utility_Lerp(1.0f, 0.0f, t);
            float b = Utility::Utility_Lerp(1.0f, 0.0f, t);
            AEGfxSetColorToMultiply(1.0f, g, b, 1.0f);
        }
        else
        {
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        }

        // Row 0 = alive animation, row 1 = death animation (sheet has 2 rows)
        const float uv_row_height = 1.0f / 2.0f;
        float uv_y = (float)boss_anim_row * uv_row_height;

        AEGfxTextureSet(boss_texture, 0.0f, uv_y);
        Graphics::Draw_DrawMeshAt(boss_sprite_mesh,
            boss_position_x, draw_y,
            sprite_w, sprite_h, 0.0f);

        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);              // reset tint

        // ----------------------------------------------------------------
        // Cutscene: slight dark overlay drawn OVER the boss sprite,
        // then "!! BOSS INCOMING !!" drawn last so it is always in front
        // ----------------------------------------------------------------
        if (boss_state == BossState::CUTSCENE)
        {
            // Slight darkening over the whole screen (0.30 alpha — subtle, not oppressive)
            if (meshes.debug_box_mesh)
            {
                float win_w = (float)AEGfxGetWindowWidth();
                float win_h = (float)AEGfxGetWindowHeight();
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(0.30f);
                AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
                Graphics::Draw_DrawMeshAt(meshes.debug_box_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxSetTransparency(1.0f);
            }

            // "!! BOSS INCOMING !!" banner — drawn last, guaranteed in front of boss sprite
            if (Graphics::font_id >= 0)
            {
                const char* banner = "!! BOSS INCOMING !!";
                float tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(Graphics::font_id, banner, 0.9f, &tw, &th);
                float bx = -(tw * 0.5f);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::font_id, banner, bx, 0.25f, 0.9f, 1.0f, 0.1f, 0.1f, 1.0f);
            }
        }

        // ----------------------------------------------------------------
        // Draw kill-word label (only when injected and boss is ACTIVE)
        // ----------------------------------------------------------------
        if (boss_state == BossState::ACTIVE && boss_kill_word_in_pool && !boss_current_word.empty())
        {
            if (Graphics::font_id >= 0)
            {
                // Anchor the label to a fixed world-space point directly above the boss sprite.
                // sprite_half is the world-space half-height of the boss; add a constant padding
                // so the word sits the same distance above the sprite regardless of word length.
                float sprite_half = sprite_h * 0.5f;
                float label_world_y = BossConfig::BOSS_POSITION_Y + sprite_half + 20.0f;   // 20px above sprite top

                float ndc_x = 0.0f, ndc_y = 0.0f;
                Utility::Convert_WorldToNdc(boss_position_x, label_world_y, ndc_x, ndc_y);

                float text_w = 0.0f, text_h = 0.0f;
                AEGfxGetPrintSize(Graphics::font_id, boss_current_word.c_str(),
                    EnemyConfig::TEXT_SCALE, &text_w, &text_h);

                float draw_x = ndc_x - (text_w * 0.5f);           // horizontally centred on boss X
                float draw_label_y = ndc_y;                             // fixed world-space Y, no text_h scaling

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::font_id, boss_current_word.c_str(),
                    draw_x, draw_label_y,
                    EnemyConfig::TEXT_SCALE,
                    0.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        // ----------------------------------------------------------------
        // BOSS X / 50 progress counter (visible while boss is alive)
        // ----------------------------------------------------------------
        if (boss_state == BossState::ACTIVE && Graphics::font_id >= 0)
        {
            char progress_buf[64];
            std::snprintf(progress_buf, sizeof(progress_buf),
                "BOSS %d / %d", boss_words_typed, BossConfig::BOSS_WORDS_TO_DEFEAT);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, progress_buf,
                -0.18f, 0.82f,
                0.55f,
                1.0f, 0.85f, 0.1f, 1.0f);
        }
    }

    /*
    @function       Boss_DrawFlashOverlay
    @params         meshes (const Data::Meshes&)
    @brief          Draws the white full-screen flash overlay when active.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_DrawFlashOverlay(const Data::Meshes& meshes)
    {
        if (boss_flash_active_timer <= 0.0f) return;
        if (!boss_flash_mesh && !meshes.debug_box_mesh) return;

        float alpha = boss_flash_active_timer / BossConfig::BOSS_FLASH_FADE_DURATION_SECONDS;
        alpha = Utility::Utility_Clamp(alpha, 0.0f, 1.0f);

        AEGfxVertexList* overlay_mesh = boss_flash_mesh ? boss_flash_mesh : meshes.debug_box_mesh;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();

        Graphics::Draw_DrawMeshAt(overlay_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);

        // Reset render state
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(1.0f);
    }

    /*
    @function       Boss_GetKillWordSlot
    @brief          Returns the pool slot index reserved for the boss kill-word enemy, or -1 if none.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Boss_GetKillWordSlot()
    {
        return boss_pool_slot;
    }

    /*
    @function       Boss_IsActive
    @brief          Returns true while the boss should be freezing gameplay (CUTSCENE or ACTIVE).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Boss_IsActive()
    {
        return boss_state == BossState::CUTSCENE || boss_state == BossState::ACTIVE;
    }

    /*
    @function       Boss_ForceSpawn
    @brief          Skips the 60-second timer and immediately triggers the boss spawn sequence.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_ForceSpawn()
    {
        if (boss_state != BossState::INACTIVE) return;          // already spawning or active — ignore
        boss_cycle_timer_seconds = BossConfig::BOSS_SPAWN_INTERVAL_SECONDS; // saturate timer
    }

} // BossSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/