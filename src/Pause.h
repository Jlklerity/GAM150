#pragma once
#include "Enemy.h"
#include <AEEngine.h>

namespace UiPauseMenu
{
	bool IsSettingsOpen();
	void Check_Paused(bool& isPaused, bool& isDead, AEGfxVertexList* bgMesh);
	void Ui_DrawPauseMenu(bool& isPaused, AEGfxVertexList* bgMesh);
}
