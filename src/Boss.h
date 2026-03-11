/*!
@file           Boss.h
@author         (implementer)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#pragma once

#include <vector>
#include <deque>
#include <string>
#include "Enemy.h"

/*
@constants      Boss system tuning constants
@brief          Centralized configuration values that control boss behavior, pacing, and visual feedback.

This block defines every tuning knob for the boss system: spawn cadence, movement speed, pool limits,
word counts, sprite sizing, hit-flash duration, flash-attack parameters, minion fall speed, minion cap,
and cutscene timing.  Adjust values here without touching core boss logic.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossConfig
{
    // Spawn timing
    extern const float BOSS_SPAWN_INTERVAL_SECONDS;            // seconds between boss waves (60 s)

    // Movement
    extern const float BOSS_MOVE_SPEED;                        // horizontal movement speed in world-px/s

    // Defeat requirement
    extern const int   BOSS_WORDS_TO_DEFEAT;                   // words needed to defeat the boss (30)

    // Word lengths (fixed, not scaled by difficulty)
    extern const int   BOSS_WORD_MIN_LEN;                      // min boss kill-word / minion word length
    extern const int   BOSS_WORD_MAX_LEN;                      // max boss kill-word / minion word length

    // Sprite sizing
    extern const float BOSS_SPRITE_SCALE;                      // 1.2× normal enemy size
    extern const float BOSS_SPRITE_W_RATIO;                    // width-to-height ratio (landscape)

    // Hit-flash feedback
    extern const float BOSS_HIT_TINT_DURATION_SECONDS;         // how long the red tint lasts after a hit

    // Flash attack
    extern const float BOSS_FLASH_INTERVAL_MIN_SECONDS;        // minimum random interval between flash attacks
    extern const float BOSS_FLASH_INTERVAL_MAX_SECONDS;        // maximum random interval between flash attacks
    extern const float BOSS_FLASH_FADE_DURATION_SECONDS;       // time for the white overlay to fade to zero

    // Minions
    extern const int   BOSS_MINION_CAP;                        // max simultaneous minions
    extern const float BOSS_MINION_SPAWN_INTERVAL_SECONDS;     // interval between minion waves
    extern const int   BOSS_MINION_WAVE_SIZE;                  // minions spawned per wave

    // Cutscene
    extern const float BOSS_CUTSCENE_DURATION_SECONDS;         // how long the spawn cutscene lasts

    // Y position of the boss sprite (stationary on the landing line)
    extern const float BOSS_POSITION_Y;                        // boss sits at (or just above) landing line
}
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      BossSystem
@brief          Manages the entire boss lifecycle: spawn timing, movement, kill-word pool injection,
                minion management, visual feedback, and defeat/cleanup.

All public functions match the call sites in game.cpp.  All internal state lives in an anonymous
namespace inside Boss.cpp — nothing leaks through this header.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace BossSystem
{
    /*
    @function       Boss_Init
    @params         meshes (Data::Meshes&), bank (const Data::WordBank&)
    @brief          Loads the boss texture, builds the boss sprite mesh, and resets all boss state.

    @usage          Game_Init

    This function loads Assets/bossEnemy.png, builds a 1-row sprite quad sized for the boss, and
    initialises every state variable (timers, counters, flags) to their default "inactive" values.
    It must be called once after EnemyManage_LoadMeshes so the base meshes are already present.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Init(Data::Meshes& meshes, const Data::WordBank& bank);

    /*
    @function       Boss_Free
    @params         meshes (Data::Meshes&), free_slots (std::deque<int>&), pool (std::vector<Data::Enemy>&)
    @brief          Unloads the boss texture and frees the boss sprite mesh without touching other meshes.
                    Returns the reserved kill-word slot to free_slots (marked inactive first) if the boss
                    was mid-wave when called, preventing an orphaned active slot on restart or level exit.

    @usage          Game_Exit

    Releases exactly the resources that Boss_Init loaded.  Safe to call even if Boss_Init was never
    reached (null-checks before free).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Free(Data::Meshes& meshes, std::deque<int>& free_slots, std::vector<Data::Enemy>& pool);

    /*
    @function       Boss_Update
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&),
                    area (const Data::EnemyArea&), bank (const Data::WordBank&),
                    dt (float), boss_spawning_blocked (bool&)
    @brief          Full per-frame boss tick: timing, state transitions, movement, minion management,
                    kill-word pool injection/removal, hit feedback, and flash attack.

    @usage          Game_Update — must be called BEFORE the regular spawn block and BEFORE
                    Typing_UpdateAndApplyToEnemies each frame.

    This function owns the 60-second trigger, the pool-clear wait, the cutscene countdown, boss
    movement, minion spawning, targeting-lock logic (remove/re-inject kill-word slot), hit-tint
    decay, and flash-attack timing.  It sets boss_spawning_blocked to true from the moment the 60 s
    mark is hit until the boss is fully defeated, so the caller can gate regular enemy spawning.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Update(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots,
        const Data::EnemyArea& area, const Data::WordBank& bank,
        float dt, bool& boss_spawning_blocked);

    /*
    @function       Boss_Draw
    @params         meshes (const Data::Meshes&)
    @brief          Renders the boss sprite with the current hit-tint colour and the kill-word label.

    @usage          Game_Update (render phase) — draw after enemy sprites, before UI overlays.

    Draws the boss sprite at the current boss position using AEGfxTextureSet + Draw_DrawMeshAt,
    then draws the kill-word centered above the sprite via AEGfxPrint.  Applies red tint while
    hit-tint timer is active; reverts to white otherwise.  Draws the BOSS X / 50 progress counter
    while boss is alive.  Does nothing when boss is inactive or in dead state.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_Draw(const Data::Meshes& meshes);

    /*
    @function       Boss_DrawFlashOverlay
    @brief          Renders the white full-screen flash overlay if a flash attack is in progress.

    @usage          Game_Update (render phase) — must be drawn LAST so it appears over everything.

    This function draws a full-window white rectangle scaled to cover the entire screen, with alpha
    derived from the current flash timer.  Does nothing when no flash is active.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_DrawFlashOverlay(const Data::Meshes& meshes);

    /*
    @function       Boss_GetKillWordSlot
    @brief          Returns the pool slot index reserved for the boss kill-word enemy, or -1 if none.

    @usage          Game_Update enemy loop — used to skip the kill-word slot in Enemy_Update so
                    Boss_Update retains full ownership of that slot's state transitions.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Boss_GetKillWordSlot();

    /*
    @function       Boss_IsActive
    @brief          Returns true while the boss is alive (CUTSCENE or ACTIVE state).

    @usage          game.cpp — used to suppress regular enemy spawning in the spawn block.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Boss_IsActive();

    /*
    @function       Boss_ForceSpawn
    @brief          Immediately triggers the boss spawn sequence, bypassing the 60-second timer.

    @usage          Debug / testing — call once from game.cpp (e.g. on a key press) to spawn the
                    boss instantly without waiting for the cycle timer.

    Sets the cycle timer to the trigger threshold so the next Boss_Update call transitions to
    WAITING and then proceeds normally through pool-clear, cutscene, and ACTIVE states.
    Does nothing if the boss is already active or in the process of spawning.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Boss_ForceSpawn();

} // BossSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/