#include "AEEngine.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include "MainMenu.h"
#include "GameStateManager.h"

namespace
{
    AEGfxVertexList* pMeshLogo = nullptr;
    AEGfxVertexList* pMeshBtn = nullptr;     // Standard mesh for Play & Shop buttons
    AEGfxVertexList* pMeshExitBtn = nullptr; // Mesh for Exit button

    AEGfxTexture* pTexPlay = nullptr;        // Play Button Texture
    AEGfxTexture* pTexShop = nullptr;        // Shop Button Texture

    const float PLAY_WIDTH = 240.0f;
    const float PLAY_HEIGHT = 240.0f;

    const float SHOP_WIDTH = 240.0f;
    const float SHOP_HEIGHT = 240.0f;

    const float EXIT_WIDTH = 200.0f;
    const float EXIT_HEIGHT = 80.0f;

    const float LOGO_WIDTH = 400.0f;
    const float LOGO_HEIGHT = 150.0f;

    // ---------------------------------------------------------------------------
    // POSITIONS
    // ---------------------------------------------------------------------------
    const float LOGO_Y = 250.0f;
    const float PLAY_BTN_Y = 50.0f;
    const float SHOP_BTN_Y = -100.0f;
    const float EXIT_BTN_Y = -230.0f;
}

void Main_Menu_Init() {
    // 1. Load the Textures
    pTexPlay = AEGfxTextureLoad("Assets/play.png");
    AE_ASSERT_MESG(pTexPlay, "Failed to load play.png");

    pTexShop = AEGfxTextureLoad("Assets/shop.png");
    AE_ASSERT_MESG(pTexShop, "Failed to load shop.png");

    // 2. Create Meshes
    // Logo (Colored Rect)
    pMeshLogo = Rect_Mesh(LOGO_WIDTH, LOGO_HEIGHT, 0xFF0000FF);

    // Play & Shop Buttons (Sprite Mesh)
    pMeshBtn = Sprite_Mesh(1.0f, 1.0f);

    // Exit Button (Red Rect)
    pMeshExitBtn = Rect_Mesh(EXIT_WIDTH, EXIT_HEIGHT, 0xFFFF0000);
}

void Main_Menu_Update() {

    // --- 1. Reset Camera (Crucial for correct mouse alignment) ---
    AEGfxSetCamPosition(0.0f, 0.0f);

    // --- Render Setup ---
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // --- Draw Logo ---
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    Mesh_Renderer(pMeshLogo, 0.0f, LOGO_Y, 1.0f, 1.0f, 0.0f);

    // --- Draw Play Button ---
    if (pTexPlay)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        AEGfxTextureSet(pTexPlay, 0.0f, 0.0f);
        Mesh_Renderer(pMeshBtn, 0.0f, PLAY_BTN_Y, PLAY_WIDTH, PLAY_HEIGHT, 0.0f);
    }

    // --- Draw Shop Button ---
    if (pTexShop)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(pTexShop, 0.0f, 0.0f);
        Mesh_Renderer(pMeshBtn, 0.0f, SHOP_BTN_Y, SHOP_WIDTH, SHOP_HEIGHT, 0.0f);
    }

    // --- Draw Exit Button (Colored) ---
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    Mesh_Renderer(pMeshExitBtn, 0.0f, EXIT_BTN_Y, 1.0f, 1.0f, 0.0f);


    // --- Handle Input ---
    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    // FIX: Use actual window dimensions instead of hardcoded 1600x900
    float winW = (float)AEGfxGetWindowWidth();
    float winH = (float)AEGfxGetWindowHeight();

    // Convert screen coordinates (Top-Left 0,0) to World Coordinates (Center 0,0)
    float worldX = (float)mouseX - (winW * 0.5f);
    float worldY = (winH * 0.5f) - (float)mouseY;

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        // Helper to check collision with a button at a specific Y
        auto CheckClick = [&](float btnY, float btnW, float btnH) -> bool {
            // AABB Collision: Mouse vs Rectangle
            // Since buttons are drawn at X=0 (Center), the X range is [-W/2, W/2]
            return (worldX >= -btnW * 0.5f && worldX <= btnW * 0.5f &&
                worldY >= btnY - btnH * 0.5f && worldY <= btnY + btnH * 0.5f);
            };

        // Check Play
        if (CheckClick(PLAY_BTN_Y, PLAY_WIDTH, PLAY_HEIGHT))
        {
            gGameStateNext = GS_GAME;
        }
        // Check Shop
        else if (CheckClick(SHOP_BTN_Y, SHOP_WIDTH, SHOP_HEIGHT))
        {
            gGameStateNext = GS_SHOP;
        }
        // Check Exit
        else if (CheckClick(EXIT_BTN_Y, EXIT_WIDTH, EXIT_HEIGHT))
        {
            gGameStateNext = GS_QUIT;
        }
    }
}

void Main_Menu_Exit() {
    // Free Meshes
    if (pMeshLogo) {
        AEGfxMeshFree(pMeshLogo);
        pMeshLogo = nullptr;
    }
    if (pMeshBtn) {
        AEGfxMeshFree(pMeshBtn);
        pMeshBtn = nullptr;
    }
    if (pMeshExitBtn) {
        AEGfxMeshFree(pMeshExitBtn);
        pMeshExitBtn = nullptr;
    }

    // Free Textures
    if (pTexPlay) {
        AEGfxTextureUnload(pTexPlay);
        pTexPlay = nullptr;
    }
    if (pTexShop) {
        AEGfxTextureUnload(pTexShop);
        pTexShop = nullptr;
    }
}
