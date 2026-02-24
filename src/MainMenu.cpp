#include "AEEngine.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Leaderboard.h"
#include "Typing.h"
#include <iostream>

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

    const float EXIT_WIDTH = 240.0f;
    const float EXIT_HEIGHT = 80.0f;

    const float PLAY_HITBOX_WIDTH = 200.0f;
    const float PLAY_HITBOX_HEIGHT = 95.0f;

    const float SHOP_HITBOX_WIDTH = 200.0f;
    const float SHOP_HITBOX_HEIGHT = 95.0f;

    const float LOGO_WIDTH = 400.0f;
    const float LOGO_HEIGHT = 150.0f;

    const float EXITNAME_WIDTH = 240.0f;
    const float EXITNAME_HEIGHT = 80.0f;

    const float ENTERGAME_WIDTH = 240.0f;
    const float ENTERGAME_HEIGHT = 80.0f;


    // ---------------------------------------------------------------------------
    // POSITIONS
    // ---------------------------------------------------------------------------
  
    float LOGO_Y;
    float PLAY_BTN_Y;
    float SHOP_BTN_Y;
    float EXIT_BTN_Y;
    float EXITNAME_BTN_Y;
    float ENTERGAME_BTN_Y;
    float EXITNAME_BTN_X;
    float ENTERGAME_BTN_X;

    bool nameEntered = false;
    bool isEnteringName = false; //draws main menu if not in entering name screen
    std::string player_name = "";

    float cursorTimer = 0.0f;
    bool cursorVisible = true;

}

std::string Main_Menu_GetPlayerName() {
    return player_name;
}

void ResetPlayerName(std::string& reset_name) {
    player_name = reset_name;
}

