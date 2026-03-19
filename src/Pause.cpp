#include "pch.h"
#include "Pause.h"

namespace UiPauseMenu
{
    static bool inSettings = false;
    static bool ignoreNextClick = false;
    static bool inConfirmExit = false;

    bool IsSettingsOpen()
    {
        return inSettings;
    }

    void Ui_DrawPauseMenu(bool& isPaused, AEGfxVertexList* bgMesh)
    {
        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        float screenHalfH = winH * 0.5f;

        // --- Dark overlay ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.75f);
        AEGfxSetColorToMultiply(0.05f, 0.03f, 0.08f, 1.0f); // dark purple-black
        Mesh_Renderer(bgMesh, 0.0f, 0.0f, winW, winH, 0.0f);

        if (Graphics::font_id < 0) return;


        // --- Gold bordered panel behind menu ---
        // Gold border
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f); // gold
        Mesh_Renderer(bgMesh, 0.0f, -0.1f * screenHalfH, 420.0f, 620.0f, 0.0f);

        // Dark panel fill
        AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f); // dark purple
        Mesh_Renderer(bgMesh, 0.0f, -0.1f * screenHalfH, 408.0f, 608.0f, 0.0f);


        // --- Button config ---
        float btnW = 340.0f;
        float btnH = 75.0f;
        float btnX = 0.0f;
        float resumeY = 0.05f;
        float settingsY = -0.25f;
        float menuY = -0.55f;

        float borderSize = 5.0f;

        // Helper to draw a gold-bordered dark button
        auto DrawButton = [&](float cx, float worldY, const char* label)
            {
                // Gold border
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f); // gold
                Mesh_Renderer(bgMesh, cx, worldY, btnW + borderSize * 2, btnH + borderSize * 2, 0.0f);

                // Dark button fill
                AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f); // dark purple
                Mesh_Renderer(bgMesh, cx, worldY, btnW, btnH, 0.0f);

                // Button label
                float tw, th;
                AEGfxGetPrintSize(Graphics::font_id, label, 0.65f, &tw, &th);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                float ndcY = worldY / screenHalfH;
                AEGfxPrint(Graphics::font_id, label, cx / (winW * 0.5f) - tw * 0.5f, ndcY - 0.035f, 0.65f, 1.0f, 0.92f, 0.6f, 1.0f); // warm gold text
            };

        // --- "PAUSED" title ---
        {
            const char* title = "PAUSED";
            float tw, th;
            AEGfxGetPrintSize(Graphics::font_id, title, 1.1f, &tw, &th);
            // Shadow
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, title, -tw * 0.5f + 0.005f, 0.42f, 1.1f, 0.0f, 0.0f, 0.0f, 1.0f);
            // Gold title
            AEGfxPrint(Graphics::font_id, title, -tw * 0.5f, 0.43f, 1.1f, 1.0f, 0.82f, 0.1f, 1.0f);
        }

        // --- Decorative gold divider line under title ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        Mesh_Renderer(bgMesh, 0.0f, 0.27f * screenHalfH, 360.0f, 3.0f, 0.0f);

        // --- Draw buttons ---
        DrawButton(btnX, resumeY * screenHalfH, "RESUME");
        DrawButton(btnX, settingsY * screenHalfH, "SETTINGS");
        DrawButton(btnX, menuY * screenHalfH, "MAIN MENU");

            // ========================================================================
            // Input Handling
            // ========================================================================
            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                s32 mx, my;
                AEInputGetCursorPosition(&mx, &my);

                // Convert pure screen coordinates to Center-Origin World Coordinates
                float wx = (float)mx - (winW * 0.5f);
                float wy = (winH * 0.5f) - (float)my;

                float halfW = btnW * 0.5f; // Button Half Width
                float halfH = btnH * 0.5f; // Button Half Height

                // Check RESUME
                // FIX: Use screenHalfH here too so the clicks align with the boxes!
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                    wy >= ((resumeY * screenHalfH) - halfH) && wy <= ((resumeY * screenHalfH) + halfH))
                {
                    SoundManager::PlayClickBtnSfx();
                    isPaused = false;
                }

                // Check SETTINGS
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                    wy >= ((settingsY * screenHalfH) - halfH) && wy <= ((settingsY * screenHalfH) + halfH))
                {
                    SoundManager::PlayClickBtnSfx();
                    inSettings = true;
                }

                // Check MAIN MENU
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                    wy >= ((menuY * screenHalfH) - halfH) && wy <= ((menuY * screenHalfH) + halfH))
                {
                    SoundManager::PlayClickBtnSfx();
                    isPaused = false;
                    std::string temp_name = "";
                    ResetPlayerName(temp_name);
                    gGameStateNext = GS_MAIN_MENU;
                }
            }
        }
    }

    void Check_Paused(bool& isPaused, bool& isDead, AEGfxVertexList* bgMesh)
    {
        if (isPaused && !isDead)
        {
            if (inSettings)
            {
                if (SysSettings::DisplaySettings() == false)
                {
                    inSettings = false; // Close settings when user clicks "Close" or hits ESC
                }
                else {

                    return; // Stop drawing the normal pause menu
                }
            }
            Ui_DrawPauseMenu(isPaused, bgMesh);
        }
    }
}