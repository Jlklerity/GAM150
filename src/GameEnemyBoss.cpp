/*!
@file           GameEnemyBoss.cpp
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

#include "GameEnemyBoss.h"
#include "AEEngine.h"
#include "Typing.h"
#include "Particles.h"
/*
@namespace      GameEnemyBoss
@brief          Owns all enemy pool and boss state and exposes lifecycle functions to Game.cpp.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace GameEnemyBoss
{
    /*
    @function       Init
    @brief          Initialises the enemy engine, pool, meshes, and boss system.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Init(Data::Meshes& meshes, Data::WordBank& word_bank, Data::Health& health,
        Data::SpriteAnim& enemy_animation, Data::EnemyArea& enemy_area,
        std::vector<Data::Enemy>& enemy_pool, std::deque<int>& free_slot_queue,
        bool& boss_spawning_blocked)
    {
        // Enemy engine and pool
        EnemyManage::EnemyManage_InitEngine(word_bank);                                                                     // start engine and load words
        HealthSystem::Health_Init(health);                                                                                  // start game with full health
        meshes = EnemyManage::EnemyManage_LoadMeshes(enemy_animation);                                                      // load textures and meshes
        enemy_area.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f);                                  // place spawn line above the screen

        enemy_pool.resize((size_t)EnemyConfig::MAX_ENEMIES);                                                                // allocate pool to configured maximum size
        free_slot_queue.clear();                                                                                            // clear any previous free-slot indices

        for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)                                                                  // loop through pool slots
        {
            enemy_pool[(size_t)i].enemy_is_active = false;                                                                  // mark slot inactive
            free_slot_queue.push_back(i);                                                                                   // push slot index into free queue
        }

        // Boss
        boss_spawning_blocked = false;                                                                                      // make sure normal spawning is allowed at the start
        BossSystem::Boss_Init(meshes, word_bank);                                                                           // load boss assets and reset all boss state
    }

    /*
    @function       Update
    @params         dt (float)
    @brief          Runs boss update, enemy spawning, enemy per-frame update, lane stacking,
                    and returns particles/impacts for newly killed or landed enemies.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Update(float dt, std::vector<Data::Enemy>& enemy_pool, std::deque<int>& free_slot_queue,
        Data::EnemyArea& enemy_area, Data::WordBank& word_bank,
        Data::Health& health, bool& boss_spawning_blocked,
        bool is_random_spawning_enabled, float& random_spawn_timer_seconds)
    {
        if (boss_spawning_blocked)                                                                                          // check if the boss is preventing normal spawns
            random_spawn_timer_seconds = 0.0f;                                                                              // reset timer so enemies don't spawn immediately after boss dies

        if (is_random_spawning_enabled && !boss_spawning_blocked)                                                           // only spawn normal enemies when spawning is on and boss is not active
        {
            random_spawn_timer_seconds += dt;                                                                               // count up toward the next spawn
            float interval = DifficultySystem::Difficulty_GetSpawnIntervalSeconds();                                        // get seconds to wait between spawns at current difficulty

            if (random_spawn_timer_seconds >= interval)                                                                     // check if enough time has passed
            {
                random_spawn_timer_seconds = 0.0f;                                                                          // reset the timer after a spawn
                int roll = Utility::Utility_RandInt(0, 99);                                                                 // roll to decide which enemy type to spawn
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();                                      // get longest word allowed at current difficulty

                if (roll < 10)                                                                                              // 10% chance: spawn a friendly enemy
                    EnemySystem::Enemy_SpawnFriendlyFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.15f);
                else if (roll < 30)                                                                                         // 30% chance: spawn a shielded enemy
                    EnemySystem::Enemy_SpawnShieldedFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.05f);
                else                                                                                                        // 60% chance: spawn a normal enemy
                {
                    int len = DifficultySystem::Difficulty_GetRandomWordLength();                                           // pick random word length within current difficulty range
                    EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, len);
                }
            }
        }

        for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)                                                                  // loop through all pool slots
        {
            Data::Enemy& e = enemy_pool[(size_t)i];                                                                         // reference enemy slot
            bool was_active     = e.enemy_is_active;                                                                        // remember if alive before update
            bool was_damage_done = e.enemy_did_damage;                                                                      // remember if damage already applied

            if (BossSystem::Boss_GetKillWordSlot() >= 0 && i == BossSystem::Boss_GetKillWordSlot()) continue;               // skip the boss kill word slot

            EnemySystem::Enemy_Update(e, enemy_area, dt, health);                                                           // run movement, animation, landing, damage logic

            if (was_active && !e.enemy_is_active)                                                                           // check if enemy just became inactive this frame
            {
                free_slot_queue.push_back(i);                                                                               // return slot to the free list

                if (!was_damage_done)                                                                                       // enemy was killed by the player before landing
                    Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);                                    // spawn damage hit effect
                else                                                                                                        // enemy landed and dealt damage
                    Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);                 // spawn landing impact effect
            }
        }

        EnemySystem::Enemy_ResolveLaneStacking(enemy_pool, enemy_area);                                                     // fix spacing in lanes

        // Re-pin the boss kill word slot after lane stacking may have moved it
        int kw_slot = BossSystem::Boss_GetKillWordSlot();                                                                   // get the pool slot the boss is using for its kill word, or -1 if none
        if (kw_slot >= 0 && kw_slot < (int)enemy_pool.size())                                                               // only re-pin if the slot is valid
        {
            float boss_half = (EnemyConfig::ENEMY_SIZE_PX * BossConfig::BOSS_SPRITE_SCALE) * 0.5f;                          // half the boss sprite height in world space
            float normal_half = EnemyConfig::ENEMY_SIZE_PX * 0.5f;                                                          // half a normal enemy sprite height in world space
            float extra = 2.5f * (boss_half - normal_half);                                                                 // vertical offset to push the kill word text above the taller boss sprite
            enemy_pool[(size_t)kw_slot].enemy_position_y = BossConfig::BOSS_POSITION_Y + extra;                             // restore the correct Y so lane stacking cannot push the word out of place
        }
    }

    /*
    @function       Draw
    @brief          Draws enemy sprites, enemy words, the boss sprite, and all typed-word highlights.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Draw(const Data::Meshes& meshes, const std::vector<Data::Enemy>& enemy_pool,
        bool is_enemy_area_box_visible, const Data::EnemyArea& enemy_area)
    {
        if (is_enemy_area_box_visible)                                                                                      // check debug draw flag
        {
            UiSystem::Debug_DrawEnemyAreaBox(meshes, enemy_area);                                                           // draw enemy area box
            UiSystem::Debug_DrawEnemyLaneLines(meshes, enemy_area);                                                         // draw lane lines
        }

        for (int di = 0; di < (int)enemy_pool.size(); ++di)                                                                 // loop through enemies for sprite draw
        {
            if (BossSystem::Boss_GetKillWordSlot() >= 0 && di == BossSystem::Boss_GetKillWordSlot()) continue;              // skip the boss kill word slot
            EnemySystem::Enemy_Draw(meshes, enemy_pool[(size_t)di]);                                                        // draw enemy sprite
        }

        for (int di = 0; di < (int)enemy_pool.size(); ++di)                                                                 // loop through enemies for word draw
        {
            if (BossSystem::Boss_GetKillWordSlot() >= 0 && di == BossSystem::Boss_GetKillWordSlot()) continue;              // skip the boss kill word slot
            EnemySystem::Enemy_DrawWord(enemy_pool[(size_t)di]);                                                            // draw enemy word
        }

        BossSystem::Boss_Draw(meshes);                                                                                      // draw boss sprite, cutscene overlay, progress counter
        Typing_DrawEnemyWords(enemy_pool);                                                                                  // draw all words with green typed prefix and black suffix
    }

} // GameEnemyBoss namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/
