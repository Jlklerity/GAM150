#pragma once
#include <vector>
#include <string>

void Typing_Init();
void Typing_Update();
void Typing_Render();
void Typing_Exit();

// forward declare to avoid heavy includes if you want


namespace Data { struct Enemy; }
namespace Data { struct Score; }

namespace UiSystem
{
    void Ui_DrawScoreAndTyped(const Data::Score& score);
    void Ui_DrawPauseMenu(bool& isPaused, AEGfxVertexList* bgMesh);
    void Ui_GameOverMenu(bool& isGameOver, const Data::Score& score, bool& openLeaderboard, AEGfxVertexList* bgMesh, bool& isDead);
}

void Typing_UpdateAndApplyToEnemies(std::vector<Data::Enemy>& enemy_pool, Data::Score& score);



