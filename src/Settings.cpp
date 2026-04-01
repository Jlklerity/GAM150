#include "pch.h"
#include "Settings.h"
#include "ButtonTween.h"

namespace
{
    static AEGfxVertexList* s_quad = nullptr;

    static float s_bgm_vol = 0.5f;
    static float s_sfx_vol = 0.8f;
    static float s_amb_vol = 0.6f;

    static int s_dragging_slider = -1;

    constexpr float SLIDER_W = 400.0f;
    constexpr float SLIDER_H = 15.0f;
    constexpr float KNOB_W = 20.0f;
    constexpr float KNOB_H = 40.0f;

    constexpr float BGM_Y = 120.0f;
    constexpr float SFX_Y = -10.0f;
    constexpr float AMB_Y = -140.0f;

    constexpr float CLOSE_W = 200.0f;
    constexpr float CLOSE_H = 70.0f;
    constexpr float CLOSE_Y = -300.0f;

    constexpr float BORDER = 5.0f;

    static ButtonTween s_tween_close;

    static float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

    static void draw_centred_text(const char* text, float ndc_y, float scale,
        float r, float g, float b, float alpha)
    {
        if (Graphics::g_font_id < 0) return;
        float w, h;
        AEGfxGetPrintSize(Graphics::g_font_id, text, scale, &w, &h);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::g_font_id, text, -w * 0.5f, ndc_y - (h * 0.5f), scale, r, g, b, alpha);
    }
}

namespace SysSettings
{
    void init()
    {
        if (!s_quad)
            s_quad = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

        s_bgm_vol = SoundManager::get_bgm_volume();
        s_sfx_vol = SoundManager::get_sfx_volume();
        s_amb_vol = SoundManager::get_ambient_volume();
        button_tween_reset(s_tween_close);
    }

    bool display_settings()
    {
        if (!s_quad) init();

        s32 mouse_x, mouse_y;
        AEInputGetCursorPosition(&mouse_x, &mouse_y);
        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();
        float world_x = (float)mouse_x - (win_w * 0.5f);
        float world_y = (win_h * 0.5f) - (float)mouse_y;
        float ndc_scale = win_h > 0 ? (2.0f / win_h) : 0.0022f;

        bool mouse_held = (AEInputCheckCurr(AEVK_LBUTTON) != 0);
        bool mouse_triggered = (AEInputCheckTriggered(AEVK_LBUTTON) != 0);
        float start_x = -(SLIDER_W * 0.5f);

        if (!mouse_held) s_dragging_slider = -1;

        // Close button tween
        {
            float dt = (float)AEFrameRateControllerGetFrameTime();
            bool h_close = (world_x >= -(CLOSE_W * 0.5f) && world_x <= (CLOSE_W * 0.5f) &&
                world_y >= CLOSE_Y - (CLOSE_H * 0.5f) && world_y <= CLOSE_Y + (CLOSE_H * 0.5f));
            button_tween_update(s_tween_close, h_close, h_close && mouse_triggered, dt);
        }

        // Close button / ESC
        if ((mouse_triggered && s_dragging_slider == -1 &&
            world_x >= -(CLOSE_W * 0.5f) && world_x <= (CLOSE_W * 0.5f) &&
            world_y >= CLOSE_Y - (CLOSE_H * 0.5f) && world_y <= CLOSE_Y + (CLOSE_H * 0.5f))
            || AEInputCheckTriggered(AEVK_ESCAPE))
        {
            SoundManager::play_click_btn_sfx();
            return false;
        }

        // Grab slider
        if (mouse_triggered)
        {
            float hit_box_w = SLIDER_W + KNOB_W;
            if (world_x >= -(hit_box_w * 0.5f) && world_x <= (hit_box_w * 0.5f))
            {
                if (world_y >= BGM_Y - KNOB_H && world_y <= BGM_Y + KNOB_H) s_dragging_slider = 0;
                else if (world_y >= SFX_Y - KNOB_H && world_y <= SFX_Y + KNOB_H) s_dragging_slider = 1;
                else if (world_y >= AMB_Y - KNOB_H && world_y <= AMB_Y + KNOB_H) s_dragging_slider = 2;
            }
        }

        // Apply drag
        if (s_dragging_slider != -1)
        {
            float percent = clamp01((world_x - start_x) / SLIDER_W);
            if (s_dragging_slider == 0) { s_bgm_vol = percent; SoundManager::set_bgm_volume(s_bgm_vol); }
            else if (s_dragging_slider == 1) { s_sfx_vol = percent; SoundManager::set_sfx_volume(s_sfx_vol); }
            else if (s_dragging_slider == 2) { s_amb_vol = percent; SoundManager::set_ambient_volume(s_amb_vol); }
        }

        // ----------------------------------------------------------------
        // RENDERING
        // ----------------------------------------------------------------

        // Fullscreen dark purple overlay
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.75f);
        AEGfxSetColorToMultiply(0.05f, 0.03f, 0.08f, 1.0f);
        mesh_renderer(s_quad, 0.0f, 0.0f, win_w, win_h, 0.0f);

