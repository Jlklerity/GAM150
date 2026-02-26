#include "AEEngine.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include "MainMenu.h"
#include "GameStateManager.h"
#include "Leaderboard.h"
#include "Typing.h"
#include "Settings.h"
#include <iostream>

namespace
{
    AEGfxVertexList* pMeshLogo = nullptr;
    AEGfxVertexList* pMeshBtn = nullptr;     // Standard mesh for Play & Shop buttons
    AEGfxVertexList* pMeshExitBtn = nullptr; // Mesh for Exit button
    AEGfxVertexList* pMeshSettingsBtn = nullptr;
    AEGfxVertexList* pMeshLeftArrow = nullptr;
    AEGfxVertexList* pMeshRightArrow = nullptr;


    AEGfxTexture* pTexPlay = nullptr;        // Play Button Texture
    AEGfxTexture* pTexShop = nullptr;        // Shop Button Texture
    AEGfxTexture* pTexArrows = nullptr;  // Arrow Button Texture

    float winW;
    float winH;
    float worldX;
    float worldY;

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

    const float ZENMODE_WIDTH = 240.0f;
    const float ZENMODE_HEIGHT = 80.0f;

    const float TIMEMODE_WIDTH = 240.0f;
    const float TIMEMODE_HEIGHT = 80.0f;

    const float SETTINGS_WIDTH = 240.0f;
    const float SETTINGS_HEIGHT = 80.0f;

    const float LEFTARROW_WIDTH = 80.0f;
    const float LEFTARROW_HEIGHT = 80.0f;

    const float RIGHTARROW_WIDTH = 80.0f;
    const float RIGHTARROW_HEIGHT = 80.0f;


    // ---------------------------------------------------------------------------
    // POSITIONS
    // ---------------------------------------------------------------------------
  
    float LOGO_Y;
    float PLAY_BTN_Y;
    float SHOP_BTN_Y;
    float EXIT_BTN_Y;
    float EXITNAME_BTN_Y;
    float EXITNAME_BTN_X;
    float ENTERGAME_BTN_Y;
    float ENTERGAME_BTN_X;
    float ZENMODE_BTN_X;
    float ZENMODE_BTN_Y;
    float TIMEMODE_BTN_X;
    float TIMEMODE_BTN_Y;
    float SETTINGS_BTN_X;
    float SETTINGS_BTN_Y;
    float LEFTARROW_BTN_X;
    float LEFTARROW_BTN_Y;
    float RIGHTARROW_BTN_X;
    float RIGHTARROW_BTN_Y;
    

    bool nameEntered = false;
    bool isEnteringName = false; //draws main menu if not in entering name screen
    bool inSettings = false;
    std::string player_name = "";
    std::string selected_mode = "";

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

    winW = (float)AEGfxGetWindowWidth();
    winH = (float)AEGfxGetWindowHeight();

    worldX = (float)mouseX - (winW * 0.5f);
    worldY = (winH * 0.5f) - (float)mouseY;

    if (AEInputCheckTriggered(AEVK_LBUTTON))
    {
        // --- Check Enter Game Button ---
        if ((!player_name.empty()) && (selected_mode != "") &&
            (worldX >= ENTERGAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= ENTERGAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
            worldY >= ENTERGAME_BTN_Y - (ENTERGAME_HEIGHT * 0.5f) && worldY <= ENTERGAME_BTN_Y + (ENTERGAME_HEIGHT * 0.5f)))
        {
            isEnteringName = false;
            if (selected_mode == "ZEN") gGameStateNext = GS_GAME;
            else if (selected_mode == "TIME") gGameStateNext = GS_GAME_TIMEMODE;
        }
        // --- Check Exit Name Button ---
        else if (worldX >= EXITNAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= EXITNAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
            worldY >= EXITNAME_BTN_Y - (EXITNAME_HEIGHT * 0.5f) && worldY <= EXITNAME_BTN_Y + (EXITNAME_HEIGHT * 0.5f))
        {
            isEnteringName = false;
            player_name = "";
            selected_mode = "";
        }
        //// --- Check Zen Mode Button ---
        //else if (worldX >= ZENMODE_BTN_X - (ZENMODE_WIDTH * 0.5f) && worldX <= ZENMODE_BTN_X + (ZENMODE_WIDTH * 0.5f) &&
        //    worldY >= ZENMODE_BTN_Y - (ZENMODE_HEIGHT * 0.5f) && worldY <= ZENMODE_BTN_Y + (ZENMODE_HEIGHT * 0.5f))
        //{
        //    selected_mode = "ZEN";
        //}
        //// --- Check Time Mode Button ---
        //else if (worldX >= TIMEMODE_BTN_X - (TIMEMODE_WIDTH * 0.5f) && worldX <= TIMEMODE_BTN_X + (TIMEMODE_WIDTH * 0.5f) &&
        //    worldY >= TIMEMODE_BTN_Y - (TIMEMODE_HEIGHT * 0.5f) && worldY <= TIMEMODE_BTN_Y + (TIMEMODE_HEIGHT * 0.5f))
        //{
        //    selected_mode = "TIME";
        //}
    }
    
