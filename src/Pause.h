#pragma once
#include "Enemy.h"
#include <AEEngine.h>

namespace UiPauseMenu
{
	enum PopupAction {
		POPUP_MAIN_MENU,
		POPUP_RESTART,
		POPUP_QUIT
	};


	bool IsPopupOpen();
	bool IsSettingsOpen();
	bool IsPopupOpen();
	void OpenPopup(PopupAction action);
	void ClosePopup();
	void Ui_DrawConfirmationPopup(AEGfxVertexList* bgMesh, bool* pIsPaused = nullptr);
	void Check_Paused(bool& isPaused, bool& isDead, AEGfxVertexList* bgMesh);
	void Ui_DrawPauseMenu(bool& isPaused, AEGfxVertexList* bgMesh);
}
