#include "AEEngine.h"
#include "Enemy.h"
#include "MeshRenderer.h"
#include "MainMenu.h"
#include "GameStateManager.h"

namespace UiPauseMenu
{
    void Ui_DrawPauseMenu(bool& isPaused, AEGfxVertexList* bgMesh) { //for game pause menu (jeremy)
        if (Graphics::font_id >= 0)
        {
            // --- Draw "PAUSED" Text ---
            const char* pauseText = "PAUSED";
            float w, h;
            AEGfxGetPrintSize(Graphics::font_id, pauseText, 1.0f, &w, &h);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, pauseText, -w * 0.5f, 0.25f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

            // ========================================================================
            // Button Configuration
            // ========================================================================
            float btnW = 300.0f;
            float btnH = 60.0f;
            float btnX = 0.0f; // Centered Horizontally

            // Y Positions
            float resumeY = -50.0f;
            float menuY = -150.0f;

            // ========================================================================
            // Draw "RESUME" Button
            // ========================================================================
            // 1. Background
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
            Mesh_Renderer(bgMesh, btnX, resumeY, btnW, btnH, 0.0f);

            // 2. Text
            const char* resumeText = "RESUME";
            float rw, rh;
            AEGfxGetPrintSize(Graphics::font_id, resumeText, 0.6f, &rw, &rh);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            // -0.15f Y is approx -50.0f world units
            AEGfxPrint(Graphics::font_id, resumeText, -rw * 0.5f, -0.15f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

            // ========================================================================
            // Draw "MAIN MENU" Button
            // ========================================================================
            // 1. Background
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
            Mesh_Renderer(bgMesh, btnX, menuY, btnW, btnH, 0.0f);

            // 2. Text
            const char* menuText = "MAIN MENU";
            float mw, mh;
            AEGfxGetPrintSize(Graphics::font_id, menuText, 0.6f, &mw, &mh);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            // -0.37f Y is approx -150.0f world units
            AEGfxPrint(Graphics::font_id, menuText, -mw * 0.5f, -0.37f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

            // ========================================================================
            // Handle Mouse Clicks
            // ========================================================================
            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                s32 mx, my;
                AEInputGetCursorPosition(&mx, &my);

                float wx = (float)mx - 800.0f;
                float wy = 450.0f - (float)my;

                float halfW = btnW * 0.5f;
                float halfH = btnH * 0.5f;

                // Check RESUME
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                    wy >= (resumeY - halfH) && wy <= (resumeY + halfH))
                {
                    isPaused = false; // Just unpause
                }

                // Check MAIN MENU
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                    wy >= (menuY - halfH) && wy <= (menuY + halfH))
                {
                    isPaused = false;
                    std::string temp_name = "";
                    ResetPlayerName(temp_name);
                    gGameStateNext = GS_MAIN_MENU;
                }
            }
        }
    }

    void Check_Paused(bool& isPaused, bool& isDead, AEGfxVertexList* bgMesh) {
        if (isPaused && !isDead)
        {
           Ui_DrawPauseMenu(isPaused, bgMesh);
        }
    }
}