    if (AEInputCheckTriggered(AEVK_ESCAPE)) { isEnteringName = false; player_name = ""; selected_mode = "ZEN"; }

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

    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);//zen mode button
    //AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    //AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 1.0f);
    //
    //Mesh_Renderer(pMeshZenModeBtn, ZENMODE_BTN_X, ZENMODE_BTN_Y, 1.0f, 1.0f, 0.0f);

    //AEGfxSetRenderMode(AE_GFX_RM_COLOR);//time mode button
    //AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    //AEGfxSetColorToMultiply(0.5f, 0.5f, 0.5f, 1.0f);

    //Mesh_Renderer(pMeshTimeModeBtn, TIMEMODE_BTN_X, TIMEMODE_BTN_Y, 1.0f, 1.0f, 0.0f);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);//exit button
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f); 
     
    Mesh_Renderer(pMeshExitBtn, EXITNAME_BTN_X, EXITNAME_BTN_Y, 1.0f, 1.0f, 0.0f);

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);//enter game button
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(0.0f, 1.0f, 0.0f, 1.0f);
    
    Mesh_Renderer(pMeshExitBtn, ENTERGAME_BTN_X, ENTERGAME_BTN_Y, 1.0f, 1.0f, 0.0f);

    if (Graphics::font_id >= 0) {//display name being typed
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        const char* promptText = "Enter Name:";
        float promptWidth = 0.0f, promptHeight = 0.0f;
        float promptScale = 1.0f; 

        //// Measure and center the prompt /////
        AEGfxGetPrintSize(Graphics::font_id, promptText, promptScale, &promptWidth, &promptHeight);
        float promptStartX = 0.0f - (promptWidth * 0.5f);

        // Place it higher on the screen (0.4f is near the top-middle)
        float promptY = 0.4f;

        // Draw the prompt
        AEGfxPrint(Graphics::font_id, promptText, promptStartX, promptY, promptScale, 1.0f, 1.0f, 1.0f, 1.0f);


        std::string modeDisplay = "Mode: " + selected_mode;

        float modeWidth = 0.0f, modeHeight = 0.0f;
        float modeScale = 1.0f;

        ///// Measure and center the combined string ("Mode: ZEN" or "Mode: TIME")
        AEGfxGetPrintSize(Graphics::font_id, modeDisplay.c_str(), modeScale, &modeWidth, &modeHeight);
        float modeStartX = 0.0f - (modeWidth * 0.5f);

        float modeY = -0.35f;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::font_id, modeDisplay.c_str(), modeStartX, modeY, modeScale, 1.0f, 1.0f, 1.0f, 1.0f);

        ///// 5. Setup variables for the player name typing logic (Keep these!)
        float textWidth = 0.0f, textHeight = 0.0f;
        float textScale = 1.0f;
        float startX = 0.0f;
        float textY = 0.2f;


        if (selected_mode == "ZEN") {
            // Measure the combined string ("Mode: ZEN" or "Mode: TIME")
            AEGfxGetPrintSize(Graphics::font_id, modeDisplay.c_str(), modeScale, &modeWidth, &modeHeight);

            float modeStartX = 0.0f - (modeWidth * 0.5f);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, modeDisplay.c_str(), modeStartX, modeY, modeScale, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        if (selected_mode == "TIME") {
            AEGfxGetPrintSize(Graphics::font_id, modeDisplay.c_str(), modeScale, &modeWidth, &modeHeight);

            float modeStartX = 0.0f - (modeWidth * 0.5f);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, modeDisplay.c_str(), modeStartX, modeY, modeScale, 1.0f, 1.0f, 1.0f, 1.0f);
        }

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

        // ==========================================================
        // --- Draw "ZEN" inside the left button ---
        // ==========================================================
        //float zenTextW = 0.0f, zenTextH = 0.0f;
        //AEGfxGetPrintSize(Graphics::font_id, "ZEN", 1.0f, &zenTextW, &zenTextH);

        //// Convert button World Coordinates to Text NDC (-1.0 to 1.0)
        //float zenNdcX = ZENMODE_BTN_X / (winW * 0.5f);
        //float zenNdcY = ZENMODE_BTN_Y / (winH * 0.5f);

        //// Center X on the button, and nudge Y down a tiny bit to visually center it in the box
        //float zenStartX = zenNdcX - (zenTextW * 0.5f);
        //float zenStartY = zenNdcY - 0.035f;

        //AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        //// Drawing in White (1, 1, 1, 1) so it pops on the red box
        //AEGfxPrint(Graphics::font_id, "ZEN", zenStartX, zenStartY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);


        //// ==========================================================
        //// --- Draw "TIME" inside the right button ---
        //// ==========================================================
        //float timeTextW = 0.0f, timeTextH = 0.0f;
        //AEGfxGetPrintSize(Graphics::font_id, "TIME", 1.0f, &timeTextW, &timeTextH);

        //// Convert button World Coordinates to Text NDC (-1.0 to 1.0)
        //float timeNdcX = TIMEMODE_BTN_X / (winW * 0.5f);
        //float timeNdcY = TIMEMODE_BTN_Y / (winH * 0.5f);

        //// Center X on the button, and nudge Y down a tiny bit
        //float timeStartX = timeNdcX - (timeTextW * 0.5f);
        //float timeStartY = timeNdcY - 0.035f;

        //AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        //AEGfxPrint(Graphics::font_id, "TIME", timeStartX, timeStartY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

}

