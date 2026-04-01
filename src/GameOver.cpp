#include "pch.h"
#include "GameOver.h"
#include "ButtonTween.h"


namespace
{
    static float s_fade_in_timer = 0.0f;
    constexpr float FADE_IN_DURATION = 0.5f;
    static bool s_initialized = false;

    static ButtonTween s_tween_restart;
    static ButtonTween s_tween_leaderboard;
    static ButtonTween s_tween_menu;

    static AEGfxVertexList* s_mesh = nullptr;

    void ensure_mesh()
    {
        if (s_mesh)
        {
            AEGfxMeshFree(s_mesh);
            s_mesh = nullptr;
        }

        if (!s_mesh)
            s_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    }

    bool draw_button(const char* label, float cx, float cy, float w, float h,
        float r, float g, float b, ButtonTween& tween)
    {
        ensure_mesh();

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();

        s32 mx, my;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - win_w * 0.5f;
        float wy = win_h * 0.5f - (float)my;
        bool hovered = (wx >= cx - w * 0.5f && wx <= cx + w * 0.5f &&
            wy >= cy - h * 0.5f && wy <= cy + h * 0.5f);
        bool clicked = AEInputCheckTriggered(AEVK_LBUTTON) && hovered;
        float dt = (float)AEFrameRateControllerGetFrameTime();
        button_tween_update(tween, hovered, clicked, dt);

        float sw = tween.scale;
        float scaled_w = w * sw;
        float scaled_h = h * sw;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.15f, 0.15f, 0.15f, 1.0f);
        mesh_renderer(s_mesh, cx, cy, scaled_w, scaled_h, 0.0f);

        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        mesh_renderer(s_mesh, cx, cy + scaled_h * 0.5f - 3.0f, scaled_w, 4.0f, 0.0f);

        if (Graphics::g_font_id >= 0)
        {
            float lw, lh;
            AEGfxGetPrintSize(Graphics::g_font_id, label, 0.6f, &lw, &lh);
            float ndc_x = -lw * 0.5f;
            float ndc_y = (cy / (win_h * 0.5f)) - lh * 0.5f;
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::g_font_id, label, ndc_x, ndc_y, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        return clicked;
    }
}

namespace UiGameOver
{
    void cleanup()
    {
        if (s_mesh)
        {
            AEGfxMeshFree(s_mesh);
            s_mesh = nullptr;
        }
        s_fade_in_timer = 0.0f;
        s_initialized = false;
        button_tween_reset(s_tween_restart);
        button_tween_reset(s_tween_leaderboard);
        button_tween_reset(s_tween_menu);
    }

    void check_if_dead(bool& is_dead, bool& is_game_over, bool& open_leaderboard,
        AEGfxVertexList* bg_mesh, std::string temp_name, const Data::Score& score)
    {
        (void)bg_mesh;
        if (!is_dead) return;

        if (!is_game_over)
        {
            temp_name = main_menu_get_player_name();
            LeaderboardSystem::write_zen_leaderboard(temp_name, score.score_value);
            is_game_over = true;
            s_fade_in_timer = 0.0f;
        }

        if (!open_leaderboard)
            ui_game_over_menu(is_game_over, score, open_leaderboard, bg_mesh, is_dead);
        else
            LeaderboardSystem::display_zen_leaderboard(open_leaderboard);
    }

