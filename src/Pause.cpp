#include "AEEngine.h"
#include "Enemy.h"
#include "MeshRenderer.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Settings.h"
#include "SoundManager.h"

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
            // EXIT CONFIRMATION POPUP OVERLAY
            // ========================================================================
            if (inConfirmExit)
            {
                float screenHalfW = winW * 0.5f;

                // 1. Draw extra dark overlay to dim the pause menu
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(0.7f);
                AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
                Mesh_Renderer(bgMesh, 0.0f, 0.0f, winW, winH, 0.0f);

                // 2. Draw Popup Box
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(0.15f, 0.15f, 0.15f, 1.0f); // Dark Grey Panel
                Mesh_Renderer(bgMesh, 0.0f, 0.0f, 600.0f, 300.0f, 0.0f);

                // 3. Draw YES and NO Buttons
                float confBtnW = 150.0f;
                float confBtnH = 60.0f;
                float yesX = -120.0f;
                float noX = 120.0f;
                float confBtnY = -50.0f;

                AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f); // Lighter Grey Buttons
                Mesh_Renderer(bgMesh, yesX, confBtnY, confBtnW, confBtnH, 0.0f);
                Mesh_Renderer(bgMesh, noX, confBtnY, confBtnW, confBtnH, 0.0f);

                // 4. Draw Text
                const char* promptText = "EXIT TO MAIN MENU?";
                const char* yesText = "YES";
                const char* noText = "NO";

                float pw, ph, yw, yh, nw, nh;
                AEGfxGetPrintSize(Graphics::font_id, promptText, 0.8f, &pw, &ph);
                AEGfxGetPrintSize(Graphics::font_id, yesText, 0.7f, &yw, &yh);
                AEGfxGetPrintSize(Graphics::font_id, noText, 0.7f, &nw, &nh);

                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

                // Align text perfectly inside the popup
                AEGfxPrint(Graphics::font_id, promptText, -(pw * 0.5f), (80.0f / screenHalfH) - (ph * 0.5f), 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxPrint(Graphics::font_id, yesText, (yesX / screenHalfW) - (yw * 0.5f), (confBtnY / screenHalfH) - (yh * 0.5f), 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxPrint(Graphics::font_id, noText, (noX / screenHalfW) - (nw * 0.5f), (confBtnY / screenHalfH) - (nh * 0.5f), 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);

                // 5. Input Handling strictly for the Popup
                if (ignoreNextClick)
                {
                    ignoreNextClick = false;
                }
                else if (AEInputCheckTriggered(AEVK_LBUTTON))
                {
                    s32 mx, my;
                    AEInputGetCursorPosition(&mx, &my);
                    float wx = (float)mx - screenHalfW;
                    float wy = screenHalfH - (float)my;

                    float halfConfW = confBtnW * 0.5f;
                    float halfConfH = confBtnH * 0.5f;

                    // Clicked YES
                    if (wx >= (yesX - halfConfW) && wx <= (yesX + halfConfW) &&
                        wy >= (confBtnY - halfConfH) && wy <= (confBtnY + halfConfH))
                    {
                        SoundManager::PlayClickBtnSfx();
                        isPaused = false;
                        std::string temp_name = "";
                        ResetPlayerName(temp_name);
                        gGameStateNext = GS_MAIN_MENU;
                        inConfirmExit = false;
                    }
                    // Clicked NO
                    else if (wx >= (noX - halfConfW) && wx <= (noX + halfConfW) &&
                        wy >= (confBtnY - halfConfH) && wy <= (confBtnY + halfConfH))
                    {
                        SoundManager::PlayClickBtnSfx();
                        inConfirmExit = false;
                        ignoreNextClick = true; // Use our trick to prevent click-through!
                    }
                }

                return; 
            }

            // ========================================================================
            // Input Handling
            // ========================================================================
            if (ignoreNextClick)
            {
                ignoreNextClick = false;
            }
            else if (AEInputCheckTriggered(AEVK_LBUTTON))
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
                    ignoreNextClick = true;
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