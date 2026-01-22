#include "AEEngine.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include "MainMenu.h"
#include "GameStateManager.h"

namespace
{
    AEGfxVertexList* pMeshLogo = nullptr;
    AEGfxVertexList* pMeshStartBtn = nullptr;
    AEGfxVertexList* pMeshQuitBtn = nullptr;

    const float BTN_WIDTH = 200.0f;
    const float BTN_HEIGHT = 80.0f;
    const float LOGO_WIDTH = 400.0f;
    const float LOGO_HEIGHT = 150.0f;

    const float LOGO_Y = 200.0f;
    const float START_BTN_Y = 0.0f;
    const float QUIT_BTN_Y = -150.0f;
}

void Main_Menu_Init() {
    pMeshLogo = Rect_Mesh(LOGO_WIDTH, LOGO_HEIGHT, 0xFF0000FF);
    pMeshStartBtn = Rect_Mesh(BTN_WIDTH, BTN_HEIGHT, 0xFF00FF00);
    pMeshQuitBtn = Rect_Mesh(BTN_WIDTH, BTN_HEIGHT, 0xFFFF0000);
}

void Main_Menu_Update() {

    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    Mesh_Renderer(pMeshLogo, 0.0f, LOGO_Y, 1.0f, 1.0f, 0.0f);
    Mesh_Renderer(pMeshStartBtn, 0.0f, START_BTN_Y, 1.0f, 1.0f, 0.0f);
    Mesh_Renderer(pMeshQuitBtn, 0.0f, QUIT_BTN_Y, 1.0f, 1.0f, 0.0f);

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float worldX = (float)mouseX - 1600.0f / 2.0f;
    float worldY = -((float)mouseY - 900.0f / 2.0f); // Y is flipped

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        // check start button collision
        if (worldX >= -BTN_WIDTH / 2.0f && worldX <= BTN_WIDTH / 2.0f &&
            worldY >= START_BTN_Y - BTN_HEIGHT / 2.0f && worldY <= START_BTN_Y + BTN_HEIGHT / 2.0f)
        {
            gGameStateNext = GS_GAME; // switch to game
        }

        // check quitbButton collision
        if (worldX >= -BTN_WIDTH / 2.0f && worldX <= BTN_WIDTH / 2.0f &&
            worldY >= QUIT_BTN_Y - BTN_HEIGHT / 2.0f && worldY <= QUIT_BTN_Y + BTN_HEIGHT / 2.0f)
        {
            gGameStateNext = GS_QUIT; // quit application
        }
    }
}
void Main_Menu_Exit() {
    if (pMeshLogo) {
        AEGfxMeshFree(pMeshLogo);
        pMeshLogo = nullptr;
    }
    if (pMeshStartBtn) {
        AEGfxMeshFree(pMeshStartBtn);
        pMeshStartBtn = nullptr;
    }
    if (pMeshQuitBtn) {
        AEGfxMeshFree(pMeshQuitBtn);
        pMeshQuitBtn = nullptr;
    }
}