#include "GameOver.h"
#include "GameStateManager.h"
#include "AEEngine.h"
#include "Enemy.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include "Leaderboard.h"
#include "MainMenu.h"

namespace
{
    float s_fadeInTimer = 0.0f;
    const float FADE_IN_DURATION = 0.5f;
    bool  s_initialized = false;

    AEGfxVertexList* s_mesh = nullptr;

    void EnsureMesh()
    {
        if (!s_mesh)
            s_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    }

    bool DrawButton(const char* label, float cx, float cy, float w, float h,
        float r, float g, float b)
    {
        EnsureMesh();

        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.15f, 0.15f, 0.15f, 1.0f);
        Mesh_Renderer(s_mesh, cx, cy, w, h, 0.0f);

        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        Mesh_Renderer(s_mesh, cx, cy + h * 0.5f - 3.0f, w, 4.0f, 0.0f);

        if (Graphics::font_id >= 0)
        {
            float lw, lh;
            AEGfxGetPrintSize(Graphics::font_id, label, 0.6f, &lw, &lh);
            float ndcX = -lw * 0.5f;
            float ndcY = (cy / (winH * 0.5f)) - lh * 0.5f;
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxPrint(Graphics::font_id, label, ndcX, ndcY, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - winW * 0.5f;
            float wy = winH * 0.5f - (float)my;
            if (wx >= cx - w * 0.5f && wx <= cx + w * 0.5f &&
                wy >= cy - h * 0.5f && wy <= cy + h * 0.5f)
                return true;
        }
        return false;
    }
}

namespace UiGameOver
{
    void Exit()
    {
        if (s_mesh)
        {
            AEGfxMeshFree(s_mesh);
            s_mesh = nullptr;
        }
        s_fadeInTimer = 0.0f;
        s_initialized = false;
    }

    void Check_IfDead(bool& isDead, bool& isGameOver, bool& openLeaderboard,
        AEGfxVertexList* bgMesh, std::string temp_name, const Data::Score& score)
    {
        (void)bgMesh;
        if (!isDead) return;

        if (!isGameOver)
        {
            temp_name = Main_Menu_GetPlayerName();
            Leaderboard_System::Write_ZenLeaderboard(temp_name, score.score_value);
            isGameOver = true;
            s_fadeInTimer = 0.0f;
        }

        if (!openLeaderboard)
            Ui_GameOverMenu(isGameOver, score, openLeaderboard, bgMesh, isDead);
        else
            Leaderboard_System::Display_ZenLeaderboard(openLeaderboard);
    }

    void Ui_GameOverMenu(bool& isGameOver, const Data::Score& score,
        bool& openLeaderboard, AEGfxVertexList* bgMesh, bool& isDead)
    {
        (void)bgMesh;
        (void)isGameOver;
        EnsureMesh();

        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        float dt = (float)AEFrameRateControllerGetFrameTime();

        s_fadeInTimer += dt;
        float fadeAlpha = s_fadeInTimer / FADE_IN_DURATION;
        if (fadeAlpha > 1.0f) fadeAlpha = 1.0f;

        AEGfxSetCamPosition(0.0f, 0.0f);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(fadeAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        Mesh_Renderer(s_mesh, 0.0f, 0.0f, winW, winH, 0.0f);

        if (Graphics::font_id < 0) return;

        const char* titleText = "GAME OVER";
        float tw, th;
        AEGfxGetPrintSize(Graphics::font_id, titleText, 1.4f, &tw, &th);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, fadeAlpha);
        AEGfxPrint(Graphics::font_id, titleText,
            -tw * 0.5f, 0.55f,
            1.4f, 1.0f, 0.15f, 0.15f, fadeAlpha);

        std::string scoreStr = "SCORE:  " + std::to_string(score.score_value);
        float sw, sh;
        AEGfxGetPrintSize(Graphics::font_id, scoreStr.c_str(), 0.8f, &sw, &sh);
        AEGfxPrint(Graphics::font_id, scoreStr.c_str(),
            -sw * 0.5f, 0.3f,
            0.8f, 1.0f, 1.0f, 1.0f, fadeAlpha);

        if (fadeAlpha < 0.3f) return;

        float btnW = 350.0f;
        float btnH = 65.0f;
        float btnX = 0.0f;
        float gap = 90.0f;

        float restartY = 50.0f;
        float leaderboardY = restartY - gap;
        float menuY = leaderboardY - gap;

        if (DrawButton("RESTART", btnX, restartY, btnW, btnH, 0.2f, 0.8f, 0.2f))
        {
            isDead = false;
            s_fadeInTimer = 0.0f;
            gGameStateNext = GS_RESTART;
        }

        if (DrawButton("LEADERBOARD", btnX, leaderboardY, btnW, btnH, 0.2f, 0.5f, 1.0f))
        {
            openLeaderboard = true;
        }

        if (DrawButton("MAIN MENU", btnX, menuY, btnW, btnH, 1.0f, 0.5f, 0.1f))
        {
            isDead = false;
            s_fadeInTimer = 0.0f;
            std::string blank = "";
            ResetPlayerName(blank);
            gGameStateNext = GS_MAIN_MENU;
        }
    }

    void DrawTimeoutScreen(const Data::Score& score, AEGfxVertexList* bgMesh, bool& openLeaderboard)
    {
        (void)bgMesh;
        EnsureMesh();

        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        float dt = (float)AEFrameRateControllerGetFrameTime();

        s_fadeInTimer += dt;
        float fadeAlpha = s_fadeInTimer / FADE_IN_DURATION;
        if (fadeAlpha > 1.0f) fadeAlpha = 1.0f;

        AEGfxSetCamPosition(0.0f, 0.0f);

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(fadeAlpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        Mesh_Renderer(s_mesh, 0.0f, 0.0f, winW, winH, 0.0f);

        if (Graphics::font_id < 0) return;

        const char* header = "TIME'S UP!";
        float hw, hh;
        AEGfxGetPrintSize(Graphics::font_id, header, 1.4f, &hw, &hh);
        AEGfxPrint(Graphics::font_id, header,
            -hw * 0.5f, 0.55f,
            1.4f, 1.0f, 0.85f, 0.1f, fadeAlpha);

        std::string scoreStr = "SCORE:  " + std::to_string(score.score_value);
        float sw, sh;
        AEGfxGetPrintSize(Graphics::font_id, scoreStr.c_str(), 0.8f, &sw, &sh);
        AEGfxPrint(Graphics::font_id, scoreStr.c_str(),
            -sw * 0.5f, 0.3f,
            0.8f, 1.0f, 1.0f, 1.0f, fadeAlpha);

        if (fadeAlpha < 0.3f) return;

        float btnW = 350.0f;
        float btnH = 65.0f;
        float btnX = 0.0f;
        float gap = 90.0f;
        float restartY = 50.0f;
        float leaderY = restartY - gap;
        float menuY = leaderY - gap;

        if (DrawButton("RESTART", btnX, restartY, btnW, btnH, 0.2f, 0.8f, 0.2f))
            gGameStateNext = GS_RESTART;

        if (DrawButton("LEADERBOARD", btnX, leaderY, btnW, btnH, 0.2f, 0.5f, 1.0f))
        {
            Leaderboard_System::Read_TimeLeaderboard();
            openLeaderboard = true;
        }

        if (DrawButton("MAIN MENU", btnX, menuY, btnW, btnH, 1.0f, 0.5f, 0.1f))
        {
            std::string blank = "";
            ResetPlayerName(blank);
            gGameStateNext = GS_MAIN_MENU;
        }
    }
}