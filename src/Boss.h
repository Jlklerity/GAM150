/*!
@file           Boss.h
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#pragma once

#include "pch.h"

/*
@namespace      BossConfig
@brief          Centralized configuration values that control boss behavior, pacing, and visual feedback.

This namespace defines every tuning value for the boss system, covering spawn timing, movement speed,
pool limits, word counts, sprite sizing, hit tint duration, flash attack parameters, minion fall speed,
minion cap, and cutscene timing. Changing values here adjusts gameplay feel without touching core boss logic.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossConfig
{
    // Spawn timing
    extern const float BOSS_SPAWN_INTERVAL_SECONDS;                                                                         // how many seconds pass between boss spawns

    // Movement
    extern const float BOSS_MOVE_SPEED;                                                                                     // how fast the boss slides left and right in pixels per second

    // Defeat requirement
    extern const int   BOSS_WORDS_TO_DEFEAT;                                                                                // how many words the player must type to kill the boss

    // Word lengths
    extern const int   BOSS_WORD_MIN_LEN;                                                                                   // shortest word the boss or its minions can use
    extern const int   BOSS_WORD_MAX_LEN;                                                                                   // longest word the boss or its minions can use

    // Sprite sizing
    extern const float BOSS_SPRITE_SCALE;                                                                                   // how much bigger the boss sprite is compared to a normal enemy
    extern const float BOSS_SPRITE_W_RATIO;                                                                                 // makes the boss sprite wider than it is tall

    // Hit-flash feedback
    extern const float BOSS_HIT_TINT_DURATION_SECONDS;                                                                      // how long the boss flashes red after being hit

    // Flash attack
    extern const float BOSS_FLASH_INTERVAL_MIN_SECONDS;                                                                     // shortest possible wait before the next screen flash attack
    extern const float BOSS_FLASH_INTERVAL_MAX_SECONDS;                                                                     // longest possible wait before the next screen flash attack
    extern const float BOSS_FLASH_FADE_DURATION_SECONDS;                                                                    // how long the white flash overlay takes to fade out

    // Minions
    extern const int   BOSS_MINION_CAP;                                                                                     // maximum number of minions allowed on screen at once
    extern const float BOSS_MINION_SPAWN_INTERVAL_SECONDS;                                                                  // how many seconds between each minion wave
    extern const int   BOSS_MINION_WAVE_SIZE;                                                                               // how many minions spawn in one wave

    // Cutscene
    extern const float BOSS_CUTSCENE_DURATION_SECONDS;                                                                      // how long the boss entrance animation plays before gameplay resumes
    extern const float BOSS_POSITION_Y;                                                                                     // the fixed vertical position where the boss sits during a fight

    // Trick Room special attack
    extern const int   BOSS_TRICK_ROOM_FIRST_TRIGGER;                                                                       // fire Trick Room after the player types this many kill words
    extern const int   BOSS_TRICK_ROOM_INTERVAL;                                                                            // fire Trick Room again every N kill words after the first trigger
    extern const float BOSS_TRICK_ROOM_DURATION_SECONDS;                                                                    // how long each Trick Room phase lasts
    extern const float BOSS_TRICK_ROOM_COOLDOWN_SECONDS;                                                                    // minimum gap before Trick Room can trigger again
    extern const float BOSS_TRICK_ROOM_OVERLAY_ALPHA;                                                                       // opacity of the purple tint drawn over the screen during Trick Room
    extern const float BOSS_TRICK_ROOM_WARNING_DURATION_SECONDS;                                                            // how long the "TRICK ROOM!" banner stays on-screen
    extern const float BOSS_TRICK_ROOM_FADE_DURATION_SECONDS;                                                               // how long the purple tint takes to fade out after Trick Room ends
}
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      BossSystem
@brief          Manages the entire boss lifecycle: spawn timing, movement, kill word pool injection,
                minion management, visual feedback, and defeat cleanup.

All public functions match the call sites in game.cpp. All internal state lives in an anonymous
namespace inside Boss.cpp.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossSystem
{
    /*
    @function       Boss_Init
    @params         meshes (Data::Meshes&), bank (const Data::WordBank&)
    @brief          Loads the boss texture, builds the boss sprite mesh, and resets all boss state.

    This function loads Assets/bossEnemy.png, builds a 1 row sprite quad sized for the boss, and
    sets every state variable (timers, counters, flags) back to its default inactive value.
    It must be called once after EnemyManage_LoadMeshes so the base meshes are already present.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void boss_init(Data::Meshes& meshes, const Data::WordBank& bank);

    /*
    @function       Boss_Free
    @params         meshes (Data::Meshes&), free_slots (std::deque<int>&), pool (std::vector<Data::Enemy>&)
    @brief          Unloads the boss texture and frees the boss sprite mesh without touching other meshes.

    This function releases exactly the resources that Boss_Init loaded. If the boss was mid wave when
    called, the reserved kill word slot is marked inactive and returned to free_slots so no orphaned
    active slot remains on restart or level exit. Safe to call even if Boss_Init was never reached
    because null checks are performed before freeing.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void boss_free(Data::Meshes& meshes, std::deque<int>& free_slots, std::vector<Data::Enemy>& pool);

    /*
    @function       Boss_Update
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), dt (float), boss_spawning_blocked (bool&), is_random_spawning_enabled (bool)
    @brief          Full per frame boss tick: timing, state transitions, movement, minion management,
                    kill word pool injection and removal, hit feedback, and flash attack.

    This function owns the 60 second trigger, the pool clear wait, the cutscene countdown, boss
    movement, minion spawning, targeting lock logic (remove and re inject kill word slot), hit tint
    decay, and flash attack timing. It sets boss_spawning_blocked to true from the moment the 60 second
    mark is hit until the boss is fully defeated, so the caller can gate regular enemy spawning.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void boss_update(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, float dt, bool& boss_spawning_blocked, bool is_random_spawning_enabled);

    /*
    @function       Boss_Draw
    @params         meshes (const Data::Meshes&)
    @brief          Renders the boss sprite with the current hit tint color and the kill word label.

    This function draws the boss sprite at the current boss position using AEGfxTextureSet and
    Draw_DrawMeshAt, then draws the kill word centered above the sprite via AEGfxPrint. A red tint
    is applied while the hit tint timer is active and reverts to white otherwise. The BOSS X of Y
    progress counter is drawn while the boss is alive. Nothing is drawn when the boss is inactive
    or in the dead state.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void boss_draw(const Data::Meshes& meshes);

    /*
    @function       Boss_DrawFlashOverlay
    @params         meshes (const Data::Meshes&)
    @brief          Renders the white full screen flash overlay if a flash attack is in progress.

    This function draws a full window white rectangle scaled to cover the entire screen, with alpha
    derived from the current flash timer. Nothing is drawn when no flash is active.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void boss_draw_flash_overlay(const Data::Meshes& meshes);

    /*
    @function       Boss_GetKillWordSlot
    @brief          Returns the pool slot index reserved for the boss kill word enemy, or -1 if none.

    This function gives read only access to the slot index the boss has reserved in the enemy pool.
    A return value of -1 means the boss does not currently hold a slot, which is the case when the
    boss is inactive or has not yet been triggered.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int boss_get_kill_word_slot();

    /*
    @function       Boss_GetKillWordAnchorY
    @brief          Returns the world-space Y position the kill word slot must always be pinned to.

    This value is computed once in Boss_Init from the boss sprite scale and position constants so
    there is a single source of truth for the kill word anchor Y. Both Boss_Update and
    GameEnemyBoss::Update read this value instead of recomputing independently, eliminating any
    risk of the two sites diverging and causing the kill word to appear at the wrong height.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float boss_get_kill_word_anchor_y();

    /*
    @function       Boss_IsActive
    @brief          Returns true while the boss is alive (CUTSCENE or ACTIVE state).

    This function checks whether the boss is currently on screen and part of gameplay. It returns
    true during both the entrance cutscene and the main fight phase. Callers use this to pause normal
    enemy spawning and gameplay logic while the boss is present.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool boss_is_active();

    /*
    @function       Boss_ForceSpawn
    @brief          Immediately triggers the boss spawn sequence, bypassing the 60 second timer.

    This function sets the cycle timer to the trigger threshold so the next Boss_Update call
    transitions to WAITING and then proceeds normally through pool clear, cutscene, and ACTIVE states.
    Nothing happens if the boss is already active or in the process of spawning.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void boss_force_spawn();

    /*
    @function       Boss_ForceKill
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&)
    @brief          Immediately kills the boss and triggers the death animation. Debug only.

    Deactivates the kill word slot, returns it to the free list, and switches the boss directly
    to the DEAD state so the death animation plays and Boss_WasJustDefeated fires normally.
    Does nothing if the boss is not in the ACTIVE state.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool boss_force_kill(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots);

    /*
    @function       Boss_WasJustDefeated
    @brief          Returns true for exactly one call after the boss death animation finishes.

    The flag is set the moment the boss transitions from DEAD to INACTIVE, meaning the full death
    animation has played through. Reading this function also clears the flag so subsequent calls
    return false until the next boss is defeated. game.cpp uses this single-shot signal to trigger
    the upgrade card screen at exactly the right moment without polling every frame.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool boss_was_just_defeated();

    /*
    @function       Boss_ForceTrickRoom
    @brief          Debug only. Immediately triggers Trick Room, bypassing the cooldown.
                    Returns false only if the boss is not in the ACTIVE fight state.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool boss_force_trick_room();

    /*
    @function       Boss_IsTrickRoomActive
    @brief          Returns true while the Trick Room reverse-typing phase is running.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool boss_is_trick_room_active();

} // BossSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/