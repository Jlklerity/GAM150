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
    // stores references to all game state the commands need, must be called once in Game_Init()
    void Init(
        bool& is_enemy_area_box_visible,
        bool& is_random_spawning_enabled,
        bool& is_difficulty_debug_visible,
        Data::Health& health,
        std::vector<Data::Enemy>& enemy_pool,
        std::deque<int>& free_slot_queue,
        Data::EnemyArea& enemy_area,
        Data::WordBank& word_bank
    );

    // handles the '/' toggle and all keyboard input, call every frame BEFORE the gameplay freeze check
    void Update();

    // draws the input bar and scrollable history panel, call every frame AFTER all other rendering
    void Render();

    // frees mesh resources, call in Game_Exit()
    void Exit();

    // returns true while the console is open, use this to freeze gameplay logic
    bool IsOpen();

} // namespace DebugConsole