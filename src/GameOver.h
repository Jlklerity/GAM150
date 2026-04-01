#pragma once
#include "Enemy.h"
#include <AEEngine.h>

/*!
@file           GameOver.cpp
@authors        Peng Fei, Jeremy Lim
@brief          Handles the game over screen UI, including displaying options, writing to the leaderboard, and menu navigation.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

namespace UiGameOver
{
    void cleanup();

    void check_if_dead(bool& is_dead, bool& is_game_over, bool& open_leaderboard,
        AEGfxVertexList* bg_mesh, std::string temp_name, const Data::Score& score);

    void ui_game_over_menu(bool& is_game_over, const Data::Score& score,
        bool& open_leaderboard, AEGfxVertexList* bg_mesh, bool& is_dead);

    void draw_timeout_screen(const Data::Score& score, AEGfxVertexList* bg_mesh, bool& open_leaderboard);
}