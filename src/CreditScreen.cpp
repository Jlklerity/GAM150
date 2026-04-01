#include "pch.h"
#include "CreditScreen.h"


namespace
{
    static float              s_scroll_y = 0.0f;
    static float              s_scroll_speed = 0.35f;
    static AEGfxVertexList* s_mesh_credits_logo = nullptr;
    static AEGfxTexture* s_tex_credits_logo = nullptr;

    constexpr float LOGO_WIDTH = 1000.0f;
    constexpr float LOGO_HEIGHT = 600.0f;

    const char* s_credit_lines[] = {
        "BY",
        "FAREWELL YK",
        "",
        "All content (c) 2026 DigiPen Institute of Technology Singapore.",
        "All Rights Reserved.",
        "",
        "PROGRAMMERS",
        "ZI YUAN",
        "PENG FEI",
        "AZFAR",
        "JEREMY LIM",
        "",
        "ART DESIGNERS",
        "PENG FEI",
        "AZFAR",
        "",
        "SOUND DESIGNERS",
        "ZI YUAN",
        "PENG FEI",
        "AZFAR",
        "JEREMY LIM",
        "",
        "TOOLS AND LIBRARIES",
        "ALPHA ENGINE",
        "metadigger",
        "aseprite on steam",
        "piskel",
        "",
        "FACULTY AND ADVISORS",
        "GERALD WONG HAN FENG",
        "DR. SOROOR MALEKMOHAMMADAI FARADOUNBEH",
        "TOMMY TAN CHEE WEI",
        "",
        "",
        "CREATED AT DIGIPEN INSTITUTE OF TECHNOLOGY SINGAPORE",
        "",
        "",
        "PRESIDENT",
        "Claude Comair",
        "",
        "EXECUTIVES",
        "Jason Chu",
        "Samir Abou Samra",
        "Michele Comair",
        "Angela Kugler",
        "Erik Mohrmann",
        "Benjamin Ellinger",
        "Melvin Gonsalvez",
        "",
        "THANK YOU FOR PLAYING!"
    };
    const int NUM_CREDIT_LINES = sizeof(s_credit_lines) / sizeof(s_credit_lines[0]);
}

void credit_screen_init()
{
    if (Graphics::g_font_id < 0)
        Graphics::g_font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 48);

    s_tex_credits_logo = AEGfxTextureLoad("Assets/title.png");
    s_mesh_credits_logo = Sprite_Mesh(1.0f, 1.0f);
    s_scroll_y = -1.2f;
}

void credit_screen_update()
{
    float dt = (float)AEFrameRateControllerGetFrameTime();
    if (dt > 0.1f) dt = 0.01667f;

    // ---------------------------------------------------------
    //  INPUT
    // ---------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON))
    {
        SoundManager::play_click_btn_sfx();
        g_game_state_next = GS_MAIN_MENU;
    }

    // ---------------------------------------------------------
    //  SCROLLING LOGIC
    // ---------------------------------------------------------
    s_scroll_y += s_scroll_speed * dt;

    constexpr float LINE_SPACING = 0.15f;

    float last_line_y = s_scroll_y - ((NUM_CREDIT_LINES - 1) * LINE_SPACING);
    if (last_line_y > 1.2f)
        g_game_state_next = GS_MAIN_MENU;

    // ---------------------------------------------------------
    //  RENDER
    // ---------------------------------------------------------
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    if (s_tex_credits_logo && s_mesh_credits_logo)
    {
        float win_h = (float)AEGfxGetWindowHeight();
        float logo_world_y = (s_scroll_y + 0.3f) * (win_h * 0.5f);
        float logo_world_x = 10.0f;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_tex_credits_logo, 0.0f, 0.0f);
        mesh_renderer(s_mesh_credits_logo, logo_world_x, logo_world_y, LOGO_WIDTH, LOGO_HEIGHT, 0.0f);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    }

    if (Graphics::g_font_id >= 0)
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        for (int i = 0; i < NUM_CREDIT_LINES; ++i)
        {
            std::string line = s_credit_lines[i];
            if (line.empty()) continue;

            float text_w = 0.0f, text_h = 0.0f;
            constexpr float SCALE = 1.0f;

            AEGfxGetPrintSize(Graphics::g_font_id, s_credit_lines[i], SCALE, &text_w, &text_h);
            float ndc_x = 0.0f - (text_w * 0.5f);
            float ndc_y = s_scroll_y - (i * LINE_SPACING);

            if (ndc_y > -1.2f && ndc_y < 1.2f)
            {
                if (line == "TEAM MEMBERS" || line == "FACULTY AND ADVISORS" ||
                    line == "PROGRAMMERS" || line == "ART DESIGNERS" ||
                    line == "SOUND DESIGNERS" || line == "SPECIAL THANKS" ||
                    line == "TOOLS AND LIBRARIES" || line == "PRESIDENT" ||
                    line == "EXECUTIVES")
                {
                    AEGfxPrint(Graphics::g_font_id, s_credit_lines[i], ndc_x, ndc_y, SCALE, 1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                }
                else
                {
                    AEGfxPrint(Graphics::g_font_id, s_credit_lines[i], ndc_x, ndc_y, SCALE, 1.0f, 1.0f, 1.0f, 1.0f); // White
                }
            }
        }
    }
}

void credit_screen_exit()
{
    if (Graphics::g_font_id >= 0)
    {
        AEGfxDestroyFont(Graphics::g_font_id);
        Graphics::g_font_id = -1;
    }
    if (s_mesh_credits_logo) { AEGfxMeshFree(s_mesh_credits_logo);          s_mesh_credits_logo = nullptr; }
    if (s_tex_credits_logo) { AEGfxTextureUnload(s_tex_credits_logo);       s_tex_credits_logo = nullptr; }
}