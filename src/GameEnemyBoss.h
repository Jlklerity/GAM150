/*!
@file           GameEnemyBoss.h
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#pragma once

#include "pch.h"

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
    void init(Data::Meshes& meshes, Data::WordBank& word_bank, Data::Health& health,
        Data::SpriteAnim& enemy_animation, Data::EnemyArea& enemy_area,
        std::vector<Data::Enemy>& enemy_pool, std::deque<int>& free_slot_queue,
        bool& boss_spawning_blocked);

    /*
    @function       Update
    @params         dt (float)
    @brief          Runs boss update, enemy spawning, enemy per-frame update, lane stacking,
                    and returns particles/impacts for newly killed or landed enemies.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void update(float dt, std::vector<Data::Enemy>& enemy_pool, std::deque<int>& free_slot_queue,
        Data::EnemyArea& enemy_area, Data::WordBank& word_bank,
        Data::Health& health, bool& boss_spawning_blocked,
        bool is_random_spawning_enabled, float& random_spawn_timer_seconds);

    /*
    @function       Free
    @brief          Releases enemy/boss owned runtime state and clears pool containers.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void free(Data::Meshes& meshes, std::vector<Data::Enemy>& enemy_pool,
            std::deque<int>& free_slot_queue, bool& boss_spawning_blocked);

    /*
    @function       Draw
    @brief          Draws enemy sprites, enemy words, the boss sprite, and all typed-word highlights.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void draw(const Data::Meshes& meshes, const std::vector<Data::Enemy>& enemy_pool,
        bool is_enemy_area_box_visible, const Data::EnemyArea& enemy_area);

} // GameEnemyBoss namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/
