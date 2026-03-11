#include "GameOver.h"
#include "GameStateManager.h"
#include "AEEngine.h"
#include "Enemy.h"
#include "MeshRenderer.h"
#include "Leaderboard.h"
#include "MainMenu.h"

namespace UiGameOver
{
    void Check_IfDead(bool& isDead, bool& isGameOver, bool& openLeaderboard, AEGfxVertexList* bgMesh, 
                      std::string temp_name, const Data::Score& score) {
        if (isDead) {

            //Save the score once
            if (!isGameOver) {
                temp_name = Main_Menu_GetPlayerName();
                Leaderboard_System::Write_ZenLeaderboard(temp_name, score.score_value);
                isGameOver = true;
            }

            if (!openLeaderboard) {
                UiGameOver::Ui_GameOverMenu(isGameOver, score, openLeaderboard, bgMesh, isDead);
            }
            //LEADERBOARD OVERLAY
            else
            {
                Leaderboard_System::Display_ZenLeaderboard(openLeaderboard);
            }

        }
    }

    void Ui_GameOverMenu(bool& isGameOver, const Data::Score& score, bool& openLeaderboard, AEGfxVertexList* bgMesh, bool& isDead) //for game over menu (jeremy)
    {
        // 1. Save the score once
        if (!isGameOver) {
            std::string temp_name = Main_Menu_GetPlayerName();
            Leaderboard_System::Write_ZenLeaderboard(temp_name, score.score_value);
            isGameOver = true;
        }

        // ==========================================
        // GAME OVER OVERLAY
        // ==========================================
        if (!openLeaderboard)
        {
            const char* gameoverText = "GAME OVER";
            float w, h;
            AEGfxGetPrintSize(Graphics::font_id, gameoverText, 1.0f, &w, &h);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, gameoverText, -w * 0.5f, 0.25f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

            float btnW = 300.0f;
            float btnH = 60.0f;
            float btnX = 0.0f; // Centered Horizontally

            // Y Positions
            float restartY = -50.0f;
            float leaderY = -150.0f;
            float menuY = -250.0f;

            // --- Draw "RESTART" Button ---
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
            Mesh_Renderer(bgMesh, btnX, restartY, btnW, btnH, 0.0f);

            const char* restartText = "RESTART";
            float rw, rh;
            AEGfxGetPrintSize(Graphics::font_id, restartText, 0.6f, &rw, &rh);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, restartText, -rw * 0.5f, -0.15f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

            // --- Draw "LEADERBOARD" Button ---
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
            Mesh_Renderer(bgMesh, btnX, leaderY, btnW, btnH, 0.0f);

            const char* leaderText = "LEADERBOARD";
            float lw, lh;
            AEGfxGetPrintSize(Graphics::font_id, leaderText, 0.6f, &lw, &lh);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, leaderText, -lw * 0.5f, -0.37f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

            // --- Draw "MAIN MENU" Button ---
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
            Mesh_Renderer(bgMesh, btnX, menuY, btnW, btnH, 0.0f);

            const char* menuText = "MAIN MENU";
            float mw, mh;
            AEGfxGetPrintSize(Graphics::font_id, menuText, 0.6f, &mw, &mh);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, menuText, -mw * 0.5f, -0.59f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                s32 mx, my;
                AEInputGetCursorPosition(&mx, &my);

                float wx = (float)mx - 800.0f;
                float wy = 450.0f - (float)my;
                float halfW = btnW * 0.5f;
                float halfH = btnH * 0.5f;

                // Check RESTART
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) && wy >= (restartY - halfH) && wy <= (restartY + halfH)) {
                    isDead = false;
                    gGameStateNext = GS_RESTART;
                }

                // Check MAIN MENU
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) && wy >= (menuY - halfH) && wy <= (menuY + halfH)) {
                    isDead = false;
                    std::string temp_name = "";
                    ResetPlayerName(temp_name);
                    gGameStateNext = GS_MAIN_MENU;
                }

                // Check LEADERBOARD
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) && wy >= (leaderY - halfH) && wy <= (leaderY + halfH)) {
                    openLeaderboard = true;
                }
            }
        }
    }

    void DrawTimeoutScreen(const Data::Score& score, AEGfxVertexList* bgMesh, bool& openLeaderboard)
    {
        if (Graphics::font_id < 0) return;

        //  "TIME'S UP!" header 
        const char* header = "TIME'S UP!";
        float hw, hh;
        AEGfxGetPrintSize(Graphics::font_id, header, 1.0f, &hw, &hh);
        AEGfxPrint(Graphics::font_id, header, -hw * 0.5f, 0.55f, 1.0f, 1.0f, 0.85f, 0.2f, 1.0f); // centered, gold color

        //  final score  
        std::string scoreStr = "Score: " + std::to_string(score.score_value);
        float sw, sh;
        AEGfxGetPrintSize(Graphics::font_id, scoreStr.c_str(), 0.7f, &sw, &sh);
        AEGfxPrint(Graphics::font_id, scoreStr.c_str(), -sw * 0.5f, 0.25f, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f); // centered white

        //  Button layout 
        float btnW = 300.0f;
        float btnH = 60.0f;
        float btnX = 0.0f;
        float restartY = -50.0f;
        float leaderY = -150.0f;
        float menuY = -250.0f;

        // helper lambda: draw a grey button background then its label centered on top
        auto DrawBtn = [&](float cy, const char* label, float scale, float textNDC_Y)
            {
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);              // dark grey button bg
                Mesh_Renderer(bgMesh, btnX, cy, btnW, btnH, 0.0f);
                float lw, lh;
                AEGfxGetPrintSize(Graphics::font_id, label, scale, &lw, &lh);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                AEGfxPrint(Graphics::font_id, label, -lw * 0.5f, textNDC_Y, scale, 1.0f, 1.0f, 1.0f, 1.0f); // centered label
            };

        DrawBtn(restartY, "RESTART", 0.6f, -0.15f);                    // NDC Y values copied from GameOver.cpp
        DrawBtn(leaderY, "LEADERBOARD", 0.6f, -0.37f);
        DrawBtn(menuY, "MAIN MENU", 0.6f, -0.59f);
        //  click handling 
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;                                 // convert cursor to world x
            float wy = 450.0f - (float)my;                                 // convert cursor to world y
            float hW = btnW * 0.5f;
            float hH = btnH * 0.5f;

            if (wx >= btnX - hW && wx <= btnX + hW && wy >= restartY - hH && wy <= restartY + hH)
                gGameStateNext = GS_RESTART;                               // restart timed mode

            if (wx >= btnX - hW && wx <= btnX + hW && wy >= leaderY - hH && wy <= leaderY + hH)
            {
                Leaderboard_System::Read_TimeLeaderboard();                                             // refresh board data before showing
                openLeaderboard = true;
            }

            if (wx >= btnX - hW && wx <= btnX + hW && wy >= menuY - hH && wy <= menuY + hH)
            {
                std::string blank = "";
                ResetPlayerName(blank);                                     // clear stored name before returning to menu
                gGameStateNext = GS_MAIN_MENU;
            }
        }
    }
}