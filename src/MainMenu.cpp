/*!
    @file           MainMenu.cpp
    @author         Jeremy Lim
    @course         GAM150
    @brief          Handles the main menu state, UI rendering, sub-state transitions, and name entry.

    This file contains the logic for navigating the main menu, selecting game modes, handling
    the screen dropping transition effect, and capturing keyboard input for the player's name.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

#include "pch.h"
#include "MainMenu.h"
#include "ButtonTween.h"


namespace
{
    // ---------------------------------------------------------------------------
    // MESHES & TEXTURES
    // ---------------------------------------------------------------------------
    static AEGfxVertexList* s_mesh_logo = nullptr;
    static AEGfxVertexList* s_mesh_btn = nullptr;
    static AEGfxVertexList* s_mesh_exit_btn = nullptr;
    static AEGfxVertexList* s_mesh_settings_btn = nullptr;
    static AEGfxVertexList* s_mesh_left_arrow = nullptr;
    static AEGfxVertexList* s_mesh_right_arrow = nullptr;
    static AEGfxVertexList* s_mesh_leaderboard_btn = nullptr;
    static AEGfxVertexList* s_mesh_enter_name = nullptr;
    static AEGfxVertexList* s_mesh_back_name = nullptr;
    static AEGfxVertexList* s_mesh_tutorial_btn = nullptr;
    static AEGfxVertexList* s_mesh_credits_btn = nullptr;

    static AEGfxTexture* s_tex_play = nullptr;
    static AEGfxTexture* s_tex_shop = nullptr;
    static AEGfxTexture* s_tex_arrows = nullptr;
    static AEGfxTexture* s_tex_leaderboard = nullptr;
    static AEGfxTexture* s_tex_settings = nullptr;
    static AEGfxTexture* s_tex_logo = nullptr;
    static AEGfxTexture* s_tex_enter_name = nullptr;
    static AEGfxTexture* s_tex_back_name = nullptr;
    static AEGfxTexture* s_tex_tutorial = nullptr;
    static AEGfxTexture* s_tex_credits = nullptr;
    static AEGfxTexture* s_tex_exit = nullptr;

    // ---------------------------------------------------------------------------
    // WINDOW & CURSOR DATA
    // ---------------------------------------------------------------------------
    static float s_win_w = 0.0f;
    static float s_win_h = 0.0f;
    static float s_world_x = 0.0f;
    static float s_world_y = 0.0f;

    // ---------------------------------------------------------------------------
    // UI DIMENSIONS (CONSTANTS)
    // ---------------------------------------------------------------------------
    constexpr float PLAY_WIDTH = 240.0f;
    constexpr float PLAY_HEIGHT = 240.0f;
    constexpr float SHOP_WIDTH = 240.0f;
    constexpr float SHOP_HEIGHT = 240.0f;
    constexpr float EXIT_WIDTH = 240.0f;
    constexpr float EXIT_HEIGHT = 240.0f;
    constexpr float LEADERBOARD_WIDTH = 240.0f;
    constexpr float LEADERBOARD_HEIGHT = 80.0f;
    constexpr float PLAY_HITBOX_WIDTH = 200.0f;
    constexpr float PLAY_HITBOX_HEIGHT = 95.0f;
    constexpr float SHOP_HITBOX_WIDTH = 200.0f;
    constexpr float SHOP_HITBOX_HEIGHT = 95.0f;
    constexpr float LOGO_WIDTH = 1000.0f;
    constexpr float LOGO_HEIGHT = 600.0f;
    constexpr float EXITNAME_WIDTH = 200.0f;
    constexpr float EXITNAME_HEIGHT = 180.0f;
    constexpr float ENTERGAME_WIDTH = 200.0f;
    constexpr float ENTERGAME_HEIGHT = 180.0f;
    constexpr float ZENMODE_WIDTH = 240.0f;
    constexpr float ZENMODE_HEIGHT = 80.0f;
    constexpr float TIMEMODE_WIDTH = 240.0f;
    constexpr float TIMEMODE_HEIGHT = 80.0f;
    constexpr float SETTINGS_WIDTH = 96.0f;
    constexpr float SETTINGS_HEIGHT = 96.0f;
    constexpr float TUTORIAL_WIDTH = 240.0f;
    constexpr float TUTORIAL_HEIGHT = 240.0f;
    constexpr float TUTORIAL_HITBOX_WIDTH = 200.0f;
    constexpr float TUTORIAL_HITBOX_HEIGHT = 95.0f;
    constexpr float LEFTARROW_WIDTH = 80.0f;
    constexpr float LEFTARROW_HEIGHT = 80.0f;
    constexpr float RIGHTARROW_WIDTH = 80.0f;
    constexpr float RIGHTARROW_HEIGHT = 80.0f;
    constexpr float CREDITS_WIDTH = 240.0f;
    constexpr float CREDITS_HEIGHT = 240.0f;

    // ---------------------------------------------------------------------------
    // UI POSITIONS (DYNAMIC)
    // ---------------------------------------------------------------------------
    static float s_logo_y = 0.0f;
    static float s_play_btn_y = 0.0f;
    static float s_shop_btn_y = 0.0f;
    static float s_exit_btn_y = 0.0f;
    static float s_exitname_btn_y = 0.0f;
    static float s_exitname_btn_x = 0.0f;
    static float s_entergame_btn_y = 0.0f;
    static float s_entergame_btn_x = 0.0f;
    static float s_zenmode_btn_x = 0.0f;
    static float s_zenmode_btn_y = 0.0f;
    static float s_timemode_btn_x = 0.0f;
    static float s_timemode_btn_y = 0.0f;
    static float s_settings_btn_x = 0.0f;
    static float s_settings_btn_y = 0.0f;
    static float s_leftarrow_btn_x = 0.0f;
    static float s_leftarrow_btn_y = 0.0f;
    static float s_rightarrow_btn_x = 0.0f;
    static float s_rightarrow_btn_y = 0.0f;
    static float s_leaderboard_btn_x = 0.0f;
    static float s_leaderboard_btn_y = 0.0f;
    static float s_tutorial_btn_y = 0.0f;
    static float s_credits_btn_x = 0.0f;
    static float s_credits_btn_y = 0.0f;
    static float s_empty_name_warning_timer = 0.0f;
    static float s_empty_name_warning_x = 0.0f;
    static float s_empty_name_warning_y = 0.0f;

    // ---------------------------------------------------------------------------
    // STATE & LOGIC VARIABLES
    // ---------------------------------------------------------------------------
    static bool s_is_transitioning_to_name = false;
    static bool s_is_transitioning_back = false;
    static bool s_name_entered = false;
    static bool s_is_entering_name = false;
    static bool s_in_settings = false;
    static bool s_in_leaderboard = false;
    static bool s_cursor_visible = true;
    static bool s_transitioning_to_shop = false;
    static bool s_is_returning_from_shop = false;

    static std::string s_player_name = "";
    static std::string s_selected_mode = "";
    static std::string s_leaderboard_mode = "ZEN";

    static float s_ui_offset_x = 0.0f;
    static float s_cursor_timer = 0.0f;

    static bool s_is_zooming_to_game = false;

    // ---------------------------------------------------------------------------
    // BUTTON TWEENS
    // ---------------------------------------------------------------------------
    static ButtonTween s_tween_play;
    static ButtonTween s_tween_shop;
    static ButtonTween s_tween_tutorial;
    static ButtonTween s_tween_exit;
    static ButtonTween s_tween_settings;
    static ButtonTween s_tween_leaderboard;
    static ButtonTween s_tween_left_arrow;
    static ButtonTween s_tween_right_arrow;
    static ButtonTween s_tween_credits;
    static ButtonTween s_tween_enter_game;
    static ButtonTween s_tween_back_name;

    // ---------------------------------------------------------------------------
    // DIGIPEN SPLASH OVERLAY
    // ---------------------------------------------------------------------------
    static AEGfxTexture* s_tex_splash_logo = nullptr;
    static AEGfxVertexList* s_mesh_splash_quad = nullptr;
    static AEGfxVertexList* s_mesh_splash_logo = nullptr;

    static constexpr float SPLASH_HOLD = 2.0f;
    static constexpr float SPLASH_FADE = 1.0f;
    static float s_splash_timer = 0.0f;
    static bool  s_splash_active = false;
}

std::string main_menu_get_player_name()
{
    return s_player_name;
}

void draw_main_menu(float text_ndc_offset)
{
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetTransparency(1.0f);

    if (s_tex_logo)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(s_tex_logo, 0.0f, 0.0f);
        mesh_renderer(s_mesh_logo, 0.0f + s_ui_offset_x, s_logo_y, LOGO_WIDTH, LOGO_HEIGHT, 0.0f);
    }

    if (s_tex_arrows)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(s_tex_arrows, 0.0f, 0.0f);
        mesh_renderer(s_mesh_left_arrow, s_leftarrow_btn_x + s_ui_offset_x, s_leftarrow_btn_y,
            LEFTARROW_WIDTH * s_tween_left_arrow.scale, LEFTARROW_HEIGHT * s_tween_left_arrow.scale, 0.0f);
        mesh_renderer(s_mesh_right_arrow, s_rightarrow_btn_x + s_ui_offset_x, s_rightarrow_btn_y,
            RIGHTARROW_WIDTH * s_tween_right_arrow.scale, RIGHTARROW_HEIGHT * s_tween_right_arrow.scale, 0.0f);
    }

    if (Graphics::g_font_id >= 0)
    {
        float text_w = 0.0f, text_h = 0.0f;
        AEGfxGetPrintSize(Graphics::g_font_id, s_selected_mode.c_str(), 1.0f, &text_w, &text_h);
        float text_ndc_x = 0.0f - (text_w * 0.5f) + s_ui_offset_x;
        float text_ndc_y = s_rightarrow_btn_y / (s_win_h * 0.5f) - 0.025f + text_ndc_offset;
        AEGfxPrint(Graphics::g_font_id, s_selected_mode.c_str(), text_ndc_x, text_ndc_y, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    if (s_tex_play)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(s_tex_play, 0.0f, 0.0f);
        mesh_renderer(s_mesh_btn, 0.0f + s_ui_offset_x, s_play_btn_y,
            PLAY_WIDTH * s_tween_play.scale, PLAY_HEIGHT * s_tween_play.scale, 0.0f);
    }

    if (s_tex_shop)
    {
        AEGfxTextureSet(s_tex_shop, 0.0f, 0.0f);
        mesh_renderer(s_mesh_btn, 0.0f + s_ui_offset_x, s_shop_btn_y,
            SHOP_WIDTH * s_tween_shop.scale, SHOP_HEIGHT * s_tween_shop.scale, 0.0f);
    }

    if (s_tex_tutorial)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_tex_tutorial, 0.0f, 0.0f);
        mesh_renderer(s_mesh_btn, 0.0f + s_ui_offset_x, s_tutorial_btn_y,
            TUTORIAL_WIDTH * s_tween_tutorial.scale, TUTORIAL_HEIGHT * s_tween_tutorial.scale, 0.0f);
    }

    if (s_tex_leaderboard)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(s_tex_leaderboard, 0.0f, 0.0f);
        mesh_renderer(s_mesh_leaderboard_btn, s_leaderboard_btn_x + s_ui_offset_x, s_leaderboard_btn_y,
            LEADERBOARD_WIDTH * s_tween_leaderboard.scale, LEADERBOARD_HEIGHT * s_tween_leaderboard.scale, 0.0f);
    }

    if (s_tex_settings)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(s_tex_settings, 0.0f, 0.0f);
        mesh_renderer(s_mesh_settings_btn, s_settings_btn_x + s_ui_offset_x, s_settings_btn_y,
            SETTINGS_WIDTH * s_tween_settings.scale, SETTINGS_HEIGHT * s_tween_settings.scale, 0.0f);
    }

    if (s_tex_credits)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(s_tex_credits, 0.0f, 0.0f);
        mesh_renderer(s_mesh_credits_btn, s_credits_btn_x + s_ui_offset_x, s_credits_btn_y,
            CREDITS_WIDTH * s_tween_credits.scale, CREDITS_HEIGHT * s_tween_credits.scale, 0.0f);
    }

    if (s_tex_exit)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_tex_exit, 0.0f, 0.0f);
        mesh_renderer(s_mesh_exit_btn, 0.0f + s_ui_offset_x, s_exit_btn_y,
            EXIT_WIDTH * s_tween_exit.scale, EXIT_HEIGHT * s_tween_exit.scale, 0.0f);
    }
}

void main_menu_input()
{
    s32 mouse_x, mouse_y;
    AEInputGetCursorPosition(&mouse_x, &mouse_y);

    s_world_x = (float)mouse_x - (s_win_w * 0.5f);
    s_world_y = (s_win_h * 0.5f) - (float)mouse_y;

    if (s_selected_mode == "") s_selected_mode = "ZEN";

    // ---------------------------------------------------------------------------
    // BUTTON TWEEN UPDATES
    // ---------------------------------------------------------------------------
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();
        bool click = AEInputCheckTriggered(AEVK_LBUTTON);

        bool h_play = (s_world_x >= -(PLAY_HITBOX_WIDTH * 0.5f) && s_world_x <= (PLAY_HITBOX_WIDTH * 0.5f) &&
            s_world_y >= s_play_btn_y - (PLAY_HITBOX_HEIGHT * 0.5f) && s_world_y <= s_play_btn_y + (PLAY_HITBOX_HEIGHT * 0.5f));
        bool h_shop = (s_world_x >= -(SHOP_HITBOX_WIDTH * 0.5f) && s_world_x <= (SHOP_HITBOX_WIDTH * 0.5f) &&
            s_world_y >= s_shop_btn_y - (SHOP_HITBOX_HEIGHT * 0.5f) && s_world_y <= s_shop_btn_y + (SHOP_HITBOX_HEIGHT * 0.5f));
        bool h_tut = (s_world_x >= -(TUTORIAL_HITBOX_WIDTH * 0.5f) && s_world_x <= (TUTORIAL_HITBOX_WIDTH * 0.5f) &&
            s_world_y >= s_tutorial_btn_y - (TUTORIAL_HITBOX_HEIGHT * 0.5f) && s_world_y <= s_tutorial_btn_y + (TUTORIAL_HITBOX_HEIGHT * 0.5f));
        bool h_exit = (s_world_x >= -(EXIT_WIDTH * 0.5f) && s_world_x <= (EXIT_WIDTH * 0.5f) &&
            s_world_y >= s_exit_btn_y - (EXIT_HEIGHT * 0.5f) && s_world_y <= s_exit_btn_y + (EXIT_HEIGHT * 0.5f));
        bool h_set = (s_world_x >= s_settings_btn_x - (SETTINGS_WIDTH * 0.5f) && s_world_x <= s_settings_btn_x + (SETTINGS_WIDTH * 0.5f) &&
            s_world_y >= s_settings_btn_y - (SETTINGS_HEIGHT * 0.5f) && s_world_y <= s_settings_btn_y + (SETTINGS_HEIGHT * 0.5f));
        bool h_ldr = (s_world_x >= s_leaderboard_btn_x - (LEADERBOARD_WIDTH * 0.5f) && s_world_x <= s_leaderboard_btn_x + (LEADERBOARD_WIDTH * 0.5f) &&
            s_world_y >= s_leaderboard_btn_y - (LEADERBOARD_HEIGHT * 0.5f) && s_world_y <= s_leaderboard_btn_y + (LEADERBOARD_HEIGHT * 0.5f));
        bool h_l_arr = (s_world_x >= s_leftarrow_btn_x - (LEFTARROW_WIDTH * 0.5f) && s_world_x <= s_leftarrow_btn_x + (LEFTARROW_WIDTH * 0.5f) &&
            s_world_y >= s_leftarrow_btn_y - (LEFTARROW_HEIGHT * 0.5f) && s_world_y <= s_leftarrow_btn_y + (LEFTARROW_HEIGHT * 0.5f));
        bool h_r_arr = (s_world_x >= s_rightarrow_btn_x - (RIGHTARROW_WIDTH * 0.5f) && s_world_x <= s_rightarrow_btn_x + (RIGHTARROW_WIDTH * 0.5f) &&
            s_world_y >= s_rightarrow_btn_y - (RIGHTARROW_HEIGHT * 0.5f) && s_world_y <= s_rightarrow_btn_y + (RIGHTARROW_HEIGHT * 0.5f));
        bool h_cred = (s_world_x >= s_credits_btn_x - (CREDITS_WIDTH * 0.5f) && s_world_x <= s_credits_btn_x + (CREDITS_WIDTH * 0.5f) &&
            s_world_y >= s_credits_btn_y - (CREDITS_HEIGHT * 0.5f) && s_world_y <= s_credits_btn_y + (CREDITS_HEIGHT * 0.5f));

        if (!s_splash_active)
        {
            button_tween_update(s_tween_play, h_play, h_play && click, dt);
            button_tween_update(s_tween_shop, h_shop, h_shop && click, dt);
            button_tween_update(s_tween_tutorial, h_tut, h_tut && click, dt);
            button_tween_update(s_tween_exit, h_exit, h_exit && click, dt);
            button_tween_update(s_tween_settings, h_set, h_set && click, dt);
            button_tween_update(s_tween_leaderboard, h_ldr, h_ldr && click, dt);
            button_tween_update(s_tween_left_arrow, h_l_arr, h_l_arr && click, dt);
            button_tween_update(s_tween_right_arrow, h_r_arr, h_r_arr && click, dt);
            button_tween_update(s_tween_credits, h_cred, h_cred && click, dt);
        }
    }

    if (UiPauseMenu::is_popup_open()) return;

    if (!s_splash_active && !s_is_transitioning_to_name && !s_transitioning_to_shop && !s_is_returning_from_shop
        && AEInputCheckTriggered(AEVK_ESCAPE))
    {
        SoundManager::play_click_btn_sfx();
        UiPauseMenu::open_popup(UiPauseMenu::POPUP_QUIT);
    }

    if (!s_splash_active && !s_is_transitioning_to_name && !s_transitioning_to_shop && !s_is_returning_from_shop
        && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        if (s_world_x >= 0.0f - (PLAY_HITBOX_WIDTH * 0.5f) && s_world_x <= 0.0f + (PLAY_HITBOX_WIDTH * 0.5f) &&
            s_world_y >= s_play_btn_y - (PLAY_HITBOX_HEIGHT * 0.5f) && s_world_y <= s_play_btn_y + (PLAY_HITBOX_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_is_transitioning_to_name = true;
            ScreenTransitions::screen_transition_init();
        }
        else if (s_world_x >= 0.0f - (SHOP_HITBOX_WIDTH * 0.5f) && s_world_x <= 0.0f + (SHOP_HITBOX_WIDTH * 0.5f) &&
            s_world_y >= s_shop_btn_y - (SHOP_HITBOX_HEIGHT * 0.5f) && s_world_y <= s_shop_btn_y + (SHOP_HITBOX_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_transitioning_to_shop = true;
            ScreenTransitions::screen_transition_init();
        }
        else if (s_world_x >= 0.0f - (EXIT_WIDTH * 0.5f) && s_world_x <= 0.0f + (EXIT_WIDTH * 0.5f) &&
            s_world_y >= s_exit_btn_y - (EXIT_HEIGHT * 0.5f) && s_world_y <= s_exit_btn_y + (EXIT_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            UiPauseMenu::open_popup(UiPauseMenu::POPUP_QUIT);
        }
        else if (s_world_x >= s_settings_btn_x - (SETTINGS_WIDTH * 0.5f) && s_world_x <= s_settings_btn_x + (SETTINGS_WIDTH * 0.5f) &&
            s_world_y >= s_settings_btn_y - (SETTINGS_HEIGHT * 0.5f) && s_world_y <= s_settings_btn_y + (SETTINGS_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_in_settings = true;
        }
        else if (s_world_x >= s_leftarrow_btn_x - (LEFTARROW_WIDTH * 0.5f) && s_world_x <= s_leftarrow_btn_x + (LEFTARROW_WIDTH * 0.5f) &&
            s_world_y >= s_leftarrow_btn_y - (LEFTARROW_HEIGHT * 0.5f) && s_world_y <= s_leftarrow_btn_y + (LEFTARROW_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_selected_mode = (s_selected_mode == "ZEN") ? "TIME" : "ZEN";
        }
        else if (s_world_x >= s_rightarrow_btn_x - (RIGHTARROW_WIDTH * 0.5f) && s_world_x <= s_rightarrow_btn_x + (RIGHTARROW_WIDTH * 0.5f) &&
            s_world_y >= s_rightarrow_btn_y - (RIGHTARROW_HEIGHT * 0.5f) && s_world_y <= s_rightarrow_btn_y + (RIGHTARROW_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_selected_mode = (s_selected_mode == "ZEN") ? "TIME" : "ZEN";
        }
        else if (s_world_x >= s_leaderboard_btn_x - (LEADERBOARD_WIDTH * 0.5f) && s_world_x <= s_leaderboard_btn_x + (LEADERBOARD_WIDTH * 0.5f) &&
            s_world_y >= s_leaderboard_btn_y - (LEADERBOARD_HEIGHT * 0.5f) && s_world_y <= s_leaderboard_btn_y + (LEADERBOARD_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_leaderboard_mode = "ZEN";
            s_in_leaderboard = true;
        }
        else if (s_world_x >= 0.0f - (TUTORIAL_HITBOX_WIDTH * 0.5f) && s_world_x <= 0.0f + (TUTORIAL_HITBOX_WIDTH * 0.5f) &&
            s_world_y >= s_tutorial_btn_y - (TUTORIAL_HITBOX_HEIGHT * 0.5f) && s_world_y <= s_tutorial_btn_y + (TUTORIAL_HITBOX_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_is_zooming_to_game = true;
            ParallaxZoom::start(GS_TUTORIAL);
        }
        else if (s_world_x >= s_credits_btn_x - (CREDITS_WIDTH * 0.5f) && s_world_x <= s_credits_btn_x + (CREDITS_WIDTH * 0.5f) &&
            s_world_y >= s_credits_btn_y - (CREDITS_HEIGHT * 0.5f) && s_world_y <= s_credits_btn_y + (CREDITS_HEIGHT * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            g_game_state_next = GS_CREDITS;
        }
    }
}

void reset_player_name(std::string& reset_name)
{
    s_player_name = reset_name;
}

void main_menu_enter_name(float world_y_offset, float ndc_y_offset, bool allow_input)
{
    s32 mouse_x, mouse_y;
    AEInputGetCursorPosition(&mouse_x, &mouse_y);

    s_world_x = (float)mouse_x - (s_win_w * 0.5f);
    s_world_y = (s_win_h * 0.5f) - (float)mouse_y;

    if (allow_input)
    {
        bool enter_pressed = AEInputCheckTriggered(AEVK_RETURN);
        bool enter_clicked = AEInputCheckTriggered(AEVK_LBUTTON) &&
            (s_world_x >= s_entergame_btn_x - (EXITNAME_WIDTH * 0.5f) && s_world_x <= s_entergame_btn_x + (EXITNAME_WIDTH * 0.5f) &&
                s_world_y >= s_entergame_btn_y - (ENTERGAME_HEIGHT * 0.5f) && s_world_y <= s_entergame_btn_y + (ENTERGAME_HEIGHT * 0.5f));

        bool exit_clicked = AEInputCheckTriggered(AEVK_LBUTTON) &&
            (s_world_x >= s_exitname_btn_x - (EXITNAME_WIDTH * 0.5f) && s_world_x <= s_exitname_btn_x + (EXITNAME_WIDTH * 0.5f) &&
                s_world_y >= s_exitname_btn_y - (EXITNAME_HEIGHT * 0.5f) && s_world_y <= s_exitname_btn_y + (EXITNAME_HEIGHT * 0.5f));

        float dt = (float)AEFrameRateControllerGetFrameTime();
        bool h_enter = (s_world_x >= s_entergame_btn_x - (EXITNAME_WIDTH * 0.5f) && s_world_x <= s_entergame_btn_x + (EXITNAME_WIDTH * 0.5f) &&
            s_world_y >= s_entergame_btn_y - (ENTERGAME_HEIGHT * 0.5f) && s_world_y <= s_entergame_btn_y + (ENTERGAME_HEIGHT * 0.5f));
        bool h_back = (s_world_x >= s_exitname_btn_x - (EXITNAME_WIDTH * 0.5f) && s_world_x <= s_exitname_btn_x + (EXITNAME_WIDTH * 0.5f) &&
            s_world_y >= s_exitname_btn_y - (EXITNAME_HEIGHT * 0.5f) && s_world_y <= s_exitname_btn_y + (EXITNAME_HEIGHT * 0.5f));
        button_tween_update(s_tween_enter_game, h_enter, enter_clicked, dt);
        button_tween_update(s_tween_back_name, h_back, exit_clicked, dt);

        if ((enter_pressed || enter_clicked) && !s_player_name.empty() && s_selected_mode != "")
        {
            SoundManager::play_click_btn_sfx();
            s_is_entering_name = false;
            s_is_zooming_to_game = true;
            int zoom_target = (s_selected_mode == "TIME") ? GS_GAME_TIMEMODE : GS_GAME;
            ParallaxZoom::start(zoom_target);
        }
        else if ((enter_pressed || enter_clicked) && s_player_name.empty() && s_selected_mode != "")
        {
            SoundManager::play_click_btn_sfx();
            s_empty_name_warning_timer = 1.5f;
            s_empty_name_warning_x = 0.0f;
            s_empty_name_warning_y = 0.05f;
        }
        else if (exit_clicked)
        {
            SoundManager::play_click_btn_sfx();
            s_is_transitioning_back = true;
            ScreenTransitions::screen_transition_init();
            s_is_entering_name = false;
            s_selected_mode = "ZEN";
        }

        if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            s_is_transitioning_back = true;
            ScreenTransitions::screen_transition_init();
            s_is_entering_name = false;
            s_selected_mode = "ZEN";
        }

        if (AEInputCheckTriggered(AEVK_BACK) && !s_player_name.empty())
            s_player_name.pop_back();

        if (s_player_name.length() < 12)
        {
            for (int k = AEVK_A; k <= AEVK_Z; ++k)
            {
                if (AEInputCheckTriggered(static_cast<u8>(k)))
                {
                    char c = (char)('A' + (k - AEVK_A));
                    s_player_name += c;
                    std::cout << s_player_name << "\n";
                }
            }
        }

        s_cursor_timer += (float)AEFrameRateControllerGetFrameTime();
        if (s_cursor_timer >= 0.5f)
        {
            s_cursor_visible = !s_cursor_visible;
            s_cursor_timer = 0.0f;
        }
    }

    if (s_tex_back_name)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_tex_back_name, 0.0f, 0.0f);
        mesh_renderer(s_mesh_back_name, s_exitname_btn_x, s_exitname_btn_y + world_y_offset,
            EXITNAME_WIDTH * s_tween_back_name.scale, EXITNAME_HEIGHT * s_tween_back_name.scale, 0.0f);
    }

    if (s_tex_enter_name)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
        AEGfxTextureSet(s_tex_enter_name, 0.0f, 0.0f);
        mesh_renderer(s_mesh_enter_name, s_entergame_btn_x, s_entergame_btn_y + world_y_offset,
            ENTERGAME_WIDTH * s_tween_enter_game.scale, ENTERGAME_HEIGHT * s_tween_enter_game.scale, 0.0f);
    }

    if (Graphics::g_font_id >= 0)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        const char* prompt_text = "Enter Name:";
        float prompt_width = 0.0f, prompt_height = 0.0f;
        constexpr float PROMPT_SCALE = 1.0f;
        float prompt_y = 0.4f + ndc_y_offset;

        AEGfxGetPrintSize(Graphics::g_font_id, prompt_text, PROMPT_SCALE, &prompt_width, &prompt_height);
        float prompt_start_x = 0.0f - (prompt_width * 0.5f);
        AEGfxPrint(Graphics::g_font_id, prompt_text, prompt_start_x, prompt_y, PROMPT_SCALE, 0.0f, 0.0f, 0.0f, 1.0f);

        float mode_y = -0.35f + ndc_y_offset;
        float text_y = 0.2f + ndc_y_offset;
        std::string mode_display = "Mode: " + s_selected_mode;

        float mode_width = 0.0f, mode_height = 0.0f;
        constexpr float MODE_SCALE = 1.0f;

        AEGfxGetPrintSize(Graphics::g_font_id, mode_display.c_str(), MODE_SCALE, &mode_width, &mode_height);
        float mode_start_x = 0.0f - (mode_width * 0.5f);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::g_font_id, mode_display.c_str(), mode_start_x, mode_y, MODE_SCALE, 0.0f, 0.0f, 0.0f, 1.0f);

        float text_width = 0.0f, text_height = 0.0f;
        constexpr float TEXT_SCALE = 1.0f;
        float start_x = 0.0f;
        text_y = 0.2f;

        if (s_selected_mode == "ZEN")
        {
            AEGfxGetPrintSize(Graphics::g_font_id, mode_display.c_str(), MODE_SCALE, &mode_width, &mode_height);
            mode_start_x = 0.0f - (mode_width * 0.5f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::g_font_id, mode_display.c_str(), mode_start_x, mode_y, MODE_SCALE, 0.0f, 0.0f, 0.0f, 1.0f);
        }

        if (s_selected_mode == "TIME")
        {
            AEGfxGetPrintSize(Graphics::g_font_id, mode_display.c_str(), MODE_SCALE, &mode_width, &mode_height);
            mode_start_x = 0.0f - (mode_width * 0.5f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::g_font_id, mode_display.c_str(), mode_start_x, mode_y, MODE_SCALE, 0.0f, 0.0f, 0.0f, 1.0f);
        }

        text_y = 0.2f + ndc_y_offset;
        if (!s_player_name.empty())
        {
            AEGfxGetPrintSize(Graphics::g_font_id, s_player_name.c_str(), TEXT_SCALE, &text_width, &text_height);
            start_x = 0.0f - (text_width * 0.5f);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::g_font_id, s_player_name.c_str(), start_x, text_y, TEXT_SCALE, 0.0f, 0.0f, 0.0f, 1.0f);
        }

        if (s_empty_name_warning_timer > 0.0f)
        {
            float alpha = s_empty_name_warning_timer / 0.5f;
            if (alpha > 1.0f) alpha = 1.0f;

            const char* warning_text = "PLEASE ENTER A NAME";
            float tw, th;
            constexpr float WARNING_SCALE = 0.85f;
            AEGfxGetPrintSize(Graphics::g_font_id, warning_text, WARNING_SCALE, &tw, &th);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);

            float ndc_x = 0.0f - (tw * 0.5f);
            float ndc_y = s_empty_name_warning_y + ndc_y_offset;
            AEGfxPrint(Graphics::g_font_id, warning_text, ndc_x, ndc_y, WARNING_SCALE, 1.0f, 0.2f, 0.2f, alpha);
        }

        if (s_cursor_visible)
        {
            float cursor_x = start_x + text_width;
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::g_font_id, "_", cursor_x, text_y, TEXT_SCALE, 0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}

void main_menu_init()
{
    // Safety cleanup before reload
    if (s_tex_logo) { AEGfxTextureUnload(s_tex_logo);           s_tex_logo = nullptr; }
    if (s_tex_leaderboard) { AEGfxTextureUnload(s_tex_leaderboard);    s_tex_leaderboard = nullptr; }
    if (s_tex_play) { AEGfxTextureUnload(s_tex_play);           s_tex_play = nullptr; }
    if (s_tex_shop) { AEGfxTextureUnload(s_tex_shop);           s_tex_shop = nullptr; }
    if (s_tex_arrows) { AEGfxTextureUnload(s_tex_arrows);         s_tex_arrows = nullptr; }
    if (s_tex_enter_name) { AEGfxTextureUnload(s_tex_enter_name);     s_tex_enter_name = nullptr; }
    if (s_tex_back_name) { AEGfxTextureUnload(s_tex_back_name);      s_tex_back_name = nullptr; }
    if (s_tex_settings) { AEGfxTextureUnload(s_tex_settings);       s_tex_settings = nullptr; }
    if (s_tex_tutorial) { AEGfxTextureUnload(s_tex_tutorial);       s_tex_tutorial = nullptr; }
    if (s_tex_credits) { AEGfxTextureUnload(s_tex_credits);        s_tex_credits = nullptr; }
    if (s_tex_exit) { AEGfxTextureUnload(s_tex_exit);           s_tex_exit = nullptr; }

    if (s_mesh_logo) { AEGfxMeshFree(s_mesh_logo);           s_mesh_logo = nullptr; }
    if (s_mesh_btn) { AEGfxMeshFree(s_mesh_btn);            s_mesh_btn = nullptr; }
    if (s_mesh_exit_btn) { AEGfxMeshFree(s_mesh_exit_btn);       s_mesh_exit_btn = nullptr; }
    if (s_mesh_enter_name) { AEGfxMeshFree(s_mesh_enter_name);     s_mesh_enter_name = nullptr; }
    if (s_mesh_back_name) { AEGfxMeshFree(s_mesh_back_name);      s_mesh_back_name = nullptr; }
    if (s_mesh_leaderboard_btn) { AEGfxMeshFree(s_mesh_leaderboard_btn); s_mesh_leaderboard_btn = nullptr; }
    if (s_mesh_credits_btn) { AEGfxMeshFree(s_mesh_credits_btn);    s_mesh_credits_btn = nullptr; }
    if (s_mesh_left_arrow) { AEGfxMeshFree(s_mesh_left_arrow);     s_mesh_left_arrow = nullptr; }
    if (s_mesh_right_arrow) { AEGfxMeshFree(s_mesh_right_arrow);    s_mesh_right_arrow = nullptr; }
    if (s_mesh_settings_btn) { AEGfxMeshFree(s_mesh_settings_btn);   s_mesh_settings_btn = nullptr; }
    if (s_mesh_tutorial_btn) { AEGfxMeshFree(s_mesh_tutorial_btn);   s_mesh_tutorial_btn = nullptr; }

    s_ui_offset_x = 0.0f;
    s_transitioning_to_shop = false;
    s_in_leaderboard = false;
    UiPauseMenu::close_popup();
    LeaderboardSystem::init();
    LeaderboardSystem::read_zen_leaderboard();
    LeaderboardSystem::read_time_leaderboard();
    s_is_zooming_to_game = false;
    ParallaxZoom::reset();

    s_empty_name_warning_timer = 0.0f;
    s_empty_name_warning_x = 0.0f;
    s_empty_name_warning_y = 0.0f;

    SoundManager::init();
    SysSettings::init();
    BackgroundParallax::menu_parallax_init();

    s_selected_mode = "ZEN";
    s_in_settings = false;
    s_is_transitioning_to_name = false;

    if (Graphics::g_font_id < 0)
        Graphics::g_font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 48);

    s_tex_logo = AEGfxTextureLoad("Assets/title.png");
    AE_ASSERT_MESG(s_tex_logo, "Failed to load title.png");

    s_tex_leaderboard = AEGfxTextureLoad("Assets/leaderboardt.png");
    AE_ASSERT_MESG(s_tex_leaderboard, "Failed to load leaderboardt.png")

        s_tex_play = AEGfxTextureLoad("Assets/play.png");
    AE_ASSERT_MESG(s_tex_play, "Failed to load play.png");

    s_tex_shop = AEGfxTextureLoad("Assets/shop.png");
    AE_ASSERT_MESG(s_tex_shop, "Failed to load shop.png");

    s_tex_arrows = AEGfxTextureLoad("Assets/LeftRightButtons.png");
    AE_ASSERT_MESG(s_tex_arrows, "Failed to load LeftRightButtons.png");

    s_tex_enter_name = AEGfxTextureLoad("Assets/playbutton.png");
    AE_ASSERT_MESG(s_tex_enter_name, "Failed to load playbutton.png");

    s_tex_back_name = AEGfxTextureLoad("Assets/backbutton.png");
    AE_ASSERT_MESG(s_tex_back_name, "Failed to load backbutton.png");

    s_tex_settings = AEGfxTextureLoad("Assets/settingshover.png");
    AE_ASSERT_MESG(s_tex_settings, "Failed to load settingshover.png");

    s_tex_tutorial = AEGfxTextureLoad("Assets/tutorial.png");
    AE_ASSERT_MESG(s_tex_tutorial, "Failed to load tutorial.png");

    s_tex_credits = AEGfxTextureLoad("Assets/credits.png");
    AE_ASSERT_MESG(s_tex_credits, "Failed to load credits.png");

    s_tex_exit = AEGfxTextureLoad("Assets/quit.png");
    AE_ASSERT_MESG(s_tex_exit, "Failed to load quit.png");

    s_mesh_logo = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_btn = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_exit_btn = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_enter_name = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_back_name = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_leaderboard_btn = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_credits_btn = Sprite_Mesh(1.0f, 1.0f);

    s_mesh_left_arrow = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);
    s_mesh_right_arrow = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 1.0f);
    s_mesh_settings_btn = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f);
    s_mesh_tutorial_btn = Sprite_Mesh(1.0f, 1.0f);

    if (g_game_state_prev == GS_SHOP)
    {
        s_is_returning_from_shop = true;
        s_ui_offset_x = -s_win_w;
        ScreenTransitions::screen_transition_init();
    }
    else
    {
        s_is_returning_from_shop = false;
        s_ui_offset_x = 0.0f;
    }

    button_tween_reset(s_tween_play);
    button_tween_reset(s_tween_shop);
    button_tween_reset(s_tween_tutorial);
    button_tween_reset(s_tween_exit);
    button_tween_reset(s_tween_settings);
    button_tween_reset(s_tween_leaderboard);
    button_tween_reset(s_tween_left_arrow);
    button_tween_reset(s_tween_right_arrow);
    button_tween_reset(s_tween_credits);
    button_tween_reset(s_tween_enter_game);
    button_tween_reset(s_tween_back_name);

    // Splash overlay ? only on very first launch
    s_splash_timer = 0.0f;
    s_splash_active = (g_game_state_prev == GS_NONE);

    if (s_splash_active)
    {
        s_tex_splash_logo = AEGfxTextureLoad("Assets/DigiPen_BLACK.png");

        AEGfxMeshStart();
        AEGfxTriAdd(-1.f, -1.f, 0xFF000000, 0.f, 1.f,
            1.f, -1.f, 0xFF000000, 1.f, 1.f,
            -1.f, 1.f, 0xFF000000, 0.f, 0.f);
        AEGfxTriAdd(1.f, -1.f, 0xFF000000, 1.f, 1.f,
            1.f, 1.f, 0xFF000000, 1.f, 0.f,
            -1.f, 1.f, 0xFF000000, 0.f, 0.f);
        s_mesh_splash_quad = AEGfxMeshEnd();

        AEGfxMeshStart();
        AEGfxTriAdd(-1.f, -1.f, 0xFFFFFFFF, 0.f, 1.f,
            1.f, -1.f, 0xFFFFFFFF, 1.f, 1.f,
            -1.f, 1.f, 0xFFFFFFFF, 0.f, 0.f);
        AEGfxTriAdd(1.f, -1.f, 0xFFFFFFFF, 1.f, 1.f,
            1.f, 1.f, 0xFFFFFFFF, 1.f, 0.f,
            -1.f, 1.f, 0xFFFFFFFF, 0.f, 0.f);
        s_mesh_splash_logo = AEGfxMeshEnd();
    }
}

void main_menu_update()
{
    s_win_w = (float)AEGfxGetWindowWidth();
    s_win_h = (float)AEGfxGetWindowHeight();

    float dt = (float)AEFrameRateControllerGetFrameTime();
    if (dt > 0.1f) dt = 0.01667f;

    if (s_empty_name_warning_timer > 0.0f)
        s_empty_name_warning_timer -= dt;

    BackgroundParallax::menu_parallax_update(dt);

    if (s_transitioning_to_shop)
    {
        ScreenTransitions::screen_transition_update(dt);
        float smooth_pan = ScreenTransitions::ease_out_expo(ScreenTransitions::result);
        s_ui_offset_x = -s_win_w * smooth_pan;
        BackgroundParallax::menu_parallax_set_pan_offset(-0.2f * smooth_pan);

        if (ScreenTransitions::result >= 0.3f)
        {
            s_transitioning_to_shop = false;
            s_ui_offset_x = -s_win_w;
            BackgroundParallax::menu_parallax_set_pan_offset(-0.2f);
            g_game_state_next = GS_SHOP;
        }
    }

    if (s_is_returning_from_shop)
    {
        ScreenTransitions::screen_transition_update(dt);
        float smooth_pan = ScreenTransitions::ease_out_expo(ScreenTransitions::result);
        s_ui_offset_x = -s_win_w * (1.0f - smooth_pan);
        BackgroundParallax::menu_parallax_set_pan_offset(-0.2f + (0.2f * smooth_pan));

        if (ScreenTransitions::result >= 0.3f)
        {
            s_is_returning_from_shop = false;
            s_ui_offset_x = 0.0f;
            BackgroundParallax::menu_parallax_set_pan_offset(0.0f);
        }
    }

    s_logo_y = s_win_h * 0.35f;
    s_play_btn_y = s_win_h * 0.15f;
    s_tutorial_btn_y = -s_win_h * 0.04f;
    s_shop_btn_y = -s_win_h * 0.23f;
    s_exit_btn_y = -s_win_h * 0.42f;

    s_exitname_btn_y = -s_win_h * 0.35f;
    s_exitname_btn_x = -s_win_w * 0.35f;
    s_entergame_btn_x = s_win_w * 0.35f;
    s_entergame_btn_y = -s_win_h * 0.35f;

    s_zenmode_btn_x = -s_win_w * 0.12f;
    s_zenmode_btn_y = s_win_h * 0.0f;
    s_timemode_btn_x = s_win_w * 0.12f;
    s_timemode_btn_y = s_win_h * 0.0f;

    s_settings_btn_x = -s_win_w * 0.35f;
    s_settings_btn_y = -s_win_h * 0.4f;
    s_leaderboard_btn_x = -s_win_w * 0.23f;
    s_leaderboard_btn_y = -s_win_h * 0.4f;

    s_leftarrow_btn_x = -s_win_h * 0.12f;
    s_leftarrow_btn_y = s_play_btn_y + 85.0f;
    s_rightarrow_btn_x = s_win_h * 0.12f;
    s_rightarrow_btn_y = s_play_btn_y + 85.0f;

    s_credits_btn_x = s_win_w * 0.23f;
    s_credits_btn_y = -s_win_h * 0.4f;

    AEGfxSetCamPosition(0.0f, 0.0f);
    if (s_is_zooming_to_game || ParallaxZoom::is_active())
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    else
        AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    float cam_y = 0.0f;
    float text_ndc_offset = 0.0f;

    if (s_is_transitioning_to_name)
    {
        ScreenTransitions::screen_transition_update((float)AEFrameRateControllerGetFrameTime());
        float bounce = ScreenTransitions::ease_out_expo(ScreenTransitions::result);

        cam_y = s_win_h * bounce;
        AEGfxSetCamPosition(0.0f, cam_y);
        text_ndc_offset = -(2.0f * bounce);

        if (ScreenTransitions::result >= 0.6f)
        {
            s_is_transitioning_to_name = false;
            s_is_entering_name = true;
            cam_y = 0.0f;
            text_ndc_offset = 0.0f;
        }
    }
    else if (s_is_transitioning_back)
    {
        ScreenTransitions::screen_transition_update((float)AEFrameRateControllerGetFrameTime());
        float bounce = ScreenTransitions::ease_out_expo(ScreenTransitions::result);
        float reverse_bounce = 1.0f - bounce;

        cam_y = s_win_h * reverse_bounce;
        text_ndc_offset = -(2.0f * reverse_bounce);

        if (ScreenTransitions::result >= 0.6f)
        {
            s_is_transitioning_back = false;
            s_is_entering_name = false;
            cam_y = 0.0f;
            text_ndc_offset = 0.0f;
            s_player_name = "";
            s_selected_mode = "ZEN";
        }
    }

    AEGfxSetCamPosition(0.0f, cam_y);

    float v_cam_y = 0.0f;
    if (s_is_transitioning_to_name) v_cam_y = s_win_h * ScreenTransitions::ease_out_expo(ScreenTransitions::result);
    else if (s_is_transitioning_back)    v_cam_y = s_win_h * (1.0f - ScreenTransitions::ease_out_expo(ScreenTransitions::result));
    else if (s_is_entering_name)         v_cam_y = s_win_h;

    if (s_is_zooming_to_game)
    {
        ParallaxZoom::update(dt);
        ParallaxZoom::draw(0.0f);
        if (!ParallaxZoom::is_active()) s_is_zooming_to_game = false;
        return;
    }

    BackgroundParallax::menu_parallax_draw(v_cam_y);

    if (s_in_leaderboard)
    {
        draw_main_menu(0.0f);

        if (s_leaderboard_mode == "ZEN")  LeaderboardSystem::display_zen_leaderboard(s_in_leaderboard);
        else if (s_leaderboard_mode == "TIME") LeaderboardSystem::display_time_leaderboard(s_in_leaderboard);

        s32 mouse_x, mouse_y;
        AEInputGetCursorPosition(&mouse_x, &mouse_y);
        s_win_w = (float)AEGfxGetWindowWidth();
        s_win_h = (float)AEGfxGetWindowHeight();

        float wx = (float)mouse_x - (s_win_w * 0.5f);
        float wy = (s_win_h * 0.5f) - (float)mouse_y;

        float arrow_w = 80.0f;
        float arrow_h = 80.0f;
        float arrow_x = 0.0f;
        float arrow_y = 0.0f;

        if (s_leaderboard_mode == "ZEN")  arrow_x = (s_win_w * 0.5f) - 380.0f;
        else                               arrow_x = -(s_win_w * 0.5f) + 380.0f;

        bool is_hovering = (wx >= arrow_x - arrow_w * 1.5f && wx <= arrow_x + arrow_w * 1.5f &&
            wy >= arrow_y - arrow_h * 1.2f && wy <= arrow_y + arrow_h * 1.2f);
        float draw_scale = is_hovering ? 1.2f : 1.0f;

        if (s_tex_arrows)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            if (is_hovering) AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
            else             AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(s_tex_arrows, 0.0f, 0.0f);

            if (s_leaderboard_mode == "ZEN")
                mesh_renderer(s_mesh_right_arrow, arrow_x, arrow_y, arrow_w * draw_scale, arrow_h * draw_scale, 0.0f);
            else
                mesh_renderer(s_mesh_left_arrow, arrow_x, arrow_y, arrow_w * draw_scale, arrow_h * draw_scale, 0.0f);

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        }

        if (Graphics::g_font_id >= 0)
        {
            const char* lbl = (s_leaderboard_mode == "ZEN") ? "TIME MODE" : "ZEN MODE";
            float tw, th;
            AEGfxGetPrintSize(Graphics::g_font_id, lbl, 0.45f, &tw, &th);

            float ndc_x = 0.0f;
            if (s_leaderboard_mode == "ZEN")
                ndc_x = (arrow_x - arrow_w * 0.5f) / (s_win_w * 0.5f) - tw - 0.05f;
            else
                ndc_x = (arrow_x + arrow_w * 0.5f) / (s_win_w * 0.5f) + 0.05f;

            float ndc_y = (arrow_y - 10.0f) / (s_win_h * 0.5f);

            if (is_hovering) AEGfxPrint(Graphics::g_font_id, lbl, ndc_x, ndc_y, 0.45f, 1.0f, 1.0f, 0.0f, 1.0f);
            else             AEGfxPrint(Graphics::g_font_id, lbl, ndc_x, ndc_y, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        if (is_hovering && AEInputCheckTriggered(AEVK_LBUTTON))
        {
            SoundManager::play_click_btn_sfx();
            s_leaderboard_mode = (s_leaderboard_mode == "ZEN") ? "TIME" : "ZEN";
        }
    }
    else if (s_in_settings)
    {
        if (SysSettings::display_settings() == false)
            s_in_settings = false;
    }
    else if (s_is_entering_name)
    {
        main_menu_enter_name(0.0f, 0.0f, true);
    }
    else if (s_is_zooming_to_game)
    {
        v_cam_y = s_win_h;
    }
    else if (s_is_transitioning_to_name || s_is_transitioning_back)
    {
        draw_main_menu(text_ndc_offset);
        float name_world_y = s_win_h;
        float name_ndc_y = 2.0f + text_ndc_offset;
        main_menu_enter_name(name_world_y, name_ndc_y, false);
    }
    else
    {
        draw_main_menu(0.0f);
        main_menu_input();
        UiPauseMenu::ui_draw_confirmation_popup(s_mesh_btn, nullptr);
    }

    // ---------------------------------------------------------------------------
    // SPLASH OVERLAY
    // ---------------------------------------------------------------------------
    if (s_splash_active)
    {
        s_splash_timer += dt;

        float alpha = 1.0f;
        if (s_splash_timer > SPLASH_HOLD)
        {
            alpha = 1.0f - (s_splash_timer - SPLASH_HOLD) / SPLASH_FADE;
            if (alpha < 0.0f) alpha = 0.0f;
        }

        if (alpha <= 0.0f)
        {
            s_splash_active = false;
            if (s_mesh_splash_quad) { AEGfxMeshFree(s_mesh_splash_quad);       s_mesh_splash_quad = nullptr; }
            if (s_mesh_splash_logo) { AEGfxMeshFree(s_mesh_splash_logo);       s_mesh_splash_logo = nullptr; }
            if (s_tex_splash_logo) { AEGfxTextureUnload(s_tex_splash_logo);   s_tex_splash_logo = nullptr; }
        }
        else
        {
            AEGfxSetCamPosition(0.0f, 0.0f);

            AEMtx33 scale_mat, transform;
            AEMtx33Scale(&scale_mat, s_win_w * 0.5f, s_win_h * 0.5f);
            AEMtx33Trans(&transform, 0.0f, 0.0f);
            AEMtx33Concat(&transform, &transform, &scale_mat);

            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(alpha);
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, alpha);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(s_mesh_splash_quad, AE_GFX_MDM_TRIANGLES);

            if (s_tex_splash_logo && s_mesh_splash_logo)
            {
                const float logo_w = s_win_w * 0.5f;
                const float logo_h = logo_w / 3.5f;

                AEMtx33 logo_scale, logo_trans, logo_transform;
                AEMtx33Scale(&logo_scale, logo_w * 0.5f, logo_h * 0.5f);
                AEMtx33Trans(&logo_trans, 0.0f, 0.0f);
                AEMtx33Concat(&logo_transform, &logo_trans, &logo_scale);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(alpha);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
                AEGfxTextureSet(s_tex_splash_logo, 0.0f, 0.0f);
                AEGfxSetTransform(logo_transform.m);
                AEGfxMeshDraw(s_mesh_splash_logo, AE_GFX_MDM_TRIANGLES);
            }
        }

        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

void main_menu_exit()
{
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);
    SysSettings::cleanup();
    if (g_game_state_next != GS_SHOP && g_game_state_next != GS_CREDITS)
    {
        BackgroundParallax::menu_parallax_exit();
        SoundManager::cleanup();
    }
    ScreenTransitions::s_is_transitioning = false;
    AEGfxSetCamPosition(0.0f, 0.0f);

    if (s_mesh_logo) { AEGfxMeshFree(s_mesh_logo);            s_mesh_logo = nullptr; }
    if (s_mesh_btn) { AEGfxMeshFree(s_mesh_btn);             s_mesh_btn = nullptr; }
    if (s_mesh_exit_btn) { AEGfxMeshFree(s_mesh_exit_btn);        s_mesh_exit_btn = nullptr; }
    if (s_mesh_settings_btn) { AEGfxMeshFree(s_mesh_settings_btn);    s_mesh_settings_btn = nullptr; }
    if (s_mesh_left_arrow) { AEGfxMeshFree(s_mesh_left_arrow);      s_mesh_left_arrow = nullptr; }
    if (s_mesh_right_arrow) { AEGfxMeshFree(s_mesh_right_arrow);     s_mesh_right_arrow = nullptr; }
    if (s_mesh_leaderboard_btn) { AEGfxMeshFree(s_mesh_leaderboard_btn); s_mesh_leaderboard_btn = nullptr; }
    if (s_mesh_enter_name) { AEGfxMeshFree(s_mesh_enter_name);      s_mesh_enter_name = nullptr; }
    if (s_mesh_back_name) { AEGfxMeshFree(s_mesh_back_name);       s_mesh_back_name = nullptr; }
    if (s_mesh_credits_btn) { AEGfxMeshFree(s_mesh_credits_btn);     s_mesh_credits_btn = nullptr; }
    if (s_mesh_tutorial_btn) { AEGfxMeshFree(s_mesh_tutorial_btn);    s_mesh_tutorial_btn = nullptr; }

    if (s_tex_play) { AEGfxTextureUnload(s_tex_play);         s_tex_play = nullptr; }
    if (s_tex_shop) { AEGfxTextureUnload(s_tex_shop);         s_tex_shop = nullptr; }
    if (s_tex_arrows) { AEGfxTextureUnload(s_tex_arrows);       s_tex_arrows = nullptr; }
    if (s_tex_logo) { AEGfxTextureUnload(s_tex_logo);         s_tex_logo = nullptr; }
    if (s_tex_enter_name) { AEGfxTextureUnload(s_tex_enter_name);   s_tex_enter_name = nullptr; }
    if (s_tex_back_name) { AEGfxTextureUnload(s_tex_back_name);    s_tex_back_name = nullptr; }
    if (s_tex_leaderboard) { AEGfxTextureUnload(s_tex_leaderboard);  s_tex_leaderboard = nullptr; }
    if (s_tex_settings) { AEGfxTextureUnload(s_tex_settings);     s_tex_settings = nullptr; }
    if (s_tex_tutorial) { AEGfxTextureUnload(s_tex_tutorial);     s_tex_tutorial = nullptr; }
    if (s_tex_credits) { AEGfxTextureUnload(s_tex_credits);      s_tex_credits = nullptr; }
    if (s_tex_exit) { AEGfxTextureUnload(s_tex_exit);         s_tex_exit = nullptr; }

    if (s_mesh_splash_quad) { AEGfxMeshFree(s_mesh_splash_quad);       s_mesh_splash_quad = nullptr; }
    if (s_mesh_splash_logo) { AEGfxMeshFree(s_mesh_splash_logo);       s_mesh_splash_logo = nullptr; }
    if (s_tex_splash_logo) { AEGfxTextureUnload(s_tex_splash_logo);   s_tex_splash_logo = nullptr; }
    s_splash_active = false;

    ParallaxZoom::reset();
    s_is_zooming_to_game = false;
    LeaderboardSystem::free();
}