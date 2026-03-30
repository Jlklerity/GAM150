#pragma once
#include "Enemy.h"
#include <AEEngine.h>

namespace UiGameOver
{
    void Exit();

    void Check_IfDead(bool& is_dead, bool& is_game_over, bool& open_leaderboard,
        AEGfxVertexList* bg_mesh, std::string temp_name, const Data::Score& score);

    void Ui_GameOverMenu(bool& is_game_over, const Data::Score& score,
        bool& open_leaderboard, AEGfxVertexList* bg_mesh, bool& is_dead);

    void DrawTimeoutScreen(const Data::Score& score, AEGfxVertexList* bg_mesh, bool& open_leaderboard);
}