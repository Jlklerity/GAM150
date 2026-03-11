#pragma once
#include "Enemy.h"
#include <AEEngine.h>

namespace UiGameOver
{
	void Check_IfDead(bool& isDead, bool& isGameOver, bool& openLeaderboard, AEGfxVertexList* bgMesh, 
					  std::string temp_name, const Data::Score& score);
	void Ui_GameOverMenu(bool& isGameOver, const Data::Score& score, bool& openLeaderboard, AEGfxVertexList* bgMesh, bool& isDead);

	void DrawTimeoutScreen(const Data::Score& score, AEGfxVertexList* bgMesh, bool& openLeaderboard);
}