#include "pch.h"
#include "Pause.h"

namespace UiPauseMenu
{
    static bool inSettings = false;
    static bool ignoreNextClick = false;
    static bool inConfirmExit = false;
    static bool isConfirmingRestart = false;

    bool IsSettingsOpen()
    {
        return inSettings;
    }

    void Ui_DrawPauseMenu(bool& isPaused, AEGfxVertexList* bgMesh)
    {
        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        float screenHalfW = winW * 0.5f;
        float screenHalfH = winH * 0.5f;

        // --- Dark overlay ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.75f);
        AEGfxSetColorToMultiply(0.05f, 0.03f, 0.08f, 1.0f);
        Mesh_Renderer(bgMesh, 0.0f, 0.0f, winW, winH, 0.0f);

        if (Graphics::font_id < 0) return;

        // --- Centered Gold bordered panel ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        Mesh_Renderer(bgMesh, 0.0f, -0.05f * screenHalfH, 440.0f, 780.0f, 0.0f);

        // Dark panel fill
        AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f);
        Mesh_Renderer(bgMesh, 0.0f, -0.05f * screenHalfH, 428.0f, 768.0f, 0.0f);

        // --- Button config ---
        float btnW = 340.0f;
        float btnH = 75.0f;
        float btnX = 0.0f;

        float resumeY = 0.25f;
        float restartY = -0.03f;
        float settingsY = -0.31f;
        float menuY = -0.59f;

        float borderSize = 5.0f;

        auto DrawButton = [&](float cx, float worldY, const char* label)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                Mesh_Renderer(bgMesh, cx, worldY, btnW + borderSize * 2, btnH + borderSize * 2, 0.0f);

                AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f);
                Mesh_Renderer(bgMesh, cx, worldY, btnW, btnH, 0.0f);

                float tw, th;
                AEGfxGetPrintSize(Graphics::font_id, label, 0.65f, &tw, &th);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

                float ndcY = worldY / screenHalfH;
                AEGfxPrint(Graphics::font_id, label,
                    cx / screenHalfW - tw * 0.5f,
                    ndcY - 0.035f,
                    0.65f, 1.0f, 0.92f, 0.6f, 1.0f);
            };

        // --- PAUSED ---
        {
            const char* title = "PAUSED";
            float tw, th;
            AEGfxGetPrintSize(Graphics::font_id, title, 1.1f, &tw, &th);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);

            // Shadow
            AEGfxPrint(Graphics::font_id, title,
                -tw * 0.5f + 0.005f,
                0.52f,
                1.1f, 0.0f, 0.0f, 0.0f, 1.0f);

            // Gold text
            AEGfxPrint(Graphics::font_id, title,
                -tw * 0.5f,
                0.53f,
                1.1f, 1.0f, 0.82f, 0.1f, 1.0f);
        }

        // --- Divider ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        Mesh_Renderer(bgMesh, 0.0f, 0.45f * screenHalfH, 360.0f, 3.0f, 0.0f);

        // --- Buttons ---
        DrawButton(btnX, resumeY * screenHalfH, "RESUME");
        DrawButton(btnX, restartY * screenHalfH, "RESTART");
        DrawButton(btnX, settingsY * screenHalfH, "SETTINGS");
        DrawButton(btnX, menuY * screenHalfH, "MAIN MENU");    

        // ========================================================================
        // EXIT CONFIRMATION POPUP OVERLAY
        // ========================================================================
        if (inConfirmExit)
        {
            // Dim the pause menu
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(0.85f);
            AEGfxSetColorToMultiply(0.02f, 0.01f, 0.04f, 1.0f);
            Mesh_Renderer(bgMesh, 0.0f, 0.0f, winW, winH, 0.0f);

            // Popup Box - Gold Border
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
            Mesh_Renderer(bgMesh, 0.0f, 0.0f, 610.0f, 310.0f, 0.0f);

            // Popup Box - Dark Fill
            AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f);
            Mesh_Renderer(bgMesh, 0.0f, 0.0f, 600.0f, 300.0f, 0.0f);

            // Draw YES and NO Buttons
            float confBtnW = 160.0f;
            float confBtnH = 65.0f;
            float yesX = -130.0f;
            float noX = 130.0f;
            float confBtnY = -60.0f;

            auto DrawConfButton = [&](float cx, float cy) {
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                Mesh_Renderer(bgMesh, cx, cy, confBtnW + borderSize * 2, confBtnH + borderSize * 2, 0.0f);
                AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f);
                Mesh_Renderer(bgMesh, cx, cy, confBtnW, confBtnH, 0.0f);
                };
            DrawConfButton(yesX, confBtnY);
            DrawConfButton(noX, confBtnY);

            // Draw Text
            const char* promptText = isConfirmingRestart ? "RESTART GAME?" : "EXIT TO MAIN MENU?";
            const char* warningText = "YOU WILL LOSE PROGRESS";
            const char* yesText = "YES";
            const char* noText = "NO";

            float pw{}, ph{}, ww{}, wh{}, yw{}, yh{}, nw{}, nh{};
            AEGfxGetPrintSize(Graphics::font_id, promptText, 0.8f, &pw, &ph);
            if (gGameStateCurr != GS_TUTORIAL)
            AEGfxGetPrintSize(Graphics::font_id, warningText, 0.5f, &ww, &wh);
            AEGfxGetPrintSize(Graphics::font_id, yesText, 0.65f, &yw, &yh);
            AEGfxGetPrintSize(Graphics::font_id, noText, 0.65f, &nw, &nh);

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Gold prompt
            AEGfxPrint(Graphics::font_id, promptText, -(pw * 0.5f), (80.0f / screenHalfH) - (ph * 0.5f), 0.8f, 1.0f, 0.82f, 0.1f, 1.0f);
            // Red warning
            if(gGameStateCurr != GS_TUTORIAL)
            AEGfxPrint(Graphics::font_id, warningText, -(ww * 0.5f), (30.0f / screenHalfH) - (wh * 0.5f), 0.5f, 1.0f, 0.4f, 0.4f, 1.0f);
            // Warm gold YES/NO
            AEGfxPrint(Graphics::font_id, yesText, (yesX / screenHalfW) - (yw * 0.5f), (confBtnY / screenHalfH) - (yh * 0.5f), 0.65f, 1.0f, 0.92f, 0.6f, 1.0f);
            AEGfxPrint(Graphics::font_id, noText, (noX / screenHalfW) - (nw * 0.5f), (confBtnY / screenHalfH) - (nh * 0.5f), 0.65f, 1.0f, 0.92f, 0.6f, 1.0f);

            // Input Handling for Popup
            if (ignoreNextClick) {
                ignoreNextClick = false;
            }
            else if (AEInputCheckTriggered(AEVK_LBUTTON)) {
                s32 mx, my;
                AEInputGetCursorPosition(&mx, &my);
                float wx = (float)mx - screenHalfW;
                float wy = screenHalfH - (float)my;

                float halfConfW = confBtnW * 0.5f;
                float halfConfH = confBtnH * 0.5f;

                if (wx >= (yesX - halfConfW) && wx <= (yesX + halfConfW) &&
                    wy >= (confBtnY - halfConfH) && wy <= (confBtnY + halfConfH)) {
                    SoundManager::PlayClickBtnSfx();
                    isPaused = false;
                    // Route the game state depending on which popup we opened
                    if (isConfirmingRestart)
                    {
                        // Safely restart the current level (re-loads current state)
                        gGameStateNext = GS_RESTART;
                    }
                    else
                    {
                        // Reset player name and go to Main Menu
                        std::string temp_name = "";
                        ResetPlayerName(temp_name);
                        gGameStateNext = GS_MAIN_MENU;
                    }

                    inConfirmExit = false;
                }
                else if (wx >= (noX - halfConfW) && wx <= (noX + halfConfW) &&
                    wy >= (confBtnY - halfConfH) && wy <= (confBtnY + halfConfH)) {
                    SoundManager::PlayClickBtnSfx();
                    inConfirmExit = false; // Close popup
                    ignoreNextClick = true; // Prevent click-through back to the main pause menu
                }
            }
            return; // Stops normal pause menu from drawing and checking inputs!
        }

        // ========================================================================
        // Input Handling (Normal Pause Menu)
        // ========================================================================
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

            float halfW = btnW * 0.5f;
            float halfH = btnH * 0.5f;

            // Check RESUME
            if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                wy >= ((resumeY * screenHalfH) - halfH) && wy <= ((resumeY * screenHalfH) + halfH))
            {
                SoundManager::PlayClickBtnSfx();
                isPaused = false; // Unpauses the game normally
            }

            // --- Check RESTART ---
            if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                wy >= ((restartY * screenHalfH) - halfH) && wy <= ((restartY * screenHalfH) + halfH))
            {
                SoundManager::PlayClickBtnSfx();
                inConfirmExit = true;
                isConfirmingRestart = true; // Tell the popup to display "RESTART" text
                ignoreNextClick = true;
            }

            // Check SETTINGS
            if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                wy >= ((settingsY * screenHalfH) - halfH) && wy <= ((settingsY * screenHalfH) + halfH))
            {
                SoundManager::PlayClickBtnSfx();
                inSettings = true; // Opens the sound slider UI
            }

            // Check MAIN MENU 
            if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                wy >= ((menuY * screenHalfH) - halfH) && wy <= ((menuY * screenHalfH) + halfH))
            {
                SoundManager::PlayClickBtnSfx();
                inConfirmExit = true;    // Opens the confirmation popup
                isConfirmingRestart = false;
                ignoreNextClick = true;  // Prevents instantly clicking "YES" or "NO"
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
                    ignoreNextClick = true; // Prevents click-through back to the pause menu
                }
                else {
                    return; // Stop drawing the normal pause menu
                }
            }

           
            Ui_DrawPauseMenu(isPaused, bgMesh);
        }
    }
}