void Main_Menu_Init() {
    winW = (float)AEGfxGetWindowWidth();
    winH = (float)AEGfxGetWindowHeight();

    LOGO_Y = winH * 0.35f;              
    PLAY_BTN_Y = winH * 0.05f;         
    SHOP_BTN_Y = winH * -0.15f;        
    EXIT_BTN_Y = winH * -0.35f;      

    EXITNAME_BTN_Y = winH * -0.35f;
    EXITNAME_BTN_X = winW * -0.35f;     

    ENTERGAME_BTN_X = winW * 0.35f;     
    ENTERGAME_BTN_Y = winH * -0.35f;

    ZENMODE_BTN_X = winW * -0.12f;
    ZENMODE_BTN_Y = winH * 0.0f;

    TIMEMODE_BTN_X = winW * 0.12f;
    TIMEMODE_BTN_Y = winH * 0.0f;

    SETTINGS_BTN_X = winW * -0.35f;
    SETTINGS_BTN_Y = winH * -0.4f;

    LEFTARROW_BTN_X = winH * -0.12f;
    LEFTARROW_BTN_Y = PLAY_BTN_Y - 85.0f;

    RIGHTARROW_BTN_X = winH * 0.12f;
    RIGHTARROW_BTN_Y = PLAY_BTN_Y - 90.0f;


    selected_mode = "ZEN";
    inSettings = false;

    if (Graphics::font_id < 0) {
        Graphics::font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 48);
    }
    // 1. Load the Textures
    pTexPlay = AEGfxTextureLoad("Assets/play.png");
    AE_ASSERT_MESG(pTexPlay, "Failed to load play.png");

    pTexShop = AEGfxTextureLoad("Assets/shop.png");
    AE_ASSERT_MESG(pTexShop, "Failed to load shop.png");

    pTexArrows = AEGfxTextureLoad("Assets/LeftRightButtons.png");
    AE_ASSERT_MESG(pTexArrows, "Failed to load LeftRightButtons.png");

    // 2. Create Meshes
    // Logo (Colored Rect)
    pMeshLogo = Rect_Mesh(LOGO_WIDTH, LOGO_HEIGHT, 0xFF0000FF);

    // Play & Shop Buttons (Sprite Mesh)
    pMeshBtn = Sprite_Mesh(1.0f, 1.0f);

    // Exit Button (Red Rect)
    pMeshExitBtn = Rect_Mesh(EXIT_WIDTH, EXIT_HEIGHT, 0xFFFFFFFF);

    //Settings button (white rect bottom left)
    pMeshSettingsBtn = Rect_Mesh(SETTINGS_WIDTH, SETTINGS_HEIGHT, 0xFFFFFFFF);

    pMeshLeftArrow = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);

    pMeshRightArrow = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 1.0f);
}

