/*!
@file           Boss.cpp
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

#include "Boss.h"
#include "AEEngine.h"

/*
@namespace      BossConfig
@brief          Tuning constants that control every aspect of boss behavior and pacing.

Every number that affects how the boss plays is defined here: how long between spawns, how fast
the boss moves, how many words the player must type to win, the shortest and longest word lengths,
how much bigger the boss sprite is, how long the red hit tint lasts, the minimum and maximum wait
between flash attacks, how long the flash overlay fades, the minion cap, the time between minion
waves, how many minions arrive per wave, how long the entrance cutscene runs, and the fixed
vertical position the boss holds during the fight. Changing any value here adjusts gameplay feel
without touching the core boss logic.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossConfig
{
    const float BOSS_SPAWN_INTERVAL_SECONDS = 60.0f;                                                                        // how many seconds pass between boss spawns
    const float BOSS_MOVE_SPEED = 120.0f;                                                                                   // how fast the boss slides left and right in pixels per second
    const int   BOSS_WORDS_TO_DEFEAT = 30;                                                                                  // how many words the player must type to kill the boss
    const int   BOSS_WORD_MIN_LEN = 3;                                                                                      // shortest word the boss or its minions can use
    const int   BOSS_WORD_MAX_LEN = 8;                                                                                      // longest word the boss or its minions can use
    const float BOSS_SPRITE_SCALE = 1.2f;                                                                                   // how much bigger the boss sprite is compared to a normal enemy
    const float BOSS_SPRITE_W_RATIO = 1.5f;                                                                                 // makes the boss sprite wider than it is tall
    const float BOSS_HIT_TINT_DURATION_SECONDS = 0.18f;                                                                     // how long the boss flashes red after being hit
    const float BOSS_FLASH_INTERVAL_MIN_SECONDS = 8.0f;                                                                     // shortest possible wait before the next screen flash attack
    const float BOSS_FLASH_INTERVAL_MAX_SECONDS = 18.0f;                                                                    // longest possible wait before the next screen flash attack
    const float BOSS_FLASH_FADE_DURATION_SECONDS = 3.0f;                                                                    // how long the white flash overlay takes to fade out
    const int   BOSS_MINION_CAP = 10;                                                                                       // maximum number of minions allowed on screen at once
    const float BOSS_MINION_SPAWN_INTERVAL_SECONDS = 20.0f;                                                                 // how many seconds between each minion wave
    const int   BOSS_MINION_WAVE_SIZE = 5;                                                                                  // how many minions spawn in one wave
    const float BOSS_CUTSCENE_DURATION_SECONDS = 5.0f;                                                                      // how long the boss entrance animation plays before gameplay resumes
    const float BOSS_POSITION_Y = 120.0f;                                                                                   // the fixed vertical position where the boss sits during a fight
} // BossConfig namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      Anonymous
@brief          Private state variables used only inside Boss.cpp.

Nothing in this block is visible outside Boss.cpp. All mutable boss state lives here, including
the state machine value, all timers, the movement direction, the word counter, the reserved pool
slot index, the current kill word string, the kill word visibility flag, the hit tint timer,
the animation row, the flash timers, and the three asset pointers.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace
{
    /*
    @enum           BossState
    @brief          Tracks which phase the boss is currently in during its lifecycle.

    The boss passes through these values in order each wave. INACTIVE is the default before the
    60 second timer fires and after the boss has been fully cleaned up. WAITING holds spawning
    blocked while regular enemies finish dying. CUTSCENE plays the slide in animation and freezes
    gameplay. ACTIVE runs the full fight loop. DEAD exists for exactly one frame to let all systems
    settle before returning to INACTIVE.
    */
    enum class BossState
    {
        INACTIVE,                                                                                                           // boss has not triggered yet or has been fully cleaned up after death
        WAITING,                                                                                                            // 60 second mark was hit and the system is waiting for the screen to clear
        CUTSCENE,                                                                                                           // boss is sliding onto screen and gameplay is frozen
        ACTIVE,                                                                                                             // boss is alive, moving, spawning minions and showing its kill word
        DEAD                                                                                                                // boss was just defeated and will clean up this frame
    };

    BossState   boss_state = BossState::INACTIVE;                                                                           // tracks which phase the boss is currently in

    // timing
    float       boss_cycle_timer_seconds = 0.0f;                                                                            // counts up every frame until it reaches the spawn interval
    float       boss_cutscene_timer_seconds = 0.0f;                                                                         // counts down to zero while the entrance cutscene plays
    float       boss_minion_spawn_timer = 0.0f;                                                                             // tracks time since the last minion wave was sent

    // movement
    float       boss_position_x = 0.0f;                                                                                     // current horizontal position of the boss in world space
    float       boss_dir = 1.0f;                                                                                            // direction the boss is moving, positive is right and negative is left

    // defeat counter
    int         boss_words_typed = 0;                                                                                       // how many kill words the player has successfully typed this fight

    // kill word pool slot
    int         boss_pool_slot = -1;                                                                                        // which slot in the enemy pool the boss is using for its kill word
    std::string boss_current_word;                                                                                          // the word the player must type to damage the boss right now

    // targeting lock
    bool        boss_kill_word_in_pool = false;                                                                             // true when the kill word is visible and targetable by the player

    // cutscene
    float       boss_cutscene_y = 0.0f;                                                                                     // vertical position of the boss sprite during the slide-in animation
    float       boss_hit_tint_timer = 0.0f;                                                                                 // counts down from BOSS_HIT_TINT_DURATION_SECONDS after each hit

    // sprite row
    int         boss_anim_row = 0;                                                                                          // which row of the sprite sheet to draw, 0 is alive and 1 is death

    // flash attack
    float       boss_flash_cooldown_timer = 0.0f;                                                                           // how many seconds until the next flash attack triggers
    float       boss_flash_active_timer = 0.0f;                                                                             // counts down while the white flash overlay is still visible

    // assets
    AEGfxTexture* boss_texture = nullptr;                                                                                   // boss sprite sheet texture
    AEGfxVertexList* boss_sprite_mesh = nullptr;                                                                            // mesh built to match the boss sprite sheet UV layout
    AEGfxVertexList* boss_flash_mesh = nullptr;                                                                             // full screen rectangle used to draw the flash overlay

    // helpers

    /*
    @function       Boss_CountActiveMinions
    @params         pool (const std::vector<Data::Enemy>&)
    @brief          Returns how many pool enemies are active and alive, excluding the boss kill word slot.

    Every slot in the pool is checked. The boss kill word slot is always skipped so it is never
    counted as a minion. Slots that are inactive or already in the dying state are also skipped
    because a dying enemy cannot block the player from reaching the kill word. The remaining count
    is what the spawning logic and the targeting lock use to decide what to show.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Boss_CountActiveMinions(const std::vector<Data::Enemy>& pool)
    {
        int count = 0;                                                                                                      // start the counter at zero
        for (int i = 0; i < (int)pool.size(); ++i)                                                                          // check every slot in the pool
        {
            if (!pool[(size_t)i].enemy_is_active) continue;                                                                 // skip slots that are not in use
            if (boss_pool_slot >= 0 && i == boss_pool_slot) continue;                                                       // skip the boss kill word slot so it is never counted as a minion
            if (pool[(size_t)i].enemy_is_dying) continue;                                                                   // skip enemies that are already dying since dying enemies cannot block the kill word
            count++;                                                                                                        // this enemy counts as a live minion
        }
        return count;                                                                                                       // return the total number of live minions found
    }

    /*
    @function       Boss_PoolIsClear
    @params         pool (const std::vector<Data::Enemy>&)
    @brief          Returns true when all regular enemies are inactive or in the dying state.

    Every slot in the pool is checked except the boss kill word slot. If a slot is active and not
    dying then a live enemy is still on screen and the function returns false. When no such slot
    exists the screen is considered clear and the boss entrance cutscene can safely begin.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Boss_PoolIsClear(const std::vector<Data::Enemy>& pool)
    {
        for (int i = 0; i < (int)pool.size(); ++i)                                                                          // check every slot in the pool
        {
            if (boss_pool_slot >= 0 && i == boss_pool_slot) continue;                                                       // always skip the boss kill word slot
            const Data::Enemy& e = pool[(size_t)i];                                                                         // get a reference to this enemy slot
            if (e.enemy_is_active && !e.enemy_is_dying) return false;                                                       // a fully alive enemy is still on screen so the pool is not clear
        }
        return true;                                                                                                        // no live enemies found so the screen is clear
    }

    /*
    @function       Boss_PickFlashCooldown
    @brief          Returns a random wait time in seconds for the next flash attack.

    The min and max interval constants are converted to integer centiseconds so
    Utility_RandInt can pick a whole number in that range. The result is then divided
    by 100 to convert back to seconds. This gives a smooth spread of wait times across
    the full allowed range each time the function is called.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Boss_PickFlashCooldown()
    {
        int min_ms = (int)(BossConfig::BOSS_FLASH_INTERVAL_MIN_SECONDS * 100.0f);                                           // convert min seconds to integer centiseconds for the rand range
        int max_ms = (int)(BossConfig::BOSS_FLASH_INTERVAL_MAX_SECONDS * 100.0f);                                           // convert max seconds to integer centiseconds for the rand range
        int picked = Utility::Utility_RandInt(min_ms, max_ms);                                                              // pick a random value between the two
        return (float)picked * 0.01f;                                                                                       // convert back to seconds and return
    }

    /*
    @function       Boss_InjectKillWord
    @params         pool (std::vector<Data::Enemy>&)
    @brief          Reveals the kill word by restoring word text on the reserved slot.

    The stored kill word string is written back into the reserved pool slot so Typing.cpp can see
    and target it. The enemy_is_active flag stays true at all times while the boss owns the slot
    because setting it to false would allow game.cpp to recycle the slot on that same frame.
    Nothing happens if the slot index is invalid or the kill word is already visible.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_InjectKillWord(std::vector<Data::Enemy>& pool)
    {
        if (boss_pool_slot < 0 || boss_pool_slot >= (int)pool.size()) return;                                               // slot is not valid so there is nothing to inject
        if (boss_kill_word_in_pool) return;                                                                                 // kill word is already visible so do nothing

        // Reveal the kill word by putting the word text back into the slot.
        // enemy_is_active stays TRUE at all times because setting it to false would let
        // game.cpp recycle this slot on that same frame and steal it from the boss.
        pool[(size_t)boss_pool_slot].enemy_word_text = boss_current_word;                                                   // restore the word text so the player can see and type it
        pool[(size_t)boss_pool_slot].enemy_word_length = (int)boss_current_word.size();                                     // update the length to match the restored word
        boss_kill_word_in_pool = true;                                                                                      // mark that the kill word is now active and targetable
    }

    /*
    @function       Boss_RemoveKillWord
    @params         pool (std::vector<Data::Enemy>&)
    @brief          Hides the kill word so Typing.cpp cannot see or target it.

    The word text on the reserved slot is cleared so the typing system skips over it. The slot
    is not returned to free_slots because the boss holds ownership for the entire fight. The
    enemy_is_active flag stays true throughout. Nothing happens if the slot is invalid or the
    kill word is already hidden.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_RemoveKillWord(std::vector<Data::Enemy>& pool)
    {
        if (boss_pool_slot < 0 || boss_pool_slot >= (int)pool.size()) return;                                               // slot is not valid so there is nothing to remove
        if (!boss_kill_word_in_pool) return;                                                                                // kill word is already hidden so do nothing

        // Hide the kill word by clearing the word text.
        // Typing.cpp skips enemies with empty word text so the slot becomes invisible to the player.
        // enemy_is_active stays TRUE to keep the boss owning the slot at all times.
        pool[(size_t)boss_pool_slot].enemy_word_text = "";                                                                  // clear the text so the slot is invisible to the typing system
        pool[(size_t)boss_pool_slot].enemy_word_length = 0;                                                                 // set length to zero to match the empty text
        boss_kill_word_in_pool = false;                                                                                     // mark that the kill word is now hidden
    }

    /*
    @function       Boss_SwapKillWord
    @params         pool (std::vector<Data::Enemy>&), bank (const Data::WordBank&)
    @brief          Picks a new unique word and updates the kill word slot with it.

    Enemy_PickUniqueWordRandomLength is asked for a fresh word that is not already on screen.
    If no unique word can be found the old kill word is kept as a fallback so the slot is never
    left empty. The new word is stored in boss_current_word and, if the kill word slot is currently
    visible, the pool slot is updated immediately so the player sees the change right away.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_SwapKillWord(std::vector<Data::Enemy>& pool, const Data::WordBank& bank)
    {
        std::string new_word = EnemySystem::Enemy_PickUniqueWordRandomLength(                                               // ask the enemy system for a fresh word not already on screen
            BossConfig::BOSS_WORD_MIN_LEN,
            BossConfig::BOSS_WORD_MAX_LEN,
            bank, pool, true,
            EnemyConfig::MAX_WORD_TRIES);

        if (new_word.empty()) new_word = boss_current_word;                                                                 // if no unique word was found keep the old one as a fallback

        boss_current_word = new_word;                                                                                       // store the chosen word as the new current kill word

        if (boss_kill_word_in_pool && boss_pool_slot >= 0 && boss_pool_slot < (int)pool.size())                             // only update the slot if the kill word is currently visible
        {
            pool[(size_t)boss_pool_slot].enemy_word_text = boss_current_word;                                               // push the new word into the slot so the player sees it
            pool[(size_t)boss_pool_slot].enemy_word_length = (int)boss_current_word.size();                                 // update the length to match the new word
        }
    }

} // anonymous namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      BossSystem
@brief          Public functions that manage the entire boss lifecycle.

All public functions here match the call sites in game.cpp. The namespace handles spawn timing,
the pool clear wait, the entrance cutscene, boss movement, minion wave spawning, kill word pool
injection and removal, hit feedback, flash attack timing, sprite drawing, and defeat cleanup.
All internal state lives in the anonymous namespace above and is not visible outside Boss.cpp.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossSystem
{
    /*
    @function       Boss_Init
    @params         meshes (Data::Meshes&), bank (const Data::WordBank&)
    @brief          Loads the boss texture, builds the boss sprite mesh, and resets all boss state.

    Assets/bossEnemy.png is loaded and a sprite quad is built using 2 rows and 6 columns to match
    the sheet layout. A plain white unit rectangle is also built for the flash overlay. Every state
    variable including timers, counters, and flags is then set back to its default inactive value.
    This function must be called once after EnemyManage_LoadMeshes so the base meshes are ready.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Init(Data::Meshes& /*meshes*/, const Data::WordBank& /*bank*/)
    {
        // Load boss texture
        boss_texture = AEGfxTextureLoad("Assets/bossEnemy.png");                                                            // load the boss sprite sheet from disk
        AE_ASSERT_MESG(boss_texture, "Failed to load: Assets/bossEnemy.png");                                               // crash with a message if the file was not found

        // Build boss sprite mesh using 2 rows and 6 columns matching the sprite sheet layout
        {
            Data::SpriteAnim boss_anim{};                                                                                   // temporary anim struct used only to calculate UV dimensions
            boss_anim.animation_columns = 6;                                                                                // the sheet has 6 frames per row
            boss_anim.animation_rows = 2;                                                                                   // row 0 is alive and row 1 is the death animation
            boss_sprite_mesh = Graphics::Mesh_BuildSpriteQuad(                                                              // build a quad whose UVs match one frame of the sheet
                boss_anim.Anim_UvWidth(),
                boss_anim.Anim_UvHeight(),
                0xFFFFFFFF);
        }

        // Build full screen flash overlay mesh
        boss_flash_mesh = Graphics::Mesh_BuildUnitRect(0xFFFFFFFF);                                                         // plain white unit rectangle that covers the whole screen

        // reset all state variables to starting values
        boss_state = BossState::INACTIVE;                                                                                   // boss starts inactive
        boss_cycle_timer_seconds = 0.0f;                                                                                    // reset the spawn cycle clock
        boss_cutscene_timer_seconds = 0.0f;                                                                                 // reset the cutscene countdown
        boss_minion_spawn_timer = 0.0f;                                                                                     // reset the minion wave timer
        boss_position_x = 0.0f;                                                                                             // place boss at the centre horizontally
        boss_dir = 1.0f;                                                                                                    // boss starts moving to the right
        boss_words_typed = 0;                                                                                               // no words have been typed yet
        boss_pool_slot = -1;                                                                                                // no pool slot reserved yet
        boss_current_word.clear();                                                                                          // no kill word assigned yet
        boss_kill_word_in_pool = false;                                                                                     // kill word is not visible yet
        boss_hit_tint_timer = 0.0f;                                                                                         // no hit flash active
        boss_flash_cooldown_timer = Boss_PickFlashCooldown();                                                               // set a random wait time for the first flash attack
        boss_flash_active_timer = 0.0f;                                                                                     // no flash overlay visible
        boss_anim_row = 0;                                                                                                  // start on the alive animation row
    }

    /*
    @function       Boss_Free
    @params         meshes (Data::Meshes&), free_slots (std::deque<int>&), pool (std::vector<Data::Enemy>&)
    @brief          Unloads the boss texture and frees the boss meshes without touching other shared meshes.

    Only the resources that Boss_Init created are released. If the boss had a reserved kill word
    slot when this function was called, that slot is marked inactive and returned to free_slots so
    no orphaned active slot remains after a restart or level exit. Null checks are performed before
    every free so the function is safe to call even if Boss_Init never completed.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Free(Data::Meshes& /*meshes*/, std::deque<int>& free_slots, std::vector<Data::Enemy>& pool)
    {
        if (boss_sprite_mesh)                                                                                               // only free the mesh if it was actually created
        {
            AEGfxMeshFree(boss_sprite_mesh);                                                                                // release the boss sprite mesh from GPU memory
            boss_sprite_mesh = nullptr;                                                                                     // clear the pointer so it cannot be freed again
        }

        if (boss_flash_mesh)                                                                                                // only free the flash mesh if it was actually created
        {
            AEGfxMeshFree(boss_flash_mesh);                                                                                 // release the flash overlay mesh from GPU memory
            boss_flash_mesh = nullptr;                                                                                      // clear the pointer so it cannot be freed again
        }

        if (boss_texture)                                                                                                   // only unload the texture if it was actually loaded
        {
            AEGfxTextureUnload(boss_texture);                                                                               // unload the boss texture from GPU memory
            boss_texture = nullptr;                                                                                         // clear the pointer so it cannot be unloaded again
        }

        // Return the reserved kill word slot so it is not lost when the game restarts or exits.
        // The slot is marked inactive first so no other system thinks it is still in use.
        if (boss_pool_slot >= 0)                                                                                            // only do this if the boss actually had a reserved slot
        {
            pool[(size_t)boss_pool_slot].enemy_is_active = false;                                                           // deactivate the slot before giving it back
            free_slots.push_back(boss_pool_slot);                                                                           // return the slot index to the free list
            boss_pool_slot = -1;                                                                                            // clear the reservation so nothing references the slot anymore
        }

        boss_state = BossState::INACTIVE;                                                                                   // mark the boss as fully inactive
        boss_kill_word_in_pool = false;                                                                                     // clear the kill word visibility flag
        boss_current_word.clear();                                                                                          // clear the kill word text
    }

    /*
    @function       Boss_Update
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), dt (float), boss_spawning_blocked (bool&)
    @brief          Full per frame boss tick covering timing, state transitions, movement, and minion management.

    This function owns the 60 second spawn trigger, the pool clear wait, the cutscene countdown,
    boss movement, minion wave spawning, the targeting lock that hides or shows the kill word
    based on whether minions are alive, hit tint decay, and flash attack timing. The
    boss_spawning_blocked flag is set to true from the moment the 60 second mark is hit until
    the boss is fully defeated so the caller can stop regular enemy spawning during the entire
    boss event.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Update(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, float dt, bool& boss_spawning_blocked)
    {
        // Flash attack timer runs every frame while the boss is active.
        // The caller already skips this function when the game is paused.
        if (boss_state == BossState::ACTIVE)                                                                                // only tick the flash timer when the boss is actually fighting
        {
            boss_flash_cooldown_timer -= dt;                                                                                // count down toward the next flash attack
            if (boss_flash_cooldown_timer <= 0.0f)                                                                          // check if it is time to fire the flash
            {
                boss_flash_active_timer = BossConfig::BOSS_FLASH_FADE_DURATION_SECONDS;                                     // start the flash overlay fading in
                boss_flash_cooldown_timer = Boss_PickFlashCooldown();                                                       // pick a new random wait time for the next flash
            }
        }

        if (boss_flash_active_timer > 0.0f)                                                                                 // only tick if the flash overlay is still showing
            boss_flash_active_timer -= dt;                                                                                  // reduce the flash alpha each frame until it fades out

        // Hit tint decay
        if (boss_hit_tint_timer > 0.0f)                                                                                     // only tick if the red hit tint is still showing
            boss_hit_tint_timer -= dt;                                                                                      // reduce the tint timer each frame until it clears

        // State machine
        switch (boss_state)
        {
            // INACTIVE state
        case BossState::INACTIVE:
        {
            boss_cycle_timer_seconds += dt;                                                                                 // count up each frame toward the spawn trigger time

            if (boss_cycle_timer_seconds >= BossConfig::BOSS_SPAWN_INTERVAL_SECONDS)                                        // check if the spawn timer has reached the threshold
            {
                // 60 second mark reached, block regular enemy spawning right away
                boss_spawning_blocked = true;                                                                               // stop normal enemies from spawning so the screen can clear
                boss_state = BossState::WAITING;                                                                            // move to the waiting state until the screen is empty
            }
            break;
        }

        // WAITING state
        case BossState::WAITING:
        {
            boss_spawning_blocked = true;                                                                                   // keep normal spawning blocked during the wait

            if (Boss_PoolIsClear(pool))                                                                                     // check if all regular enemies are gone
            {
                // Reserve a pool slot for the boss kill word enemy
                if (free_slots.empty()) break;                                                                              // no free slot available yet so keep waiting

                boss_pool_slot = free_slots.front();                                                                        // grab the first available slot index
                free_slots.pop_front();                                                                                     // remove it from the free list so nothing else can use it

                // Pick the first kill word for this fight
                boss_current_word = EnemySystem::Enemy_PickUniqueWordRandomLength(                                          // ask the enemy system for a fresh unique word
                    BossConfig::BOSS_WORD_MIN_LEN,
                    BossConfig::BOSS_WORD_MAX_LEN,
                    bank, pool, true,
                    EnemyConfig::MAX_WORD_TRIES);

                if (boss_current_word.empty()) boss_current_word = "BOSS";                                                  // use BOSS as a fallback if no unique word could be found

                // Set up the kill word enemy in the reserved slot.
                // enemy_has_landed and enemy_did_damage are both true so Enemy_Update
                // never tries to move it or deal landing damage.
                Data::Enemy kill_word_enemy{};                                                                              // create a blank enemy struct to fill in
                kill_word_enemy.enemy_is_active = true;                                                                     // keep this slot active permanently so game.cpp never recycles it
                kill_word_enemy.enemy_has_landed = true;                                                                    // pretend it has already landed to stop Enemy_Update from moving it
                kill_word_enemy.enemy_did_damage = true;                                                                    // pretend damage was already applied to stop landing damage
                kill_word_enemy.enemy_is_dying = false;                                                                     // it is not dying
                kill_word_enemy.enemy_type = Data::EnemyType::NORMAL;                                                       // treat it as a normal enemy type for drawing purposes
                kill_word_enemy.enemy_word_text = "";                                                                       // start hidden, Boss_InjectKillWord will reveal it later
                kill_word_enemy.enemy_word_length = 0;                                                                      // length matches the empty hidden text
                kill_word_enemy.enemy_word_text_alt.clear();                                                                // no alternate word needed
                kill_word_enemy.enemy_shield_intact = false;                                                                // no shield on this slot
                kill_word_enemy.enemy_fall_speed_units_per_second = 0.0f;                                                   // this slot never falls
                kill_word_enemy.enemy_lane_id = -1;                                                                         // not assigned to any lane
                kill_word_enemy.enemy_position_x = 0.0f;                                                                    // start centred, Boss_Update will move it each frame
                kill_word_enemy.enemy_position_y = BossConfig::BOSS_POSITION_Y;                                             // place it at the boss fixed Y position
                kill_word_enemy.enemy_animation.animation_row_index = 0;                                                    // start on the first animation row
                kill_word_enemy.enemy_animation.animation_frame_index = 0;                                                  // start on the first animation frame
                kill_word_enemy.enemy_animation.animation_timer_seconds = 0.0f;                                             // reset the frame timer
                kill_word_enemy.enemy_animation.animation_uv_x = 0.0f;                                                      // reset the UV offset X
                kill_word_enemy.enemy_animation.animation_uv_y = 0.0f;                                                      // reset the UV offset Y
                kill_word_enemy.enemy_death_row_index = 1;                                                                  // row 1 is the death animation row

                pool[(size_t)boss_pool_slot] = kill_word_enemy;                                                             // write the prepared enemy into the reserved pool slot
                boss_kill_word_in_pool = false;                                                                             // kill word is not visible yet, cutscene comes first

                // Start the entrance cutscene, boss slides in from above the screen
                boss_cutscene_timer_seconds = BossConfig::BOSS_CUTSCENE_DURATION_SECONDS;                                   // set how long the cutscene lasts
                boss_cutscene_y = 600.0f;                                                                                   // start the boss sprite well above the visible area
                boss_position_x = 0.0f;                                                                                     // start the boss centred horizontally
                boss_dir = 1.0f;                                                                                            // boss will move right first
                boss_words_typed = 0;                                                                                       // reset the word counter for this new fight
                boss_minion_spawn_timer = 0.0f;                                                                             // reset the minion wave timer
                boss_hit_tint_timer = 0.0f;                                                                                 // clear any leftover hit tint
                boss_flash_cooldown_timer = Boss_PickFlashCooldown();                                                       // pick a random wait before the first flash attack
                boss_flash_active_timer = 0.0f;                                                                             // no flash overlay at the start

                boss_state = BossState::CUTSCENE;                                                                           // move to the cutscene state
            }
            break;
        }

        // CUTSCENE state
        case BossState::CUTSCENE:
        {
            boss_spawning_blocked = true;                                                                                   // keep normal enemy spawning blocked during the cutscene
            // Gameplay is frozen by the caller through the Boss_IsActive check

            boss_cutscene_timer_seconds -= dt;                                                                              // count down the cutscene timer each frame

            // Slide the boss from off screen at the top down to its normal fight position
            float progress = 1.0f - (boss_cutscene_timer_seconds / BossConfig::BOSS_CUTSCENE_DURATION_SECONDS);             // 0 at the start and 1 at the end of the cutscene
            progress = Utility::Utility_Clamp(progress, 0.0f, 1.0f);                                                        // clamp so it never goes outside the 0 to 1 range
            boss_cutscene_y = Utility::Utility_Lerp(600.0f, BossConfig::BOSS_POSITION_Y, progress);                         // smoothly move from the top entry position to the fight position

            if (boss_cutscene_timer_seconds <= 0.0f)                                                                        // check if the cutscene has finished
            {
                // Cutscene is over, snap to final position and begin the fight
                boss_cutscene_y = BossConfig::BOSS_POSITION_Y;                                                              // lock the boss to its exact fight position
                Boss_InjectKillWord(pool);                                                                                  // reveal the kill word so the player can start typing
                boss_state = BossState::ACTIVE;                                                                             // switch to the active fighting state
            }
            break;
        }

        // ACTIVE state
        case BossState::ACTIVE:
        {
            boss_spawning_blocked = true;                                                                                   // keep normal enemy spawning blocked while the boss is alive

            // Check if Typing.cpp just matched the kill word this frame.
            // When Typing.cpp calls Enemy_Killed on this slot it sets enemy_is_dying to true.
            if (boss_pool_slot >= 0 && boss_pool_slot < (int)pool.size())                                                   // make sure the slot index is still valid
            {
                Data::Enemy& kw = pool[(size_t)boss_pool_slot];                                                             // get a reference to the kill word enemy slot
                if (kw.enemy_is_dying)                                                                                      // check if the player just typed the kill word
                {
                    // A hit was registered. Cancel dying immediately so Enemy_Update
                    // cannot start dropping this slot away from BOSS_POSITION_Y.
                    kw.enemy_is_dying = false;                                                                              // cancel the dying state so Enemy_Update ignores this slot
                    kw.enemy_has_landed = true;                                                                             // restore the guard that keeps Enemy_Update from moving the slot

                    boss_words_typed++;                                                                                     // count this as one successful hit on the boss
                    boss_hit_tint_timer = BossConfig::BOSS_HIT_TINT_DURATION_SECONDS;                                       // start the red flash to show the boss was hit

                    if (boss_words_typed >= BossConfig::BOSS_WORDS_TO_DEFEAT)                                               // check if the boss has taken enough hits to die
                    {
                        // Boss is defeated, clean up the slot and end the fight
                        kw.enemy_is_active = false;                                                                         // deactivate the kill word slot
                        boss_kill_word_in_pool = false;                                                                     // mark the kill word as no longer visible

                        // Return the slot to the free pool
                        free_slots.push_back(boss_pool_slot);                                                               // give the slot index back so it can be reused
                        boss_pool_slot = -1;                                                                                // clear the stored slot reference

                        boss_cycle_timer_seconds = 0.0f;                                                                    // reset the spawn cycle clock for the next boss wave
                        boss_spawning_blocked = false;                                                                      // allow normal enemy spawning to resume
                        boss_anim_row = 1;                                                                                  // switch to the death animation row on the sprite sheet
                        boss_state = BossState::DEAD;                                                                       // move to the dead state
                        break;
                    }
                    else
                    {
                        // Boss survived, swap in a new kill word for the next hit
                        kw.enemy_word_text = "";                                                                            // hide the old word immediately using empty text
                        kw.enemy_word_length = 0;                                                                           // clear the length to match the empty text
                        boss_kill_word_in_pool = false;                                                                     // mark the kill word as hidden

                        Boss_SwapKillWord(pool, bank);                                                                      // pick a fresh word and update the slot

                        // Show the new word right away if no minions are alive
                        if (Boss_CountActiveMinions(pool) == 0)                                                             // check if there are any minions on screen
                            Boss_InjectKillWord(pool);                                                                      // no minions so reveal the new kill word immediately
                    }
                }
            }

            // Targeting lock: show the kill word only when no minions are alive
            int minion_count = Boss_CountActiveMinions(pool);                                                               // count how many minions are currently on screen

            if (minion_count == 0 && !boss_kill_word_in_pool)                                                               // no minions and kill word is hidden
                Boss_InjectKillWord(pool);                                                                                  // reveal the kill word since the screen is clear
            else if (minion_count > 0 && boss_kill_word_in_pool)                                                            // minions are alive and kill word is visible
                Boss_RemoveKillWord(pool);                                                                                  // hide the kill word until all minions are gone

            // Move the kill word slot to always follow the boss position
            if (boss_pool_slot >= 0 && boss_pool_slot < (int)pool.size())                                                   // make sure the slot index is still valid
            {
                pool[(size_t)boss_pool_slot].enemy_position_x = boss_position_x;                                            // keep the kill word enemy locked to the boss X position
                pool[(size_t)boss_pool_slot].enemy_position_y = BossConfig::BOSS_POSITION_Y;                                // keep the kill word enemy locked to the boss Y position
            }

            // Boss movement, bounce left and right within the play area
            boss_position_x += boss_dir * BossConfig::BOSS_MOVE_SPEED * dt;                                                 // move the boss horizontally based on its speed and direction

            float boss_half = (EnemyConfig::ENEMY_SIZE_PX * BossConfig::BOSS_SPRITE_SCALE *                                 // calculate how wide half the boss sprite is in world space
                BossConfig::BOSS_SPRITE_W_RATIO) * 0.5f;

            if (boss_position_x + boss_half >= area.area_max_x)                                                             // check if the right edge of the boss hit the right boundary
            {
                boss_position_x = area.area_max_x - boss_half;                                                              // push the boss back inside the right boundary
                boss_dir = -1.0f;                                                                                           // reverse direction to move left
            }
            else if (boss_position_x - boss_half <= area.area_min_x)                                                        // check if the left edge of the boss hit the left boundary
            {
                boss_position_x = area.area_min_x + boss_half;                                                              // push the boss back inside the left boundary
                boss_dir = 1.0f;                                                                                            // reverse direction to move right
            }

            // Minion spawning, send a wave every BOSS_MINION_SPAWN_INTERVAL_SECONDS
            if (minion_count < BossConfig::BOSS_MINION_CAP)                                                                 // only spawn if the minion count is under the cap
            {
                boss_minion_spawn_timer += dt;                                                                              // count up toward the next wave trigger
                if (boss_minion_spawn_timer >= BossConfig::BOSS_MINION_SPAWN_INTERVAL_SECONDS)                              // check if enough time has passed for a new wave
                {
                    boss_minion_spawn_timer = 0.0f;                                                                         // reset the wave timer

                    int wave = BossConfig::BOSS_MINION_WAVE_SIZE;                                                           // how many minions to try spawning this wave
                    int current = Boss_CountActiveMinions(pool);                                                            // check the current live minion count before spawning
                    for (int w = 0; w < wave && current < BossConfig::BOSS_MINION_CAP; ++w)                                 // loop through each spawn slot in the wave
                    {
                        int len = Utility::Utility_RandInt(BossConfig::BOSS_WORD_MIN_LEN, BossConfig::BOSS_WORD_MAX_LEN);   // pick a random word length for this minion
                        int before_spawn = Boss_CountActiveMinions(pool);                                                   // snapshot the count before attempting the spawn
                        EnemySystem::Enemy_SpawnFromPool(pool, free_slots, area, bank, len);                                // try to spawn one minion into a free pool slot
                        if (Boss_CountActiveMinions(pool) > before_spawn)                                                   // check if the spawn actually succeeded
                            ++current;                                                                                      // only advance the counter when a real spawn happened
                    }
                }
            }

            break;
        }

        // DEAD state
        case BossState::DEAD:
        {
            // One frame pause to let all systems settle before going back to inactive
            boss_spawning_blocked = false;                                                                                  // allow normal enemy spawning to resume
            boss_anim_row = 0;                                                                                              // reset the animation row ready for the next boss wave
            boss_state = BossState::INACTIVE;                                                                               // return to inactive so the cycle timer can start again
            break;
        }

        } // end switch
    }

    /*
    @function       Boss_Draw
    @params         meshes (const Data::Meshes&)
    @brief          Renders the boss sprite, hit tint, kill word label, and progress counter.

    The boss sprite is drawn at the current position using the correct sprite sheet row. A red
    tint is applied while the hit tint timer is active and fades back to white as it counts down.
    During the cutscene a dark overlay and a warning banner are drawn on top of the sprite. When
    the boss is active and the kill word is visible, the word is printed centered above the sprite.
    The BOSS X of Y progress counter is also printed while the boss is in the active state.
    Nothing is drawn when the boss is inactive or waiting.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Draw(const Data::Meshes& meshes)
    {
        if (boss_state != BossState::CUTSCENE &&
            boss_state != BossState::ACTIVE &&
            boss_state != BossState::DEAD)
            return;                                                                                                         // boss is not visible in any other state so skip drawing

        if (!boss_texture || !boss_sprite_mesh) return;                                                                     // assets are missing so there is nothing to draw

        // Draw the boss sprite first so all overlays and text appear on top of it.
        // During the cutscene boss_cutscene_y is used, otherwise the fixed fight position is used.
        float draw_y = (boss_state == BossState::CUTSCENE) ? boss_cutscene_y : BossConfig::BOSS_POSITION_Y;                 // pick the correct Y depending on whether the cutscene is playing

        float sprite_base = EnemyConfig::ENEMY_SIZE_PX * BossConfig::BOSS_SPRITE_SCALE;                                     // base size of the boss in pixels
        float sprite_w = sprite_base * BossConfig::BOSS_SPRITE_W_RATIO;                                                     // final draw width of the boss sprite
        float sprite_h = sprite_base;                                                                                       // final draw height of the boss sprite

        Graphics::Graphics_SetSpriteState();                                                                                // set the render mode to textured sprite drawing

        // Apply red hit tint when the boss was just struck, fade back to white over time
        if (boss_hit_tint_timer > 0.0f)                                                                                     // check if the hit tint is still active
        {
            float t = Utility::Utility_Clamp(boss_hit_tint_timer / BossConfig::BOSS_HIT_TINT_DURATION_SECONDS, 0.0f, 1.0f); // normalize the timer to a 0 to 1 range for the lerp
            float g = Utility::Utility_Lerp(1.0f, 0.0f, t);                                                                 // green channel goes to 0 when fully tinted red
            float b = Utility::Utility_Lerp(1.0f, 0.0f, t);                                                                 // blue channel goes to 0 when fully tinted red
            AEGfxSetColorToMultiply(1.0f, g, b, 1.0f);                                                                      // apply the red tint to the sprite
        }
        else                                                                                                                // no hit tint active
        {
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                // draw the sprite with no colour modification
        }

        // Select the correct row from the sprite sheet
        const float uv_row_height = 1.0f / 2.0f;                                                                            // each row takes up half the texture height since there are 2 rows
        float uv_y = (float)boss_anim_row * uv_row_height;                                                                  // calculate the V offset for the current animation row

        AEGfxTextureSet(boss_texture, 0.0f, uv_y);                                                                          // bind the boss texture and scroll to the correct row
        Graphics::Draw_DrawMeshAt(boss_sprite_mesh,                                                                         // draw the boss sprite at its current position
            boss_position_x, draw_y,
            sprite_w, sprite_h, 0.0f);

        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                    // reset colour multiply to white so nothing else is tinted

        // Cutscene overlay: dark screen and warning banner drawn over the boss sprite
        if (boss_state == BossState::CUTSCENE)                                                                              // only draw the cutscene elements during the entrance
        {
            // Draw a slight dark overlay over the whole screen at 30 percent opacity
            if (meshes.debug_box_mesh)                                                                                      // check that the box mesh is available for drawing
            {
                float win_w = (float)AEGfxGetWindowWidth();                                                                 // get the current window width in pixels
                float win_h = (float)AEGfxGetWindowHeight();                                                                // get the current window height in pixels
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);                                                                        // switch to solid colour drawing mode
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                         // use alpha blending so the overlay is see through
                AEGfxSetTransparency(0.30f);                                                                                // set the overlay to 30 percent opacity
                AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);                                                            // use black so the screen darkens
                Graphics::Draw_DrawMeshAt(meshes.debug_box_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);                           // draw the dark rectangle over the whole screen
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                            // reset the colour multiply back to white
                AEGfxSetTransparency(1.0f);                                                                                 // reset transparency back to fully opaque
            }

            // Draw the boss incoming warning banner on top of everything
            if (Graphics::font_id >= 0)                                                                                     // check that a font is loaded before trying to print text
            {
                const char* banner = "!! BOSS INCOMING !!";                                                                 // the warning message to display on screen
                float tw = 0.0f, th = 0.0f;                                                                                 // will hold the measured text width and height
                AEGfxGetPrintSize(Graphics::font_id, banner, 0.9f, &tw, &th);                                               // measure how wide and tall the banner text will be
                float bx = -(tw * 0.5f);                                                                                    // offset left by half the width to centre the text on screen
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                         // use blending so the text renders correctly over the overlay
                AEGfxPrint(Graphics::font_id, banner, bx, 0.25f, 0.9f, 1.0f, 0.1f, 0.1f, 1.0f);                             // draw the banner in red near the top of the screen
            }
        }

        // Draw the kill word label above the boss when it is active and visible
        if (boss_state == BossState::ACTIVE && boss_kill_word_in_pool && !boss_current_word.empty())                        // only draw when the kill word is currently shown to the player
        {
            if (Graphics::font_id >= 0)                                                                                     // check that a font is loaded
            {
                // Position the label a fixed distance above the top of the boss sprite
                float sprite_half = sprite_h * 0.5f;                                                                        // half the sprite height in world space
                float label_world_y = BossConfig::BOSS_POSITION_Y + sprite_half + 20.0f;                                    // 20 pixels above the top edge of the sprite

                float ndc_x = 0.0f, ndc_y = 0.0f;                                                                           // will hold the converted screen space coordinates
                Utility::Convert_WorldToNdc(boss_position_x, label_world_y, ndc_x, ndc_y);                                  // convert the world position to normalized device coordinates

                float text_w = 0.0f, text_h = 0.0f;                                                                         // will hold the measured text dimensions
                AEGfxGetPrintSize(Graphics::font_id, boss_current_word.c_str(),                                             // measure the kill word text size
                    EnemyConfig::TEXT_SCALE, &text_w, &text_h);

                float draw_x = ndc_x - (text_w * 0.5f);                                                                     // offset left by half the text width to centre it on the boss
                float draw_label_y = ndc_y;                                                                                 // use the converted Y directly with no extra offset

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                         // use blending so the text draws cleanly
                AEGfxPrint(Graphics::font_id, boss_current_word.c_str(),                                                    // print the kill word above the boss
                    draw_x, draw_label_y,
                    EnemyConfig::TEXT_SCALE,
                    0.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        // Draw the BOSS X of Y progress counter while the boss is alive
        if (boss_state == BossState::ACTIVE && Graphics::font_id >= 0)                                                      // only draw when the boss is in the active fighting state
        {
            char progress_buf[64];                                                                                          // buffer to hold the formatted progress string
            std::snprintf(progress_buf, sizeof(progress_buf),                                                               // format the progress text as BOSS X of Y
                "BOSS %d / %d", boss_words_typed, BossConfig::BOSS_WORDS_TO_DEFEAT);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                             // use blending so the text renders correctly
            AEGfxPrint(Graphics::font_id, progress_buf,                                                                     // draw the progress counter near the top of the screen
                -0.18f, 0.82f,
                0.55f,
                1.0f, 0.85f, 0.1f, 1.0f);
        }
    }

    /*
    @function       Boss_DrawFlashOverlay
    @params         meshes (const Data::Meshes&)
    @brief          Renders a white full screen overlay when a flash attack is in progress.

    A white rectangle scaled to fill the entire window is drawn with alpha calculated from
    how much time is left on the flash timer. The dedicated flash mesh is used when available
    and the debug box mesh is used as a fallback. Nothing is drawn when the flash timer is at
    zero or below.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_DrawFlashOverlay(const Data::Meshes& meshes)
    {
        if (boss_flash_active_timer <= 0.0f) return;                                                                        // no flash is active so there is nothing to draw
        if (!boss_flash_mesh && !meshes.debug_box_mesh) return;                                                             // no mesh available to draw the overlay with

        float alpha = boss_flash_active_timer / BossConfig::BOSS_FLASH_FADE_DURATION_SECONDS;                               // calculate how opaque the flash should be right now
        alpha = Utility::Utility_Clamp(alpha, 0.0f, 1.0f);                                                                  // clamp to a valid opacity range

        AEGfxVertexList* overlay_mesh = boss_flash_mesh ? boss_flash_mesh : meshes.debug_box_mesh;                          // use the dedicated flash mesh or fall back to the debug box mesh

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);                                                                                // switch to solid colour mode for the overlay
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // use alpha blending so the flash fades smoothly
        AEGfxSetTransparency(alpha);                                                                                        // set the overlay opacity based on how far the fade has progressed
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                    // use white multiply so the overlay stays white
        AEGfxSetColorToAdd(1.0f, 1.0f, 1.0f, 0.0f);                                                                         // add white on top to make the flash extra bright

        float win_w = (float)AEGfxGetWindowWidth();                                                                         // get the window width to size the overlay
        float win_h = (float)AEGfxGetWindowHeight();                                                                        // get the window height to size the overlay

        Graphics::Draw_DrawMeshAt(overlay_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);                                            // draw the white rectangle over the full screen

        // Reset render state after drawing the overlay
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);                                                                         // clear the colour add so nothing else gets brightened
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                    // reset colour multiply back to white
        AEGfxSetTransparency(1.0f);                                                                                         // reset transparency back to fully opaque
    }

    /*
    @function       Boss_GetKillWordSlot
    @brief          Returns the pool slot index reserved for the boss kill word enemy, or -1 if none.

    This function gives read only access to the slot index the boss holds in the enemy pool.
    A return value of -1 means the boss does not currently own a slot, which is the case when
    the boss is inactive or has not yet been triggered for the current wave.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Boss_GetKillWordSlot()
    {
        return boss_pool_slot;                                                                                              // return the reserved slot index or -1 if there is none
    }

    /*
    @function       Boss_IsActive
    @brief          Returns true while the boss is on screen and part of gameplay.

    True is returned during both the entrance cutscene and the main fight phase. Callers use
    this to pause normal enemy spawning and freeze regular gameplay logic while the boss event
    is running. False is returned in all other states including INACTIVE, WAITING, and DEAD.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Boss_IsActive()
    {
        return boss_state == BossState::CUTSCENE || boss_state == BossState::ACTIVE;                                        // return true during the cutscene and while the boss is fighting
    }

    /*
    @function       Boss_ForceSpawn
    @brief          Immediately triggers the boss spawn sequence, bypassing the 60 second timer.

    The cycle timer is set to the spawn interval threshold so the next Boss_Update call moves
    the state from INACTIVE to WAITING and then proceeds normally through pool clear, cutscene,
    and ACTIVE. Nothing happens if the boss is already active or in any state other than INACTIVE.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_ForceSpawn()
    {
        if (boss_state != BossState::INACTIVE) return;                                                                      // boss is already active or spawning so ignore this call
        boss_cycle_timer_seconds = BossConfig::BOSS_SPAWN_INTERVAL_SECONDS;                                                 // set the timer to the trigger threshold so it fires next update
    }

} // BossSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/