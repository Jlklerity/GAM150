#include "pch.h"
#include "Shop.h"
#include "ButtonTween.h"


int g_shop_skin_id = 0;
int g_shop_bg_id = 0;

namespace
{
    static AEGfxTexture* s_tex_shop_bg = nullptr;
    static AEGfxTexture* s_tex_bg_options[3] = { nullptr };
    static AEGfxTexture* s_tex_skin_options[3] = { nullptr };
    static AEGfxTexture* s_tex_item_frame = nullptr;
    static AEGfxTexture* s_tex_bg_frame = nullptr;
    static AEGfxTexture* s_tex_back = nullptr;

    static AEGfxVertexList* s_mesh_back = nullptr;
    static AEGfxVertexList* s_mesh_shop = nullptr;
    static AEGfxVertexList* s_mesh_bg_item = nullptr;
    static AEGfxVertexList* s_mesh_skin_item = nullptr;

    static float s_win_w = 0.0f;
    static float s_win_h = 0.0f;
    static float s_world_x = 0.0f;
    static float s_world_y = 0.0f;

    static float s_shop_bg_width = 0.0f;
    static float s_shop_bg_height = 0.0f;
    static float s_back_btn_width = 0.0f;
    static float s_back_btn_height = 0.0f;

    static float s_bg_item_width = 0.0f;
    static float s_bg_item_height = 0.0f;
    static float s_skin_item_width = 0.0f;
    static float s_skin_item_height = 0.0f;

    static float s_bg_left_x = 0.0f;
    static float s_bg_top_y = 0.0f;
    static float s_text_x = 0.0f;
    static float s_text_y = 0.0f;
    static float s_offset_x = 0.0f;
    static float s_offset_y = 0.0f;

    constexpr float SHOP_BG_Y = 0.0f;
    constexpr float SHOP_BG_X = 0.0f;

    constexpr int SKIN_UNLOCK[3] = { 0, 1500, 3000 };
    constexpr int BG_UNLOCK[3] = { 0, 2000, 5000 };

    static float s_back_btn_x = 0.0f;
    static float s_back_btn_y = 0.0f;

    static float s_bg_item_x[3] = {};
    static float s_bg_item_y = 0.0f;

    static float s_skin_item_x[3] = {};
    static float s_skin_item_y = 0.0f;
    static int   s_score_amount = 0;

    static bool  s_is_transitioning_in = false;
    static bool  s_is_transitioning_out = false;
    static float s_shop_ui_offset_x = 0.0f;

    // ---------------------------------------------------------------------------
    // BUTTON TWEENS
    // ---------------------------------------------------------------------------
    static ButtonTween s_tween_back;
    static ButtonTween s_tween_bg_item[3];
    static ButtonTween s_tween_skin_item[3];
}


void display_score()
{
    if (Graphics::g_font_id < 0) return;

    s_bg_left_x = SHOP_BG_X - (s_shop_bg_width * 0.5f);
    s_bg_top_y = SHOP_BG_Y + (s_shop_bg_height * 0.5f);
    s_offset_x = s_shop_bg_width * 0.1f;
    s_offset_y = s_shop_bg_height * 0.27f;
    s_text_x = (s_bg_left_x + s_offset_x) / (s_win_w * 0.5f);
    s_text_y = (s_bg_top_y - s_offset_y) / (s_win_h * 0.5f);

    char score_buf[128];
    std::snprintf(score_buf, sizeof(score_buf), "SCORE: %d", s_score_amount);

    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxPrint(Graphics::g_font_id, score_buf, s_text_x + s_shop_ui_offset_x, s_text_y, 1.0f, 1, 1, 1, 1);
}

