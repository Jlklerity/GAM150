#pragma once
/*!
@file           DebugConsole.h
@brief          Minecraft-style debug console overlay for the typing game.

Provides a self-contained console that can be toggled with '/' during gameplay.
While open, the game is fully frozen. Commands map to all existing debug functions.

Integration in game.cpp:
    game_init()   -> DebugConsole::init(...)
    game_update() -> DebugConsole::update() and DebugConsole::render()
    game_exit()   -> DebugConsole::cleanup()

Freeze check:
    if (DebugConsole::is_open()) return; // skip gameplay logic
*/

#include <vector>
#include <deque>
#include <string>
#include "Enemy.h"

namespace DebugConsole
{
    // stores references to all game state the commands need, must be called once in game_init()
    void init(
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
    void update();

    // draws the input bar and scrollable history panel, call every frame AFTER all other rendering
    void render();

    // frees mesh resources, call in game_exit()
    void cleanup();

    // returns true while the console is open, use this to freeze gameplay logic
    bool is_open();

} // namespace DebugConsole