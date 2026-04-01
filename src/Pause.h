#pragma once
#include "Enemy.h"
#include <AEEngine.h>

/*!
@file           Pause.h
@author         Jeremy Lim
@brief          Manages the pause menu UI, popup logic, and handles user interactions while the game is paused.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace UiPauseMenu
{
    enum PopupAction
    {
        POPUP_MAIN_MENU,
        POPUP_RESTART,
        POPUP_QUIT
    };

    bool is_popup_open();
    bool is_settings_open();
    void open_popup(PopupAction action);
    void close_popup();
    void ui_draw_confirmation_popup(AEGfxVertexList* bg_mesh, bool* p_is_paused = nullptr);
    void check_paused(bool& is_paused, bool& is_dead, AEGfxVertexList* bg_mesh);
    void ui_draw_pause_menu(bool& is_paused, AEGfxVertexList* bg_mesh);
}