void shop_init()
{
    s_is_transitioning_in = true;
    s_shop_ui_offset_x = (float)AEGfxGetWindowWidth();
    ScreenTransitions::screen_transition_init();

    s_win_w = (float)AEGfxGetWindowWidth();
    s_win_h = (float)AEGfxGetWindowHeight();

    s_shop_bg_width = s_win_h * 1.2f;
    s_shop_bg_height = s_win_h * 1.0f;
    s_back_btn_width = 80.0f;
    s_back_btn_height = 80.0f;
    s_bg_item_width = s_win_w * 0.2f;
    s_bg_item_height = s_bg_item_width * (120.0f / 450.0f);
    s_skin_item_width = s_win_w * 0.25f;
    s_skin_item_height = s_skin_item_width;

    s_bg_item_y = s_win_h * 0.15f;
    s_skin_item_y = -s_win_h * 0.15f;
    s_back_btn_x = -s_win_w * 0.23f;
    s_back_btn_y = -s_win_h * 0.3f;

    // Backgrounds
    s_bg_item_x[0] = -s_win_w * 0.2f;
    s_bg_item_x[1] = 0.0f;
    s_bg_item_x[2] = s_win_w * 0.2f;

    // Skins
    s_skin_item_x[0] = -s_win_w * 0.20f;
    s_skin_item_x[1] = 0.0f;
    s_skin_item_x[2] = s_win_w * 0.20f;

    s_tex_shop_bg = AEGfxTextureLoad("Assets/shopBackground.png");
    AE_ASSERT_MESG(s_tex_shop_bg, "Failed to load shopBackground.png");

    s_tex_item_frame = AEGfxTextureLoad("Assets/shopItemFrame.png");
    AE_ASSERT_MESG(s_tex_item_frame, "Failed to load shopItemFrame.png");

    s_tex_back = AEGfxTextureLoad("Assets/LeftRightButtons.png");
    AE_ASSERT_MESG(s_tex_back, "Failed to load LeftRightButtons.png");

    s_tex_bg_frame = AEGfxTextureLoad("Assets/BackgroundSelectorFrame.png");
    AE_ASSERT_MESG(s_tex_bg_frame, "Failed to load BackgroundSelectorFrame.png");

    s_tex_bg_options[0] = AEGfxTextureLoad("Assets/Background1.png");
    AE_ASSERT_MESG(s_tex_bg_options, "Failed to load Background1.png");
    s_tex_bg_options[1] = AEGfxTextureLoad("Assets/floorground2.png");
    AE_ASSERT_MESG(s_tex_bg_options[1], "Failed to load floorground2shop.png");
    s_tex_bg_options[2] = s_tex_bg_options[0]; // TODO: Change to bg2.png later

    s_tex_skin_options[0] = AEGfxTextureLoad("Assets/skin1.png");
    AE_ASSERT_MESG(s_tex_skin_options, "Failed to load skin1.png");
    s_tex_skin_options[1] = AEGfxTextureLoad("Assets/skin2.png");
    AE_ASSERT_MESG(s_tex_skin_options, "Failed to load skin2.png");
    s_tex_skin_options[2] = AEGfxTextureLoad("Assets/skin3.png");
    AE_ASSERT_MESG(s_tex_skin_options, "Failed to load skin3.png");

    s_mesh_shop = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_bg_item = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_skin_item = Sprite_Mesh(1.0f, 1.0f);
    s_mesh_back = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);

    s_score_amount = LeaderboardSystem::get_highest_score();

    button_tween_reset(s_tween_back);
    for (int i = 0; i < 3; i++)
    {
        button_tween_reset(s_tween_bg_item[i]);
        button_tween_reset(s_tween_skin_item[i]);
    }
}