void Main_Menu_Update() {


    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    if (isEnteringName) {
        Main_Menu_EnterName();
    }
    else {

        // --- Draw Logo ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        Mesh_Renderer(pMeshLogo, 0.0f, LOGO_Y, 1.0f, 1.0f, 0.0f);

        if (pTexArrows) {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Tell the engine to use the Arrows texture
            AEGfxTextureSet(pTexArrows, 0.0f, 0.0f);

            // Draw the Left Button Mesh (Will only show the left arrow!)
            Mesh_Renderer(pMeshLeftArrow, LEFTARROW_BTN_X, LEFTARROW_BTN_Y, LEFTARROW_WIDTH, LEFTARROW_HEIGHT, 0.0f);

            // Draw the Right Button Mesh (Will only show the right arrow!)
            Mesh_Renderer(pMeshRightArrow, RIGHTARROW_BTN_X, RIGHTARROW_BTN_Y, RIGHTARROW_WIDTH, RIGHTARROW_HEIGHT, 0.0f);
        }

        // ==========================================================
        // --- Draw the Mode Text ("ZEN" / "TIME") Between Arrows ---
        // ==========================================================
        if (Graphics::font_id >= 0) {
            float textW = 0.0f, textH = 0.0f;
            AEGfxGetPrintSize(Graphics::font_id, selected_mode.c_str(), 1.0f, &textW, &textH);

            // Center horizontally
            float textNdcX = 0.0f - (textW * 0.5f);
            // Convert the Arrow's World Y to Text NDC (-1.0 to 1.0), and nudge down to center visually
            float textNdcY = RIGHTARROW_BTN_Y / (winH * 0.5f) - 0.025f;

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, selected_mode.c_str(), textNdcX, textNdcY, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

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

        // --- Draw Exit Button (Colored) ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        Mesh_Renderer(pMeshSettingsBtn, SETTINGS_BTN_X, SETTINGS_BTN_Y, 1.0f, 1.0f, 0.0f);

        if (inSettings) {
            // If DisplaySettings returns false (e.g., player hit ESC), close settings
            if (SysSettings::DisplaySettings() == false) {
                inSettings = false;
            }
        }
        else {
            // --- Handle Input ---
            s32 mouseX, mouseY;
            AEInputGetCursorPosition(&mouseX, &mouseY);

            winW = (float)AEGfxGetWindowWidth();
            winH = (float)AEGfxGetWindowHeight();
            // Convert screen coordinates (Top-Left 0,0) to World Coordinates (Center 0,0)
            worldX = (float)mouseX - (winW * 0.5f);
            worldY = (winH * 0.5f) - (float)mouseY;

            if (selected_mode == "") selected_mode = "ZEN";

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
                // --- Check Settings Button ---
                else if (worldX >= SETTINGS_BTN_X - (SETTINGS_WIDTH * 0.5f) && worldX <= SETTINGS_BTN_X + (SETTINGS_WIDTH * 0.5f) &&
                    worldY >= SETTINGS_BTN_Y - (SETTINGS_HEIGHT * 0.5f) && worldY <= SETTINGS_BTN_Y + (SETTINGS_HEIGHT * 0.5f))
                {
                    inSettings = true;
                }
                // --- Check Left Arrow Button ---
                else if (worldX >= LEFTARROW_BTN_X - (LEFTARROW_WIDTH * 0.5f) && worldX <= LEFTARROW_BTN_X + (LEFTARROW_WIDTH * 0.5f) &&
                    worldY >= LEFTARROW_BTN_Y - (LEFTARROW_HEIGHT * 0.5f) && worldY <= LEFTARROW_BTN_Y + (LEFTARROW_HEIGHT * 0.5f))
                {
                    selected_mode = (selected_mode == "ZEN") ? "TIME" : "ZEN";
                }
                // --- Check Right Arrow Button ---
                else if (worldX >= RIGHTARROW_BTN_X - (RIGHTARROW_WIDTH * 0.5f) && worldX <= RIGHTARROW_BTN_X + (RIGHTARROW_WIDTH * 0.5f) &&
                    worldY >= RIGHTARROW_BTN_Y - (RIGHTARROW_HEIGHT * 0.5f) && worldY <= RIGHTARROW_BTN_Y + (RIGHTARROW_HEIGHT * 0.5f))
                {       
                    selected_mode = (selected_mode == "ZEN") ? "TIME" : "ZEN";
                }
            }
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