void Main_Menu_EnterName() {

    s32 mouseX, mouseY;
    AEInputGetCursorPosition(&mouseX, &mouseY);

    float winW = (float)AEGfxGetWindowWidth();
    float winH = (float)AEGfxGetWindowHeight();

    float worldX = (float)mouseX - (winW * 0.5f);
    float worldY = (winH * 0.5f) - (float)mouseY;

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        // --- Check Enter Game Button ---
        if ((!player_name.empty()) &&
            worldX >= ENTERGAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= ENTERGAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
            worldY >= ENTERGAME_BTN_Y - (ENTERGAME_HEIGHT * 0.5f) && worldY <= ENTERGAME_BTN_Y + (ENTERGAME_HEIGHT * 0.5f))
        {
            isEnteringName = false;
            gGameStateNext = GS_GAME;
        }
        // --- Check Exit Name Button ---
        else if (worldX >= EXITNAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= EXITNAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
            worldY >= EXITNAME_BTN_Y - (EXITNAME_HEIGHT * 0.5f) && worldY <= EXITNAME_BTN_Y + (EXITNAME_HEIGHT * 0.5f))
        {
            isEnteringName = false;
            player_name = "";
        }
    }
    
    if (AEInputCheckTriggered(AEVK_ESCAPE)) { isEnteringName = false; player_name = ""; }

    if (AEInputCheckTriggered(AEVK_BACK) && !player_name.empty()) { //allows backspace only if there is at least one char
        player_name.pop_back(); // Removes the last character
    }

    
    if (player_name.length() < 12) { //max length of name allowed
        for (int k = AEVK_A; k <= AEVK_Z; ++k) {
            // Check if this specific key was triggered this frame
            if (AEInputCheckTriggered(static_cast<u8>(k))) {
                char c = (char)('A' + (k - AEVK_A)); // Convert keycode to char
                player_name += c;                   // Append to string
                std::cout << player_name << "\n";
            }
        }
    }
    cursorTimer += (float)AEFrameRateControllerGetFrameTime();
    if (cursorTimer >= 0.5f) { // Blinks every half-second
        cursorVisible = !cursorVisible;
        cursorTimer = 0.0f;
    }

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);//exit button
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); 
    // Reusing your Exit Button mesh temporarily just to get a box on screen
    Mesh_Renderer(pMeshExitBtn, EXITNAME_BTN_X, EXITNAME_BTN_Y, 1.0f, 1.0f, 0.0f);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);//enter game button
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    // Reusing your Exit Button mesh temporarily just to get a box on screen
    Mesh_Renderer(pMeshExitBtn, ENTERGAME_BTN_X, ENTERGAME_BTN_Y, 1.0f, 1.0f, 0.0f);

    if (Graphics::font_id >= 0) {//display name being typed
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        const char* promptText = "Enter Name:";
        float promptWidth = 0.0f, promptHeight = 0.0f;
        float promptScale = 1.0f; 

        // Measure and center the prompt
        AEGfxGetPrintSize(Graphics::font_id, promptText, promptScale, &promptWidth, &promptHeight);
        float promptStartX = 0.0f - (promptWidth * 0.5f);

        // Place it higher on the screen (0.4f is near the top-middle)
        float promptY = 0.4f;

        // Draw the prompt
        AEGfxPrint(Graphics::font_id, promptText, promptStartX, promptY, promptScale, 1.0f, 1.0f, 1.0f, 1.0f);

        float textWidth = 0.0f, textHeight = 0.0f;
        float textScale = 1.0f;
        float startX = 0.0f;
        float textY = 0.2f;

        if (!player_name.empty()) {

            AEGfxGetPrintSize(Graphics::font_id, player_name.c_str(), textScale, &textWidth, &textHeight);
            startX = 0.0f - (textWidth * 0.5f);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, player_name.c_str(), startX, textY, textScale, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        if (cursorVisible) {
            // Place the cursor exactly at the end of the typed word.
            // (If the string is empty, textWidth is 0.0f, so it draws dead center!)
            float cursorX = startX + textWidth;

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, "_", cursorX, textY, textScale, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

}

void Main_Menu_Init() {
    float winW = (float)AEGfxGetWindowWidth();
    float winH = (float)AEGfxGetWindowHeight();

    LOGO_Y = winH * 0.35f;              
    PLAY_BTN_Y = winH * 0.05f;         
    SHOP_BTN_Y = winH * -0.15f;        
    EXIT_BTN_Y = winH * -0.35f;      

    EXITNAME_BTN_Y = winH * -0.35f;
    ENTERGAME_BTN_Y = winH * -0.35f;

    EXITNAME_BTN_X = winW * -0.35f;     
    ENTERGAME_BTN_X = winW * 0.35f;     

    if (Graphics::font_id < 0) {
        Graphics::font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 48);
    }
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
    pMeshExitBtn = Rect_Mesh(EXIT_WIDTH, EXIT_HEIGHT, 0xFFFFFFFF);
}

void Main_Menu_Update() {

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    if (!isEnteringName) {

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

        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        // Convert screen coordinates (Top-Left 0,0) to World Coordinates (Center 0,0)
        float worldX = (float)mouseX - (winW * 0.5f);
        float worldY = (winH * 0.5f) - (float)mouseY;

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            // --- Check Play Button (Using the smaller Hitbox dimensions) ---
            if (worldX >= 0.0f - (PLAY_HITBOX_WIDTH * 0.5f) && worldX <= 0.0f + (PLAY_HITBOX_WIDTH * 0.5f) &&
                worldY >= PLAY_BTN_Y - (PLAY_HITBOX_HEIGHT * 0.5f) && worldY <= PLAY_BTN_Y + (PLAY_HITBOX_HEIGHT * 0.5f))
            {
                isEnteringName = true;
            }
            // --- Check Shop Button (Using the smaller Hitbox dimensions) ---
            else if (worldX >= 0.0f - (SHOP_HITBOX_WIDTH * 0.5f) && worldX <= 0.0f + (SHOP_HITBOX_WIDTH * 0.5f) &&
                worldY >= SHOP_BTN_Y - (SHOP_HITBOX_HEIGHT * 0.5f) && worldY <= SHOP_BTN_Y + (SHOP_HITBOX_HEIGHT * 0.5f))
            {
                gGameStateNext = GS_SHOP;
            }
            // --- Check Exit Button (Using EXIT_WIDTH/HEIGHT since it's a solid Rect_Mesh) ---
            else if (worldX >= 0.0f - (EXIT_WIDTH * 0.5f) && worldX <= 0.0f + (EXIT_WIDTH * 0.5f) &&
                worldY >= EXIT_BTN_Y - (EXIT_HEIGHT * 0.5f) && worldY <= EXIT_BTN_Y + (EXIT_HEIGHT * 0.5f))
            {
                gGameStateNext = GS_QUIT;
            }
        }
   }
    else {
        Main_Menu_EnterName();
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