void shop_update()
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    float dt = (float)AEFrameRateControllerGetFrameTime();
    if (dt > 0.1f) dt = 0.01667f;
    BackgroundParallax::menu_parallax_update(dt);

    s32 mouse_x, mouse_y;
    AEInputGetCursorPosition(&mouse_x, &mouse_y);

    s_win_w = (float)AEGfxGetWindowWidth();
    s_win_h = (float)AEGfxGetWindowHeight();
    s_world_x = (float)mouse_x - (s_win_w * 0.5f);
    s_world_y = (s_win_h * 0.5f) - (float)mouse_y;

    // ---------------------------------------------------------------------------
    // BUTTON TWEEN UPDATES
    // ---------------------------------------------------------------------------
    if (!s_is_transitioning_in && !s_is_transitioning_out)
    {
        float dt_tween = (float)AEFrameRateControllerGetFrameTime();
        bool click = AEInputCheckTriggered(AEVK_LBUTTON);

        bool h_back = (s_world_x >= s_back_btn_x - (s_back_btn_width * 0.5f) &&
            s_world_x <= s_back_btn_x + (s_back_btn_width * 0.5f) &&
            s_world_y >= s_back_btn_y - (s_back_btn_height * 0.5f) &&
            s_world_y <= s_back_btn_y + (s_back_btn_height * 0.5f));
        button_tween_update(s_tween_back, h_back, h_back && click, dt_tween);

        for (int i = 0; i < 3; i++)
        {
            bool h_bg = (s_world_x >= s_bg_item_x[i] - (s_bg_item_width * 0.5f) &&
                s_world_x <= s_bg_item_x[i] + (s_bg_item_width * 0.5f) &&
                s_world_y >= s_bg_item_y - (s_bg_item_height * 0.5f) &&
                s_world_y <= s_bg_item_y + (s_bg_item_height * 0.5f));
            button_tween_update(s_tween_bg_item[i], h_bg, h_bg && click, dt_tween);

            float hit_box_w = s_skin_item_width * 0.65f;
            float hit_box_h = s_skin_item_height * 0.65f;
            float hit_box_x = s_skin_item_x[i] - 10.0f;
            float hit_box_y = s_skin_item_y + 60.0f;
            bool h_skin = (s_world_x >= hit_box_x - (hit_box_w * 0.5f) &&
                s_world_x <= hit_box_x + (hit_box_w * 0.5f) &&
                s_world_y >= hit_box_y - (hit_box_h * 0.5f) &&
                s_world_y <= hit_box_y + (hit_box_h * 0.5f));
            button_tween_update(s_tween_skin_item[i], h_skin, h_skin && click, dt_tween);
        }
    }

    if (s_is_transitioning_in)
    {
        ScreenTransitions::screen_transition_update(dt);
        float smooth_pan = ScreenTransitions::ease_out_expo(ScreenTransitions::result);

        s_shop_ui_offset_x = s_win_w * (1.0f - smooth_pan);
        BackgroundParallax::menu_parallax_set_pan_offset(-0.2f - (0.2f * smooth_pan));

        if (ScreenTransitions::result >= 0.3f)
        {
            s_shop_ui_offset_x = 0.0f;
            s_is_transitioning_in = false;
            BackgroundParallax::menu_parallax_set_pan_offset(-0.4f);
        }
    }
    BackgroundParallax::menu_parallax_draw(0.0f);

    if (!s_is_transitioning_in && AEInputCheckTriggered(AEVK_ESCAPE))
    {
        s_is_transitioning_out = true;
        ScreenTransitions::screen_transition_init();
    }

    if (!s_is_transitioning_in && AEInputCheckTriggered(AEVK_LBUTTON))
    {
        // Check Back Button
        if (s_world_x >= s_back_btn_x - (s_back_btn_width * 0.5f) &&
            s_world_x <= s_back_btn_x + (s_back_btn_width * 0.5f) &&
            s_world_y >= s_back_btn_y - (s_back_btn_height * 0.5f) &&
            s_world_y <= s_back_btn_y + (s_back_btn_height * 0.5f))
        {
            SoundManager::play_click_btn_sfx();
            s_is_transitioning_out = true;
            ScreenTransitions::screen_transition_init();
        }

        // --- CHECK BACKGROUND SELECTION ---
        for (int i = 0; i < 3; i++)
        {
            if (s_world_x >= s_bg_item_x[i] - (s_bg_item_width * 0.5f) &&
                s_world_x <= s_bg_item_x[i] + (s_bg_item_width * 0.5f) &&
                s_world_y >= s_bg_item_y - (s_bg_item_height * 0.5f) &&
                s_world_y <= s_bg_item_y + (s_bg_item_height * 0.5f))
            {
                SoundManager::play_click_btn_sfx();
                if (s_score_amount >= BG_UNLOCK[i])
                    g_shop_bg_id = i;
            }
        }

        // --- CHECK SKIN SELECTION ---
        for (int i = 0; i < 3; i++)
        {
            float hit_box_w = s_skin_item_width * 0.65f;
            float hit_box_h = s_skin_item_height * 0.65f;
            float hit_box_x = s_skin_item_x[i] - 10.0f;
            float hit_box_y = s_skin_item_y + 60.0f;

            if (s_world_x >= hit_box_x - (hit_box_w * 0.5f) &&
                s_world_x <= hit_box_x + (hit_box_w * 0.5f) &&
                s_world_y >= hit_box_y - (hit_box_h * 0.5f) &&
                s_world_y <= hit_box_y + (hit_box_h * 0.5f))
            {
                SoundManager::play_click_btn_sfx();
                if (s_score_amount >= SKIN_UNLOCK[i])
                    g_shop_skin_id = i;
            }
        }
    }

    if (s_is_transitioning_out)
    {
        ScreenTransitions::screen_transition_update(dt);
        float smooth_pan = ScreenTransitions::ease_out_expo(ScreenTransitions::result);

        s_shop_ui_offset_x = s_win_w * smooth_pan;
        BackgroundParallax::menu_parallax_set_pan_offset(-0.4f + (0.2f * smooth_pan));

        if (ScreenTransitions::result >= 0.3f)
        {
            s_is_transitioning_out = false;
            BackgroundParallax::menu_parallax_set_pan_offset(0.2f);
            g_game_state_next = GS_MAIN_MENU;
        }
    }

    if (s_tex_shop_bg)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_tex_shop_bg, 0.0f, 0.0f);
        mesh_renderer(s_mesh_shop, SHOP_BG_X + s_shop_ui_offset_x, SHOP_BG_Y, s_shop_bg_width, s_shop_bg_height, 0.0f);
    }

    // Draw the 3 Background Options
    for (int i = 0; i < 3; i++)
    {
        if (s_tex_bg_options[i])
        {
            if (s_score_amount >= BG_UNLOCK[i])
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Bright (Unlocked)
            else
                AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f); // Dark (Locked)

            AEGfxTextureSet(s_tex_bg_options[i], 0.0f, 0.0f);
            mesh_renderer(s_mesh_bg_item,
                s_bg_item_x[i] + s_shop_ui_offset_x, s_bg_item_y,
                s_bg_item_width * 0.7f * s_tween_bg_item[i].scale,
                s_bg_item_height * 0.7f * s_tween_bg_item[i].scale, 0.0f);

            if (g_shop_bg_id == i && s_tex_bg_frame)
            {
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxTextureSet(s_tex_bg_frame, 0.0f, 0.0f);
                mesh_renderer(s_mesh_bg_item,
                    s_bg_item_x[i] + s_shop_ui_offset_x, s_bg_item_y,
                    s_bg_item_width * 0.7f * s_tween_bg_item[i].scale,
                    s_bg_item_height * 0.7f * s_tween_bg_item[i].scale, 0.0f);
            }

            if ((s_score_amount < BG_UNLOCK[i]) && Graphics::g_font_id >= 0)
            {
                char price_str[32];
                std::snprintf(price_str, sizeof(price_str), "%d", BG_UNLOCK[i]);

                float text_scale = 0.8f;
                float text_w = 0.0f, text_h = 0.0f;
                AEGfxGetPrintSize(Graphics::g_font_id, price_str, text_scale, &text_w, &text_h);

                float text_ndc_x = (s_bg_item_x[i] / (s_win_w * 0.5f)) - (text_w * 0.5f);
                float text_ndc_y = (s_bg_item_y / (s_win_h * 0.5f)) - 0.025f;
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::g_font_id, price_str,
                    text_ndc_x + s_shop_ui_offset_x, text_ndc_y,
                    text_scale, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    // Draw the 3 Skin Options
    for (int i = 0; i < 3; i++)
    {
        if (s_tex_skin_options[i])
        {
            if (s_score_amount >= SKIN_UNLOCK[i])
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Bright (Unlocked)
            else
                AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f); // Dark (Locked)

            AEGfxTextureSet(s_tex_skin_options[i], 0.0f, 0.0f);
            mesh_renderer(s_mesh_skin_item,
                s_skin_item_x[i] + s_shop_ui_offset_x, s_skin_item_y,
                s_skin_item_width * s_tween_skin_item[i].scale,
                s_skin_item_height * s_tween_skin_item[i].scale, 0.0f);

            if (g_shop_skin_id == i && s_tex_item_frame)
            {
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxTextureSet(s_tex_item_frame, 0.0f, 0.0f);
                constexpr float FRAME_SCALE = 0.65f;
                mesh_renderer(s_mesh_skin_item,
                    s_skin_item_x[i] - 10.0f + s_shop_ui_offset_x, s_skin_item_y + 60.0f,
                    s_skin_item_width * FRAME_SCALE * s_tween_skin_item[i].scale,
                    s_skin_item_height * FRAME_SCALE * s_tween_skin_item[i].scale, 0.0f);
            }

            if ((s_score_amount < SKIN_UNLOCK[i]) && Graphics::g_font_id >= 0)
            {
                char price_str[32];
                std::snprintf(price_str, sizeof(price_str), "%d", SKIN_UNLOCK[i]);

                float text_scale = 0.8f;
                float text_w = 0.0f, text_h = 0.0f;
                AEGfxGetPrintSize(Graphics::g_font_id, price_str, text_scale, &text_w, &text_h);

                float text_ndc_x = (s_skin_item_x[i] / (s_win_w * 0.5f)) - (text_w * 0.5f);
                float text_ndc_y = ((s_skin_item_y + 60.0f) / (s_win_h * 0.5f)) - 0.025f;
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::g_font_id, price_str,
                    text_ndc_x + s_shop_ui_offset_x, text_ndc_y,
                    text_scale, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
    }

    if (s_mesh_back && s_tex_back)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_tex_back, 0.0f, 0.0f);
        mesh_renderer(s_mesh_back,
            s_back_btn_x + s_shop_ui_offset_x, s_back_btn_y,
            s_back_btn_width * s_tween_back.scale,
            s_back_btn_height * s_tween_back.scale, 0.0f);
    }

    display_score();
}

