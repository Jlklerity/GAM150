#pragma once
#include "Enemy.h"
#include <AEEngine.h>

namespace UiPauseMenu
{
    enum PopupAction
    {
        POPUP_MAIN_MENU,
        POPUP_RESTART,
        POPUP_QUIT
    };

    bool IsPopupOpen();
    bool IsSettingsOpen();
    void OpenPopup(PopupAction action);
    void ClosePopup();
    void Ui_DrawConfirmationPopup(AEGfxVertexList* bg_mesh, bool* p_is_paused = nullptr);
    void Check_Paused(bool& is_paused, bool& is_dead, AEGfxVertexList* bg_mesh);
    void Ui_DrawPauseMenu(bool& is_paused, AEGfxVertexList* bg_mesh);
}