    void ui_game_over_menu(bool& is_game_over, const Data::Score& score,
        bool& open_leaderboard, AEGfxVertexList* bg_mesh, bool& is_dead)
    {
        (void)bg_mesh;
        (void)is_game_over;
        ensure_mesh();

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();
        float dt = (float)AEFrameRateControllerGetFrameTime();

        s_fade_in_timer += dt;
        float fade_alpha = s_fade_in_timer / FADE_IN_DURATION;
        if (fade_alpha > 1.0f) fade_alpha = 1.0f;

        AEGfxSetCamPosition(0.0f, 0.0f);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(fade_alpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        mesh_renderer(s_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);

        if (Graphics::g_font_id < 0) return;

        const char* title_text = "GAME OVER";
        float tw, th;
        AEGfxGetPrintSize(Graphics::g_font_id, title_text, 1.4f, &tw, &th);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, fade_alpha);
        AEGfxPrint(Graphics::g_font_id, title_text,
            -tw * 0.5f, 0.55f,
            1.4f, 1.0f, 0.15f, 0.15f, fade_alpha);

        std::string score_str = "SCORE:  " + std::to_string(score.score_value);
        float sw, sh;
        AEGfxGetPrintSize(Graphics::g_font_id, score_str.c_str(), 0.8f, &sw, &sh);
        AEGfxPrint(Graphics::g_font_id, score_str.c_str(),
            -sw * 0.5f, 0.3f,
            0.8f, 1.0f, 1.0f, 1.0f, fade_alpha);

        if (fade_alpha < 0.3f) return;

        constexpr float BTN_W = 350.0f;
        constexpr float BTN_H = 65.0f;
        constexpr float BTN_X = 0.0f;
        constexpr float GAP = 90.0f;

        float restart_y = 50.0f;
        float leaderboard_y = restart_y - GAP;
        float menu_y = leaderboard_y - GAP;

        if (draw_button("RESTART", BTN_X, restart_y, BTN_W, BTN_H, 0.2f, 0.8f, 0.2f, s_tween_restart))
        {
            is_dead = false;
            s_fade_in_timer = 0.0f;
            g_game_state_next = GS_RESTART;
        }

        if (draw_button("LEADERBOARD", BTN_X, leaderboard_y, BTN_W, BTN_H, 0.2f, 0.5f, 1.0f, s_tween_leaderboard))
        {
            open_leaderboard = true;
        }

        if (draw_button("MAIN MENU", BTN_X, menu_y, BTN_W, BTN_H, 1.0f, 0.5f, 0.1f, s_tween_menu))
        {
            is_dead = false;
            s_fade_in_timer = 0.0f;
            std::string blank = "";
            reset_player_name(blank);
            g_game_state_next = GS_MAIN_MENU;
        }
    }

    void draw_timeout_screen(const Data::Score& score, AEGfxVertexList* bg_mesh, bool& open_leaderboard)
    {
        (void)bg_mesh;
        ensure_mesh();

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();
        float dt = (float)AEFrameRateControllerGetFrameTime();

        s_fade_in_timer += dt;
        float fade_alpha = s_fade_in_timer / FADE_IN_DURATION;
        if (fade_alpha > 1.0f) fade_alpha = 1.0f;

        AEGfxSetCamPosition(0.0f, 0.0f);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(fade_alpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        mesh_renderer(s_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);

        if (Graphics::g_font_id < 0) return;

        const char* header = "TIME'S UP!";
        float hw, hh;
        AEGfxGetPrintSize(Graphics::g_font_id, header, 1.4f, &hw, &hh);
        AEGfxPrint(Graphics::g_font_id, header,
            -hw * 0.5f, 0.55f,
            1.4f, 1.0f, 0.85f, 0.1f, fade_alpha);

        std::string score_str = "SCORE:  " + std::to_string(score.score_value);
        float sw, sh;
        AEGfxGetPrintSize(Graphics::g_font_id, score_str.c_str(), 0.8f, &sw, &sh);
        AEGfxPrint(Graphics::g_font_id, score_str.c_str(),
            -sw * 0.5f, 0.3f,
            0.8f, 1.0f, 1.0f, 1.0f, fade_alpha);

        if (fade_alpha < 0.3f) return;

        constexpr float BTN_W = 350.0f;
        constexpr float BTN_H = 65.0f;
        constexpr float BTN_X = 0.0f;
        constexpr float GAP = 90.0f;

        float restart_y = 50.0f;
        float leader_y = restart_y - GAP;
        float menu_y = leader_y - GAP;

        if (draw_button("RESTART", BTN_X, restart_y, BTN_W, BTN_H, 0.2f, 0.8f, 0.2f, s_tween_restart))
            g_game_state_next = GS_RESTART;

        if (draw_button("LEADERBOARD", BTN_X, leader_y, BTN_W, BTN_H, 0.2f, 0.5f, 1.0f, s_tween_leaderboard))
        {
            LeaderboardSystem::read_time_leaderboard();
            open_leaderboard = true;
        }

        if (draw_button("MAIN MENU", BTN_X, menu_y, BTN_W, BTN_H, 1.0f, 0.5f, 0.1f, s_tween_menu))
        {
            std::string blank = "";
            reset_player_name(blank);
            g_game_state_next = GS_MAIN_MENU;
        }
    }
}