        // Gold border panel
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        mesh_renderer(s_quad, 0.0f, -20.0f, 620.0f, 820.0f, 0.0f);

        // Dark purple panel fill
        AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f);
        mesh_renderer(s_quad, 0.0f, -20.0f, 608.0f, 808.0f, 0.0f);

        // Gold divider under title
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        mesh_renderer(s_quad, 0.0f, 260.0f, 540.0f, 3.0f, 0.0f);

        // Title shadow then gold
        draw_centred_text("SOUND SETTINGS", 0.68f, 0.9f, 0.0f, 0.0f, 0.0f, 1.0f);
        draw_centred_text("SOUND SETTINGS", 0.69f, 0.9f, 1.0f, 0.82f, 0.1f, 1.0f);

        // Sliders
        auto draw_slider = [&](float y_pos, float current_vol, const char* label)
            {
                draw_centred_text(label, (y_pos + 50.0f) * ndc_scale, 0.55f, 0.85f, 0.70f, 0.15f, 1.0f);

                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                mesh_renderer(s_quad, 0.0f, y_pos, SLIDER_W + BORDER * 2, SLIDER_H + BORDER * 2, 0.0f);

                AEGfxSetColorToMultiply(0.12f, 0.10f, 0.18f, 1.0f);
                mesh_renderer(s_quad, 0.0f, y_pos, SLIDER_W, SLIDER_H, 0.0f);

                float fill_w = SLIDER_W * current_vol;
                float fill_x = start_x + (fill_w * 0.5f);
                if (fill_w > 0.0f)
                {
                    AEGfxSetColorToMultiply(0.95f, 0.78f, 0.10f, 1.0f);
                    mesh_renderer(s_quad, fill_x, y_pos, fill_w, SLIDER_H, 0.0f);
                }

                float knob_x = start_x + fill_w;
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                mesh_renderer(s_quad, knob_x, y_pos, KNOB_W + BORDER, KNOB_H + BORDER, 0.0f);
                AEGfxSetColorToMultiply(0.15f, 0.12f, 0.20f, 1.0f);
                mesh_renderer(s_quad, knob_x, y_pos, KNOB_W, KNOB_H, 0.0f);
            };

        draw_slider(BGM_Y, s_bgm_vol, "MUSIC VOLUME");
        draw_slider(SFX_Y, s_sfx_vol, "SFX VOLUME");
        draw_slider(AMB_Y, s_amb_vol, "AMBIENT VOLUME");

        // Close button scaled by tween
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        mesh_renderer(s_quad, 0.0f, CLOSE_Y,
            (CLOSE_W + BORDER * 2) * s_tween_close.scale,
            (CLOSE_H + BORDER * 2) * s_tween_close.scale, 0.0f);
        AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f);
        mesh_renderer(s_quad, 0.0f, CLOSE_Y,
            CLOSE_W * s_tween_close.scale,
            CLOSE_H * s_tween_close.scale, 0.0f);

        draw_centred_text("CLOSE", CLOSE_Y * ndc_scale, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);

        return true;
    }

    void cleanup()
    {
        if (s_quad) { AEGfxMeshFree(s_quad); s_quad = nullptr; }
    }
}