const char* shop_get_active_skin_path()
{
    if (g_shop_skin_id == 0) return "Assets/characterSheet.png";
    if (g_shop_skin_id == 1) return "Assets/character2Sheet.png";
    if (g_shop_skin_id == 2) return "Assets/catskin-Sheet.png";
    return "Assets/characterSheet.png"; // Safe fallback
}

void shop_exit()
{
    if (g_game_state_next != GS_MAIN_MENU)
        BackgroundParallax::menu_parallax_exit();

    if (s_tex_shop_bg) { AEGfxTextureUnload(s_tex_shop_bg);  s_tex_shop_bg = nullptr; }
    if (s_tex_item_frame) { AEGfxTextureUnload(s_tex_item_frame); s_tex_item_frame = nullptr; }
    if (s_tex_bg_frame) { AEGfxTextureUnload(s_tex_bg_frame); s_tex_bg_frame = nullptr; }

    // Null out aliases BEFORE unloading to prevent double-free
    if (s_tex_bg_options[1] == s_tex_bg_options[0]) s_tex_bg_options[1] = nullptr;
    if (s_tex_bg_options[2] == s_tex_bg_options[0]) s_tex_bg_options[2] = nullptr;
    if (s_tex_bg_options[0]) { AEGfxTextureUnload(s_tex_bg_options[0]); s_tex_bg_options[0] = nullptr; }
    if (s_tex_bg_options[1]) { AEGfxTextureUnload(s_tex_bg_options[1]); s_tex_bg_options[1] = nullptr; }
    if (s_tex_bg_options[2]) { AEGfxTextureUnload(s_tex_bg_options[2]); s_tex_bg_options[2] = nullptr; }

    if (s_tex_skin_options[2] == s_tex_skin_options[0]) s_tex_skin_options[2] = nullptr;
    if (s_tex_skin_options[0]) { AEGfxTextureUnload(s_tex_skin_options[0]); s_tex_skin_options[0] = nullptr; }
    if (s_tex_skin_options[1]) { AEGfxTextureUnload(s_tex_skin_options[1]); s_tex_skin_options[1] = nullptr; }
    if (s_tex_skin_options[2]) { AEGfxTextureUnload(s_tex_skin_options[2]); s_tex_skin_options[2] = nullptr; }

    if (s_tex_back) { AEGfxTextureUnload(s_tex_back);      s_tex_back = nullptr; }
    if (s_mesh_shop) { AEGfxMeshFree(s_mesh_shop);          s_mesh_shop = nullptr; }
    if (s_mesh_back) { AEGfxMeshFree(s_mesh_back);          s_mesh_back = nullptr; }
    if (s_mesh_bg_item) { AEGfxMeshFree(s_mesh_bg_item);       s_mesh_bg_item = nullptr; }
    if (s_mesh_skin_item) { AEGfxMeshFree(s_mesh_skin_item);     s_mesh_skin_item = nullptr; }
}