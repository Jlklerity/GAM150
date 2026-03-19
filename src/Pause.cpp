#include "pch.h"
#include "Pause.h"

namespace UiPauseMenu
{
    static bool inSettings = false;

    bool IsSettingsOpen()
    {
        return inSettings;
    }

    void Ui_DrawPauseMenu(bool& isPaused, AEGfxVertexList* bgMesh)
    {

        float winW = (float)AEGfxGetWindowWidth();                                                                                    // store window width
        float winH = (float)AEGfxGetWindowHeight();

        // --- Draw Background Quad (Dark Overlay) ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.6f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        Mesh_Renderer(bgMesh, 0.0f, 0.0f, winW, winH, 0.0f);

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
            float btnH = 80.0f;
            float btnX = 0.0f;

            // These are NDC positions (-1.0 to 1.0)
            float resumeY = 0.0f;
            float settingsY = -0.30f;
            float menuY = -0.60f;

            // half the screen height to convert NDC to World Space perfectly
            float screenHalfH = winH * 0.5f;

            // --- Draw Button Backgrounds ---
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.0f);

            // Multiply by screenHalfH instead of winH!
            Mesh_Renderer(bgMesh, btnX, resumeY * screenHalfH, btnW, btnH, 0.0f);
            Mesh_Renderer(bgMesh, btnX, settingsY * screenHalfH, btnW, btnH, 0.0f);
            Mesh_Renderer(bgMesh, btnX, menuY * screenHalfH, btnW, btnH, 0.0f);

            // --- Draw Button Text ---
            const char* text1 = "RESUME";
            const char* text2 = "SETTINGS";
            const char* text3 = "MAIN MENU";

            float w1, h1, w2, h2, w3, h3;
            AEGfxGetPrintSize(Graphics::font_id, text1, 0.7f, &w1, &h1);
            AEGfxGetPrintSize(Graphics::font_id, text2, 0.7f, &w2, &h2);
            AEGfxGetPrintSize(Graphics::font_id, text3, 0.7f, &w3, &h3);

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Text uses NDC natively, so this stays exactly as is!
            AEGfxPrint(Graphics::font_id, text1, btnX - (w1 * 0.5f), resumeY - 0.04f, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, text2, btnX - (w2 * 0.5f), settingsY - 0.04f, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, text3, btnX - (w3 * 0.5f), menuY - 0.04f, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);

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
            // Intercept normal pause menu if settings is open
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

            // Otherwise, draw the normal pause menu
            Ui_DrawPauseMenu(isPaused, bgMesh);
        }
    }
}