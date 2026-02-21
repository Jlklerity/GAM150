#pragma once
/*!
@file           DebugConsole.h
@brief          Minecraft-style debug console overlay for the typing game.

Provides a self-contained console that can be toggled with '/' during gameplay.
While open, the game is fully frozen. Commands map to all existing debug functions.

Integration in game.cpp:
    Game_Init()   -> DebugConsole::Init(...)
    Game_Update() -> DebugConsole::Update() and DebugConsole::Render()
    Game_Exit()   -> DebugConsole::Exit()

Freeze check:
    if (DebugConsole::IsOpen()) return; // skip gameplay logic
*/

#include <vector>
#include <deque>
#include <string>
#include "Enemy.h"

namespace DebugConsole
{
    /*
    @function   Init
    @brief      Initialises the console. Must be called once in Game_Init().
                Stores references/pointers to all game state the console commands need.
    */
    void Init(
        bool&                       is_enemy_area_box_visible,
        bool&                       is_random_spawning_enabled,
        bool&                       is_difficulty_debug_visible,
        Data::Health&               health,
        std::vector<Data::Enemy>&   enemy_pool,
        std::deque<int>&            free_slot_queue,
        Data::EnemyArea&            enemy_area,
        Data::WordBank&             word_bank
    );

    /*
    @function   Update
    @brief      Handles console open/close toggle and all keyboard input while open.
                Call every frame BEFORE the gameplay freeze check.
    */
    void Update();

    /*
    @function   Render
    @brief      Draws the console overlay (input bar + scrollable history).
                Call every frame AFTER all other rendering so it sits on top.
    */
    void Render();

    /*
    @function   Exit
    @brief      Frees any mesh resources created by the console. Call in Game_Exit().
    */
    void Exit();

    /*
    @function   IsOpen
    @brief      Returns true while the console is open.
                Use this in game.cpp to freeze all gameplay logic:

                    DebugConsole::Update();
                    if (DebugConsole::IsOpen()) goto render; // or early-return pattern
    */
    bool IsOpen();

} // namespace DebugConsole
