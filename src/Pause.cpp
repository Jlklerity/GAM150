/*!
@file           Pause.cpp
@author         Jeremy Lim
@brief          Manages the pause menu UI, popup logic, and handles user interactions while the game is paused.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

#include "pch.h"
#include "Pause.h"
#include "ButtonTween.h"

/*!
@namespace      UiPauseMenu
@brief          Contains the internal state for the pause menu overlays, active popups, and button tweens.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace UiPauseMenu
{
    static bool         s_in_settings = false;
    static bool         s_ignore_next_click = false;
    static bool         s_in_confirm_exit = false;
    static PopupAction  s_current_popup_action = POPUP_MAIN_MENU;

    static ButtonTween s_tween_resume;
    static ButtonTween s_tween_restart;
    static ButtonTween s_tween_settings;
    static ButtonTween s_tween_menu;
    static ButtonTween s_tween_yes;
    static ButtonTween s_tween_no;

    bool is_settings_open()
    {
        return s_in_settings;
    }

    /*!
    @function       open_popup
    @brief          Triggers a specific confirmation popup (e.g., Restart, Main Menu) and locks background UI interaction.
    @param          action (PopupAction) The enum specifying which popup to open.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void open_popup(PopupAction action)
    {
        s_in_confirm_exit = true;
        s_current_popup_action = action;
        s_ignore_next_click = true;
    }

    void close_popup()
    {
        s_in_confirm_exit = false;
        s_ignore_next_click = true;
    }

    bool is_popup_open()
    {
        return s_in_confirm_exit;
    }

    /*!
    @function       ui_draw_pause_menu
    @brief          Renders the main pause menu panel, buttons, and handles hover/click interactions.
    @param          is_paused (bool&) Reference to the flag determining if the game is paused.
    @param          bg_mesh (AEGfxVertexList*) Pointer to the quad mesh used for UI backgrounds.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void ui_draw_pause_menu(bool& is_paused, AEGfxVertexList* bg_mesh)
    {
        if (s_in_confirm_exit)
        {
            ui_draw_confirmation_popup(bg_mesh, &is_paused);
            return;
        }

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();
        float screen_half_w = win_w * 0.5f;
        float screen_half_h = win_h * 0.5f;

        // --- Dark overlay ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.75f);
        AEGfxSetColorToMultiply(0.05f, 0.03f, 0.08f, 1.0f);
        mesh_renderer(bg_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);

        if (Graphics::g_font_id < 0) return;

        // --- Gold bordered panel ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        mesh_renderer(bg_mesh, 0.0f, -0.05f * screen_half_h, 440.0f, 780.0f, 0.0f);

        AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f);
        mesh_renderer(bg_mesh, 0.0f, -0.05f * screen_half_h, 428.0f, 768.0f, 0.0f);

        // --- Button config ---
        constexpr float BTN_W = 340.0f;
        constexpr float BTN_H = 75.0f;
        constexpr float BTN_X = 0.0f;

        constexpr float RESUME_Y = 0.25f;
        constexpr float RESTART_Y = -0.03f;
        constexpr float SETTINGS_Y = -0.31f;
        constexpr float MENU_Y = -0.59f;

        constexpr float BORDER_SIZE = 5.0f;

        s32 mx_hover, my_hover;
        AEInputGetCursorPosition(&mx_hover, &my_hover);
        float wx_hover = (float)mx_hover - screen_half_w;
        float wy_hover = screen_half_h - (float)my_hover;
        float dt_p = (float)AEFrameRateControllerGetFrameTime();
        bool click_p = AEInputCheckTriggered(AEVK_LBUTTON);

        auto hover_btn = [&](float cy)
            {
                float wy = cy * screen_half_h;
                return (wx_hover >= (BTN_X - BTN_W * 0.5f) && wx_hover <= (BTN_X + BTN_W * 0.5f) &&
                    wy_hover >= (wy - BTN_H * 0.5f) && wy_hover <= (wy + BTN_H * 0.5f));
            };

        button_tween_update(s_tween_resume, hover_btn(RESUME_Y), hover_btn(RESUME_Y) && click_p, dt_p);
        button_tween_update(s_tween_restart, hover_btn(RESTART_Y), hover_btn(RESTART_Y) && click_p, dt_p);
        button_tween_update(s_tween_settings, hover_btn(SETTINGS_Y), hover_btn(SETTINGS_Y) && click_p, dt_p);
        button_tween_update(s_tween_menu, hover_btn(MENU_Y), hover_btn(MENU_Y) && click_p, dt_p);

        auto draw_button = [&](float cx, float world_y, const char* label, ButtonTween& tween)
            {
                float sw = tween.scale;
                float scaled_w = BTN_W * sw;
                float scaled_h = BTN_H * sw;

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                mesh_renderer(bg_mesh, cx, world_y, scaled_w + BORDER_SIZE * 2, scaled_h + BORDER_SIZE * 2, 0.0f);

                AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f);
                mesh_renderer(bg_mesh, cx, world_y, scaled_w, scaled_h, 0.0f);

                float tw, th;
                AEGfxGetPrintSize(Graphics::g_font_id, label, 0.65f, &tw, &th);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

                float ndc_y = world_y / screen_half_h;
                AEGfxPrint(Graphics::g_font_id, label,
                    cx / screen_half_w - tw * 0.5f,
                    ndc_y - 0.035f,
                    0.65f, 1.0f, 0.92f, 0.6f, 1.0f);
            };

        // --- PAUSED title ---
        {
            const char* title = "PAUSED";
            float tw, th;
            AEGfxGetPrintSize(Graphics::g_font_id, title, 1.1f, &tw, &th);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            // Shadow
            AEGfxPrint(Graphics::g_font_id, title, -tw * 0.5f + 0.005f, 0.52f, 1.1f, 0.0f, 0.0f, 0.0f, 1.0f);
            // Gold
            AEGfxPrint(Graphics::g_font_id, title, -tw * 0.5f, 0.53f, 1.1f, 1.0f, 0.82f, 0.1f, 1.0f);
        }

        // --- Divider ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        mesh_renderer(bg_mesh, 0.0f, 0.45f * screen_half_h, 360.0f, 3.0f, 0.0f);

        draw_button(BTN_X, RESUME_Y * screen_half_h, "RESUME", s_tween_resume);
        draw_button(BTN_X, RESTART_Y * screen_half_h, "RESTART", s_tween_restart);
        draw_button(BTN_X, SETTINGS_Y * screen_half_h, "SETTINGS", s_tween_settings);
        draw_button(BTN_X, MENU_Y * screen_half_h, "MAIN MENU", s_tween_menu);

        // ========================================================================
        // EXIT CONFIRMATION POPUP OVERLAY (inside pause menu)
        // ========================================================================
        if (s_in_confirm_exit)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(0.85f);
            AEGfxSetColorToMultiply(0.02f, 0.01f, 0.04f, 1.0f);
            mesh_renderer(bg_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);

            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
            mesh_renderer(bg_mesh, 0.0f, 0.0f, 610.0f, 310.0f, 0.0f);

            AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f);
            mesh_renderer(bg_mesh, 0.0f, 0.0f, 600.0f, 300.0f, 0.0f);

            constexpr float CONF_BTN_W = 160.0f;
            constexpr float CONF_BTN_H = 65.0f;
            constexpr float YES_X = -130.0f;
            constexpr float NO_X = 130.0f;
            constexpr float CONF_BTN_Y = -60.0f;

            auto draw_conf_button = [&](float cx, float cy, ButtonTween& tween)
                {
                    float sw = tween.scale;
                    AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                    mesh_renderer(bg_mesh, cx, cy, (CONF_BTN_W + BORDER_SIZE * 2) * sw, (CONF_BTN_H + BORDER_SIZE * 2) * sw, 0.0f);
                    AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f);
                    mesh_renderer(bg_mesh, cx, cy, CONF_BTN_W * sw, CONF_BTN_H * sw, 0.0f);
                };

            {
                float dt_conf = (float)AEFrameRateControllerGetFrameTime();
                bool click_conf = AEInputCheckTriggered(AEVK_LBUTTON);
                s32 mx_c, my_c;
                AEInputGetCursorPosition(&mx_c, &my_c);
                float wx_c = (float)mx_c - screen_half_w;
                float wy_c = screen_half_h - (float)my_c;
                bool h_yes = (wx_c >= (YES_X - CONF_BTN_W * 0.5f) && wx_c <= (YES_X + CONF_BTN_W * 0.5f) &&
                    wy_c >= (CONF_BTN_Y - CONF_BTN_H * 0.5f) && wy_c <= (CONF_BTN_Y + CONF_BTN_H * 0.5f));
                bool h_no = (wx_c >= (NO_X - CONF_BTN_W * 0.5f) && wx_c <= (NO_X + CONF_BTN_W * 0.5f) &&
                    wy_c >= (CONF_BTN_Y - CONF_BTN_H * 0.5f) && wy_c <= (CONF_BTN_Y + CONF_BTN_H * 0.5f));
                button_tween_update(s_tween_yes, h_yes, h_yes && click_conf, dt_conf);
                button_tween_update(s_tween_no, h_no, h_no && click_conf, dt_conf);
            }

            draw_conf_button(YES_X, CONF_BTN_Y, s_tween_yes);
            draw_conf_button(NO_X, CONF_BTN_Y, s_tween_no);

            const char* prompt_text = "EXIT TO MAIN MENU?";
            if (s_current_popup_action == POPUP_RESTART)
                prompt_text = (g_game_state_curr == GS_TUTORIAL) ? "RESTART TUTORIAL?" : "RESTART GAME?";

            const char* warning_text = "YOU WILL LOSE PROGRESS";
            const char* yes_text = "YES";
            const char* no_text = "NO";

            float pw{}, ph{}, ww{}, wh{}, yw{}, yh{}, nw{}, nh{};
            AEGfxGetPrintSize(Graphics::g_font_id, prompt_text, 0.8f, &pw, &ph);
            if (g_game_state_curr != GS_TUTORIAL)
                AEGfxGetPrintSize(Graphics::g_font_id, warning_text, 0.5f, &ww, &wh);
            AEGfxGetPrintSize(Graphics::g_font_id, yes_text, 0.65f, &yw, &yh);
            AEGfxGetPrintSize(Graphics::g_font_id, no_text, 0.65f, &nw, &nh);

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            AEGfxPrint(Graphics::g_font_id, prompt_text, -(pw * 0.5f), (80.0f / screen_half_h) - (ph * 0.5f), 0.8f, 1.0f, 0.82f, 0.1f, 1.0f);
            if (g_game_state_curr != GS_TUTORIAL)
                AEGfxPrint(Graphics::g_font_id, warning_text, -(ww * 0.5f), (30.0f / screen_half_h) - (wh * 0.5f), 0.5f, 1.0f, 0.4f, 0.4f, 1.0f);
            AEGfxPrint(Graphics::g_font_id, yes_text, (YES_X / screen_half_w) - (yw * 0.5f), (CONF_BTN_Y / screen_half_h) - (yh * 0.5f), 0.65f, 1.0f, 0.92f, 0.6f, 1.0f);
            AEGfxPrint(Graphics::g_font_id, no_text, (NO_X / screen_half_w) - (nw * 0.5f), (CONF_BTN_Y / screen_half_h) - (nh * 0.5f), 0.65f, 1.0f, 0.92f, 0.6f, 1.0f);

            if (s_ignore_next_click)
            {
                s_ignore_next_click = false;
            }
            else if (AEInputCheckTriggered(AEVK_ESCAPE))
            {
                SoundManager::play_click_btn_sfx();
                s_in_confirm_exit = false;
                s_ignore_next_click = true;
            }
            else if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                s32 mx, my;
                AEInputGetCursorPosition(&mx, &my);
                float wx = (float)mx - screen_half_w;
                float wy = screen_half_h - (float)my;

                float half_conf_w = CONF_BTN_W * 0.5f;
                float half_conf_h = CONF_BTN_H * 0.5f;

                if (wx >= (YES_X - half_conf_w) && wx <= (YES_X + half_conf_w) &&
                    wy >= (CONF_BTN_Y - half_conf_h) && wy <= (CONF_BTN_Y + half_conf_h))
                {
                    SoundManager::play_click_btn_sfx();
                    is_paused = false;
                    if (s_current_popup_action == POPUP_RESTART)
                    {
                        g_game_state_next = GS_RESTART;
                    }
                    else
                    {
                        std::string temp_name = "";
                        reset_player_name(temp_name);
                        g_game_state_next = GS_MAIN_MENU;
                    }
                    s_in_confirm_exit = false;
                }
                else if (wx >= (NO_X - half_conf_w) && wx <= (NO_X + half_conf_w) &&
                    wy >= (CONF_BTN_Y - half_conf_h) && wy <= (CONF_BTN_Y + half_conf_h))
                {
                    SoundManager::play_click_btn_sfx();
                    s_in_confirm_exit = false;
                    s_ignore_next_click = true;
                }
            }
            return;
        }

        // ========================================================================
        // Input Handling (Normal Pause Menu)
        // ========================================================================
        if (s_ignore_next_click)
        {
            s_ignore_next_click = false;
        }
        else if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - screen_half_w;
            float wy = screen_half_h - (float)my;

            float half_w = BTN_W * 0.5f;
            float half_h = BTN_H * 0.5f;

            if (wx >= (BTN_X - half_w) && wx <= (BTN_X + half_w) &&
                wy >= ((RESUME_Y * screen_half_h) - half_h) && wy <= ((RESUME_Y * screen_half_h) + half_h))
            {
                SoundManager::play_click_btn_sfx();
                is_paused = false;
            }

            if (wx >= (BTN_X - half_w) && wx <= (BTN_X + half_w) &&
                wy >= ((RESTART_Y * screen_half_h) - half_h) && wy <= ((RESTART_Y * screen_half_h) + half_h))
            {
                SoundManager::play_click_btn_sfx();
                open_popup(POPUP_RESTART);
            }

            if (wx >= (BTN_X - half_w) && wx <= (BTN_X + half_w) &&
                wy >= ((SETTINGS_Y * screen_half_h) - half_h) && wy <= ((SETTINGS_Y * screen_half_h) + half_h))
            {
                SoundManager::play_click_btn_sfx();
                s_in_settings = true;
            }

            if (wx >= (BTN_X - half_w) && wx <= (BTN_X + half_w) &&
                wy >= ((MENU_Y * screen_half_h) - half_h) && wy <= ((MENU_Y * screen_half_h) + half_h))
            {
                SoundManager::play_click_btn_sfx();
                open_popup(POPUP_MAIN_MENU);
            }
        }
    }

    /*!
    @function       ui_draw_confirmation_popup
    @brief          Renders the YES/NO confirmation prompt over the pause menu and processes the respective click responses.
    @param          bg_mesh (AEGfxVertexList*) Pointer to the quad mesh used for UI backgrounds.
    @param          p_is_paused (bool*) Optional pointer to the paused flag to resume the game upon confirmation.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void ui_draw_confirmation_popup(AEGfxVertexList* bg_mesh, bool* p_is_paused)
    {
        if (!s_in_confirm_exit) return;

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();
        float screen_half_w = win_w * 0.5f;
        float screen_half_h = win_h * 0.5f;
        constexpr float BORDER_SIZE = 5.0f;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.85f);
        AEGfxSetColorToMultiply(0.02f, 0.01f, 0.04f, 1.0f);
        mesh_renderer(bg_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);

        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        mesh_renderer(bg_mesh, 0.0f, 0.0f, 610.0f, 310.0f, 0.0f);
        AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f);
        mesh_renderer(bg_mesh, 0.0f, 0.0f, 600.0f, 300.0f, 0.0f);

        constexpr float CONF_BTN_W = 160.0f;
        constexpr float CONF_BTN_H = 65.0f;
        constexpr float YES_X = -130.0f;
        constexpr float NO_X = 130.0f;
        constexpr float CONF_BTN_Y = -60.0f;

        float dt_conf = (float)AEFrameRateControllerGetFrameTime();
        bool click_conf = AEInputCheckTriggered(AEVK_LBUTTON);

        s32 mx_c, my_c;
        AEInputGetCursorPosition(&mx_c, &my_c);
        float wx_c = (float)mx_c - screen_half_w;
        float wy_c = screen_half_h - (float)my_c;

        bool h_yes = (wx_c >= (YES_X - CONF_BTN_W * 0.5f) && wx_c <= (YES_X + CONF_BTN_W * 0.5f) &&
            wy_c >= (CONF_BTN_Y - CONF_BTN_H * 0.5f) && wy_c <= (CONF_BTN_Y + CONF_BTN_H * 0.5f));
        bool h_no = (wx_c >= (NO_X - CONF_BTN_W * 0.5f) && wx_c <= (NO_X + CONF_BTN_W * 0.5f) &&
            wy_c >= (CONF_BTN_Y - CONF_BTN_H * 0.5f) && wy_c <= (CONF_BTN_Y + CONF_BTN_H * 0.5f));

        button_tween_update(s_tween_yes, h_yes, h_yes && click_conf, dt_conf);
        button_tween_update(s_tween_no, h_no, h_no && click_conf, dt_conf);

        auto draw_conf_button = [&](float cx, float cy, ButtonTween& tween)
            {
                float sw = tween.scale;
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                mesh_renderer(bg_mesh, cx, cy, (CONF_BTN_W + BORDER_SIZE * 2) * sw, (CONF_BTN_H + BORDER_SIZE * 2) * sw, 0.0f);
                AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f);
                mesh_renderer(bg_mesh, cx, cy, CONF_BTN_W * sw, CONF_BTN_H * sw, 0.0f);
            };

        draw_conf_button(YES_X, CONF_BTN_Y, s_tween_yes);
        draw_conf_button(NO_X, CONF_BTN_Y, s_tween_no);

        const char* prompt_text = "";
        const char* warning_text = "YOU WILL LOSE PROGRESS";

        if (s_current_popup_action == POPUP_QUIT)    prompt_text = "DO YOU WISH TO EXIT GAME?";
        else if (s_current_popup_action == POPUP_RESTART) prompt_text = (g_game_state_curr == GS_TUTORIAL) ? "RESTART TUTORIAL?" : "RESTART GAME?";
        else                                               prompt_text = "EXIT TO MAIN MENU?";

        float pw{}, ph{}, ww{}, wh{}, yw{}, yh{}, nw{}, nh{};
        AEGfxGetPrintSize(Graphics::g_font_id, prompt_text, 0.8f, &pw, &ph);

        if (s_current_popup_action != POPUP_QUIT && g_game_state_curr != GS_TUTORIAL)
            AEGfxGetPrintSize(Graphics::g_font_id, warning_text, 0.5f, &ww, &wh);

        AEGfxGetPrintSize(Graphics::g_font_id, "YES", 0.65f, &yw, &yh);
        AEGfxGetPrintSize(Graphics::g_font_id, "NO", 0.65f, &nw, &nh);

        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        float prompt_y = (s_current_popup_action == POPUP_QUIT || g_game_state_curr == GS_TUTORIAL) ? 60.0f : 80.0f;
        AEGfxPrint(Graphics::g_font_id, prompt_text, -(pw * 0.5f), (prompt_y / screen_half_h) - (ph * 0.5f), 0.8f, 1.0f, 0.82f, 0.1f, 1.0f);

        if (s_current_popup_action != POPUP_QUIT && g_game_state_curr != GS_TUTORIAL)
            AEGfxPrint(Graphics::g_font_id, warning_text, -(ww * 0.5f), (30.0f / screen_half_h) - (wh * 0.5f), 0.5f, 1.0f, 0.4f, 0.4f, 1.0f);

        AEGfxPrint(Graphics::g_font_id, "YES", (YES_X / screen_half_w) - (yw * 0.5f), (CONF_BTN_Y / screen_half_h) - (yh * 0.5f), 0.65f, 1.0f, 0.92f, 0.6f, 1.0f);
        AEGfxPrint(Graphics::g_font_id, "NO", (NO_X / screen_half_w) - (nw * 0.5f), (CONF_BTN_Y / screen_half_h) - (nh * 0.5f), 0.65f, 1.0f, 0.92f, 0.6f, 1.0f);

        if (s_ignore_next_click)
        {
            s_ignore_next_click = false;
        }
        else if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            SoundManager::play_click_btn_sfx();
            s_in_confirm_exit = false;
            s_ignore_next_click = true;
        }
        else if (click_conf)
        {
            if (h_yes)
            {
                SoundManager::play_click_btn_sfx();
                if (p_is_paused) *p_is_paused = false;

                if (s_current_popup_action == POPUP_QUIT)    g_game_state_next = GS_QUIT;
                else if (s_current_popup_action == POPUP_RESTART) g_game_state_next = GS_RESTART;
                else
                {
                    std::string temp_name = "";
                    reset_player_name(temp_name);
                    g_game_state_next = GS_MAIN_MENU;
                }
                s_in_confirm_exit = false;
            }
            else if (h_no)
            {
                SoundManager::play_click_btn_sfx();
                s_in_confirm_exit = false;
                s_ignore_next_click = true;
            }
        }
    }

    void check_paused(bool& is_paused, bool& is_dead, AEGfxVertexList* bg_mesh)
    {
        if (is_paused && !is_dead)
        {
            if (s_in_settings)
            {
                if (SysSettings::display_settings() == false)
                {
                    s_in_settings = false;
                    s_ignore_next_click = true;
                }
                else return;
            }

            ui_draw_pause_menu(is_paused, bg_mesh);
        }
    }
}