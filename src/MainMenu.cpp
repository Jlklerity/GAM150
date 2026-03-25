/*!
    @file           MainMenu.cpp
    @author         Jeremy Lim
    @course         GAM150
    @brief          Handles the main menu state, UI rendering, sub-state transitions, and name entry.

    This file contains the logic for navigating the main menu, selecting game modes, handling
    the screen dropping transition effect, and capturing keyboard input for the player's name.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

#include "pch.h"
#include "MainMenu.h"
#include "ButtonTween.h"


/*
@namespace      Anonymous (Main Menu State)
@brief          Local variables for UI elements, positions, and menu states.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace
{
    // ---------------------------------------------------------------------------
    // MESHES & TEXTURES
    // ---------------------------------------------------------------------------
    AEGfxVertexList* pMeshLogo = nullptr;                                                                                   // mesh for the game logo
    AEGfxVertexList* pMeshBtn = nullptr;                                                                                    // standard mesh for play & shop buttons
    AEGfxVertexList* pMeshExitBtn = nullptr;                                                                                // mesh for exit button
    AEGfxVertexList* pMeshSettingsBtn = nullptr;                                                                            // mesh for settings button
    AEGfxVertexList* pMeshLeftArrow = nullptr;                                                                              // mesh for left arrow
    AEGfxVertexList* pMeshRightArrow = nullptr;                                                                             // mesh for right arrow
    AEGfxVertexList* pMeshLeaderboardBtn = nullptr;
    AEGfxVertexList* pMeshEnterName = nullptr;
    AEGfxVertexList* pMeshBackName = nullptr;
    AEGfxVertexList* pMeshTutorialBtn = nullptr;
    AEGfxVertexList* pMeshCreditsBtn = nullptr;

    AEGfxTexture* pTexPlay = nullptr;                                                                                       // play button texture
    AEGfxTexture* pTexShop = nullptr;                                                                                       // shop button texture
    AEGfxTexture* pTexArrows = nullptr;                                                                                     // arrow buttons texture
    AEGfxTexture* pTexLeaderboard = nullptr;
    AEGfxTexture* pTexSettings = nullptr;
    AEGfxTexture* pTexLogo = nullptr;
    AEGfxTexture* pTexEnterName = nullptr;
    AEGfxTexture* pTexBackName = nullptr;
    AEGfxTexture* pTexTutorial = nullptr;
    AEGfxTexture* pTexCredits = nullptr;
    // ---------------------------------------------------------------------------
    // WINDOW & CURSOR DATA
    // ---------------------------------------------------------------------------
    float winW;                                                                                                             // window width
    float winH;                                                                                                             // window height
    float worldX;                                                                                                           // mouse world X coordinate
    float worldY;                                                                                                           // mouse world Y coordinate

    // ---------------------------------------------------------------------------
    // UI DIMENSIONS (CONSTANTS)
    // ---------------------------------------------------------------------------
    const float PLAY_WIDTH = 240.0f;                                                                                        // width of play button
    const float PLAY_HEIGHT = 240.0f;                                                                                       // height of play button
    const float SHOP_WIDTH = 240.0f;                                                                                        // width of shop button
    const float SHOP_HEIGHT = 240.0f;                                                                                       // height of shop button
    const float EXIT_WIDTH = 240.0f;                                                                                        // width of exit button
    const float EXIT_HEIGHT = 80.0f;                                                                                        // height of exit button
    const float LEADERBOARD_WIDTH = 240.0f;
    const float LEADERBOARD_HEIGHT = 80.0f;

    const float PLAY_HITBOX_WIDTH = 200.0f;                                                                                 // interactable width for play
    const float PLAY_HITBOX_HEIGHT = 95.0f;                                                                                 // interactable height for play
    const float SHOP_HITBOX_WIDTH = 200.0f;                                                                                 // interactable width for shop
    const float SHOP_HITBOX_HEIGHT = 95.0f;                                                                                 // interactable height for shop

    const float LOGO_WIDTH = 1000.0f;                                                                                        // width of title logo
    const float LOGO_HEIGHT = 600.0f;                                                                                       // height of title logo

    const float EXITNAME_WIDTH = 200.0f;                                                                                    // width of exit name button
    const float EXITNAME_HEIGHT = 180.0f;                                                                                    // height of exit name button
    const float ENTERGAME_WIDTH = 200.0f;                                                                                   // width of enter game button
    const float ENTERGAME_HEIGHT = 180.0f;                                                                                   // height of enter game button

    const float ZENMODE_WIDTH = 240.0f;                                                                                     // width of zen mode button
    const float ZENMODE_HEIGHT = 80.0f;                                                                                     // height of zen mode button
    const float TIMEMODE_WIDTH = 240.0f;                                                                                    // width of time mode button
    const float TIMEMODE_HEIGHT = 80.0f;                                                                                    // height of time mode button

    const float SETTINGS_WIDTH = 96.0f;                                                                                    // width of settings button
    const float SETTINGS_HEIGHT = 96.0f;                                                                                    // height of settings button

    const float TUTORIAL_WIDTH = 240.0f;
    const float TUTORIAL_HEIGHT = 240.0f;
    const float TUTORIAL_HITBOX_WIDTH = 200.0f;
    const float TUTORIAL_HITBOX_HEIGHT = 95.0f;

    const float LEFTARROW_WIDTH = 80.0f;                                                                                    // width of left arrow
    const float LEFTARROW_HEIGHT = 80.0f;                                                                                   // height of left arrow
    const float RIGHTARROW_WIDTH = 80.0f;                                                                                   // width of right arrow
    const float RIGHTARROW_HEIGHT = 80.0f;                                                                                  // height of right arrow

    const float CREDITS_WIDTH = 240.0f;
    const float CREDITS_HEIGHT = 240.0f;

    // ---------------------------------------------------------------------------
    // UI POSITIONS (DYNAMIC)
    // ---------------------------------------------------------------------------
    float LOGO_Y;                                                                                                           // Y pos for logo
    float PLAY_BTN_Y;                                                                                                       // Y pos for play button
    float SHOP_BTN_Y;                                                                                                       // Y pos for shop button
    float EXIT_BTN_Y;                                                                                                       // Y pos for exit button
    float EXITNAME_BTN_Y;                                                                                                   // Y pos for exit name button
    float EXITNAME_BTN_X;                                                                                                   // X pos for exit name button
    float ENTERGAME_BTN_Y;                                                                                                  // Y pos for enter game button
    float ENTERGAME_BTN_X;                                                                                                  // X pos for enter game button
    float ZENMODE_BTN_X;                                                                                                    // X pos for zen mode button
    float ZENMODE_BTN_Y;                                                                                                    // Y pos for zen mode button
    float TIMEMODE_BTN_X;                                                                                                   // X pos for time mode button
    float TIMEMODE_BTN_Y;                                                                                                   // Y pos for time mode button
    float SETTINGS_BTN_X;                                                                                                   // X pos for settings button
    float SETTINGS_BTN_Y;                                                                                                   // Y pos for settings button
    float LEFTARROW_BTN_X;                                                                                                  // X pos for left arrow
    float LEFTARROW_BTN_Y;                                                                                                  // Y pos for left arrow
    float RIGHTARROW_BTN_X;                                                                                                 // X pos for right arrow
    float RIGHTARROW_BTN_Y;                                                                                                 // Y pos for right arrow
    float LEADERBOARD_BTN_X;
    float LEADERBOARD_BTN_Y;
    float TUTORIAL_BTN_Y;
    float CREDITS_BTN_X;
    float CREDITS_BTN_Y;
    float EMPTY_NAME_WARNING_TIMER;  
    float EMPTY_NAME_WARNING_X;      
    float EMPTY_NAME_WARNING_Y;      

    // ---------------------------------------------------------------------------
    // STATE & LOGIC VARIABLES
    // ---------------------------------------------------------------------------
    bool isTransitioningToName = false;                                                                                     // flag for entering name screen
    bool isTransitioningBack = false;                                                                                       // flag for returning to main menu
    bool nameEntered = false;                                                                                               // flag indicating name submission
    bool isEnteringName = false;                                                                                            // flag for current sub-state
    bool inSettings = false;                                                                                                // flag for settings overlay
    bool inLeaderboard = false;
    bool cursorVisible = true;
    bool transitioningToShop = false;
    bool isReturningFromShop = false;

    std::string player_name = "";                                                                                           // holds typed player name
    std::string selected_mode = "";                                                                                         // holds currently selected game mode
    std::string leaderboard_mode = "ZEN";

    float uiOffsetX = 0.0f;
    float cursorTimer = 0.0f;                                                                                               // timer for text cursor blink


    bool isZoomingToGame = false;

    // ---------------------------------------------------------------------------
    // BUTTON TWEENS
    // ---------------------------------------------------------------------------
    ButtonTween tweenPlay;
    ButtonTween tweenShop;
    ButtonTween tweenTutorial;
    ButtonTween tweenExit;
    ButtonTween tweenSettings;
    ButtonTween tweenLeaderboard;
    ButtonTween tweenLeftArrow;
    ButtonTween tweenRightArrow;
    ButtonTween tweenCredits;
    ButtonTween tweenEnterGame;
    ButtonTween tweenBackName;

    // ---------------------------------------------------------------------------
    // DIGIPEN SPLASH OVERLAY
    // ---------------------------------------------------------------------------
    AEGfxTexture* pTexSplashLogo = nullptr;   // DigiPen logo texture
    AEGfxVertexList* pMeshSplashQuad = nullptr;   // full-screen black quad
    AEGfxVertexList* pMeshSplashLogo = nullptr;   // logo quad (centred)

    static constexpr float SPLASH_HOLD = 2.0f;   // seconds fully visible
    static constexpr float SPLASH_FADE = 1.0f;   // seconds to fade out
    float splashTimer = 0.0f;
    bool  splashActive = false;                   // only true on first launch
} // Anonymous namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@function       Main_Menu_GetPlayerName
@brief          Retrieves the currently entered player name.
@usage          Leaderboard, Game states
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
std::string Main_Menu_GetPlayerName()
{
    return player_name;                                                                                                     // return stored string
}

/*
@function       Draw_Main_Menu
@params         textNdcOffset (float)
@brief          Helper function to render the normal menu elements (logo, buttons, mode text).
@usage          Main_Menu_Update
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
void Draw_Main_Menu(float textNdcOffset)
{
    // ==========================================
    // SETUP TEXTURE SHADER (For Play, Shop, Arrows)
    // ==========================================
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);                                                                                  // switch to texture mode
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                        // reset texture shader tint
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);                                                                             // no additive color
    AEGfxSetTransparency(1.0f);                                                                                             // force textures fully opaque

    // Draw Logo
    if (pTexLogo) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(pTexLogo, 0.0f, 0.0f);
        Mesh_Renderer(pMeshLogo, 0.0f + uiOffsetX, LOGO_Y, LOGO_WIDTH, LOGO_HEIGHT, 0.0f);                                                               // render colored logo mesh
    }
    // Draw Arrows
    if (pTexArrows)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);                                                                              // ensure texture mode active
        AEGfxTextureSet(pTexArrows, 0.0f, 0.0f);                                                                            // bind arrow sprite sheet
        Mesh_Renderer(pMeshLeftArrow, LEFTARROW_BTN_X + uiOffsetX, LEFTARROW_BTN_Y, LEFTARROW_WIDTH * tweenLeftArrow.scale, LEFTARROW_HEIGHT * tweenLeftArrow.scale, 0.0f);           // render left arrow
        Mesh_Renderer(pMeshRightArrow, RIGHTARROW_BTN_X + uiOffsetX, RIGHTARROW_BTN_Y, RIGHTARROW_WIDTH * tweenRightArrow.scale, RIGHTARROW_HEIGHT * tweenRightArrow.scale, 0.0f);      // render right arrow
    }

    // Draw Mode Text
    if (Graphics::font_id >= 0)
    {
        float textW = 0.0f, textH = 0.0f;                                                                                   // measurement containers
        AEGfxGetPrintSize(Graphics::font_id, selected_mode.c_str(), 1.0f, &textW, &textH);                                  // measure text width/height
        float textNdcX = 0.0f - (textW * 0.5f) + uiOffsetX;                                                                             // center text horizontally
        float textNdcY = RIGHTARROW_BTN_Y / (winH * 0.5f) - 0.025f + textNdcOffset;                                         // calc Y and apply transition offset
        AEGfxPrint(Graphics::font_id, selected_mode.c_str(), textNdcX, textNdcY, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);             // render mode text
    }

    // Draw Play Button
    if (pTexPlay)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);                                                                              // ensure texture mode active
        AEGfxTextureSet(pTexPlay, 0.0f, 0.0f);                                                                              // bind play texture
        Mesh_Renderer(pMeshBtn, 0.0f + uiOffsetX, PLAY_BTN_Y, PLAY_WIDTH * tweenPlay.scale, PLAY_HEIGHT * tweenPlay.scale, 0.0f);                                           // render play button
    }

    // Draw Shop Button
    if (pTexShop)
    {
        AEGfxTextureSet(pTexShop, 0.0f, 0.0f);                                                                              // bind shop texture
        Mesh_Renderer(pMeshBtn, 0.0f + uiOffsetX, SHOP_BTN_Y, SHOP_WIDTH * tweenShop.scale, SHOP_HEIGHT * tweenShop.scale, 0.0f);                                           // render shop button
    }

    if (pTexTutorial)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(pTexTutorial, 0.0f, 0.0f);
        Mesh_Renderer(pMeshBtn, 0.0f + uiOffsetX, TUTORIAL_BTN_Y, TUTORIAL_WIDTH * tweenTutorial.scale, TUTORIAL_HEIGHT * tweenTutorial.scale, 0.0f);
    }

    // Draw Leaderboard Button
    if (pTexLeaderboard)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(pTexLeaderboard, 0.0f, 0.0f);
        Mesh_Renderer(pMeshLeaderboardBtn, LEADERBOARD_BTN_X + uiOffsetX, LEADERBOARD_BTN_Y, LEADERBOARD_WIDTH * tweenLeaderboard.scale, LEADERBOARD_HEIGHT * tweenLeaderboard.scale, 0.0f);
    }

    //Draw settings button
    if (pTexSettings)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(pTexSettings, 0.0f, 0.0f);
        Mesh_Renderer(pMeshSettingsBtn, SETTINGS_BTN_X + uiOffsetX, SETTINGS_BTN_Y, SETTINGS_WIDTH * tweenSettings.scale, SETTINGS_HEIGHT * tweenSettings.scale, 0.0f);
    }

    if (pTexCredits)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxTextureSet(pTexCredits, 0.0f, 0.0f);
        Mesh_Renderer(pMeshCreditsBtn, CREDITS_BTN_X + uiOffsetX, CREDITS_BTN_Y, CREDITS_WIDTH * tweenCredits.scale, CREDITS_HEIGHT * tweenCredits.scale, 0.0f);
    }

    // Draw Exit Buttons
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);                                                                                    // swap back to color mode
    Mesh_Renderer(pMeshExitBtn, 0.0f + uiOffsetX, EXIT_BTN_Y, 1.0f, 1.0f, 0.0f);                                           // render exit button


}

/*
@function       Main_Menu_Input
@brief          Helper function to handle mouse clicks for the primary menu buttons.
@usage          Main_Menu_Update
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
void Main_Menu_Input()
{
    s32 mouseX, mouseY;                                                                                                     // raw mouse inputs
    AEInputGetCursorPosition(&mouseX, &mouseY);                                                                             // grab cursor screen pos

    worldX = (float)mouseX - (winW * 0.5f);                                                                                 // convert screen X to world X
    worldY = (winH * 0.5f) - (float)mouseY;                                                                                 // convert screen Y to world Y

    if (selected_mode == "") selected_mode = "ZEN";                                                                         // default mode safety check
     
    // ---------------------------------------------------------------------------
    // BUTTON TWEEN UPDATES
    // ---------------------------------------------------------------------------
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();
        bool click = AEInputCheckTriggered(AEVK_LBUTTON);

        bool hPlay = (worldX >= -(PLAY_HITBOX_WIDTH * 0.5f) && worldX <= (PLAY_HITBOX_WIDTH * 0.5f) &&
            worldY >= PLAY_BTN_Y - (PLAY_HITBOX_HEIGHT * 0.5f) && worldY <= PLAY_BTN_Y + (PLAY_HITBOX_HEIGHT * 0.5f));
        bool hShop = (worldX >= -(SHOP_HITBOX_WIDTH * 0.5f) && worldX <= (SHOP_HITBOX_WIDTH * 0.5f) &&
            worldY >= SHOP_BTN_Y - (SHOP_HITBOX_HEIGHT * 0.5f) && worldY <= SHOP_BTN_Y + (SHOP_HITBOX_HEIGHT * 0.5f));
        bool hTut = (worldX >= -(TUTORIAL_HITBOX_WIDTH * 0.5f) && worldX <= (TUTORIAL_HITBOX_WIDTH * 0.5f) &&
            worldY >= TUTORIAL_BTN_Y - (TUTORIAL_HITBOX_HEIGHT * 0.5f) && worldY <= TUTORIAL_BTN_Y + (TUTORIAL_HITBOX_HEIGHT * 0.5f));
        bool hExit = (worldX >= -(EXIT_WIDTH * 0.5f) && worldX <= (EXIT_WIDTH * 0.5f) &&
            worldY >= EXIT_BTN_Y - (EXIT_HEIGHT * 0.5f) && worldY <= EXIT_BTN_Y + (EXIT_HEIGHT * 0.5f));
        bool hSet = (worldX >= SETTINGS_BTN_X - (SETTINGS_WIDTH * 0.5f) && worldX <= SETTINGS_BTN_X + (SETTINGS_WIDTH * 0.5f) &&
            worldY >= SETTINGS_BTN_Y - (SETTINGS_HEIGHT * 0.5f) && worldY <= SETTINGS_BTN_Y + (SETTINGS_HEIGHT * 0.5f));
        bool hLdr = (worldX >= LEADERBOARD_BTN_X - (LEADERBOARD_WIDTH * 0.5f) && worldX <= LEADERBOARD_BTN_X + (LEADERBOARD_WIDTH * 0.5f) &&
            worldY >= LEADERBOARD_BTN_Y - (LEADERBOARD_HEIGHT * 0.5f) && worldY <= LEADERBOARD_BTN_Y + (LEADERBOARD_HEIGHT * 0.5f));
        bool hLArr = (worldX >= LEFTARROW_BTN_X - (LEFTARROW_WIDTH * 0.5f) && worldX <= LEFTARROW_BTN_X + (LEFTARROW_WIDTH * 0.5f) &&
            worldY >= LEFTARROW_BTN_Y - (LEFTARROW_HEIGHT * 0.5f) && worldY <= LEFTARROW_BTN_Y + (LEFTARROW_HEIGHT * 0.5f));
        bool hRArr = (worldX >= RIGHTARROW_BTN_X - (RIGHTARROW_WIDTH * 0.5f) && worldX <= RIGHTARROW_BTN_X + (RIGHTARROW_WIDTH * 0.5f) &&
            worldY >= RIGHTARROW_BTN_Y - (RIGHTARROW_HEIGHT * 0.5f) && worldY <= RIGHTARROW_BTN_Y + (RIGHTARROW_HEIGHT * 0.5f));
        bool hCred = (worldX >= CREDITS_BTN_X - (CREDITS_WIDTH * 0.5f) && worldX <= CREDITS_BTN_X + (CREDITS_WIDTH * 0.5f) &&
            worldY >= CREDITS_BTN_Y - (CREDITS_HEIGHT * 0.5f) && worldY <= CREDITS_BTN_Y + (CREDITS_HEIGHT * 0.5f));


        if (!splashActive) {
            ButtonTween_Update(tweenPlay, hPlay, hPlay && click, dt);
            ButtonTween_Update(tweenShop, hShop, hShop && click, dt);
            ButtonTween_Update(tweenTutorial, hTut, hTut && click, dt);
            ButtonTween_Update(tweenExit, hExit, hExit && click, dt);
            ButtonTween_Update(tweenSettings, hSet, hSet && click, dt);
            ButtonTween_Update(tweenLeaderboard, hLdr, hLdr && click, dt);
            ButtonTween_Update(tweenLeftArrow, hLArr, hLArr && click, dt);
            ButtonTween_Update(tweenRightArrow, hRArr, hRArr && click, dt);
            ButtonTween_Update(tweenCredits, hCred, hCred && click, dt);
        }
    }
        if (UiPauseMenu::IsPopupOpen()) return;

    if (!splashActive && !isTransitioningToName && !transitioningToShop && !isReturningFromShop && AEInputCheckTriggered(AEVK_ESCAPE))
    {
        SoundManager::PlayClickBtnSfx();
        UiPauseMenu::OpenPopup(UiPauseMenu::POPUP_QUIT);
    }
    if (!splashActive && !isTransitioningToName && !transitioningToShop && !isReturningFromShop && AEInputCheckTriggered(AEVK_LBUTTON))                                                                                // check for left click
    {
        // --- Check Play Button ---
        if (worldX >= 0.0f - (PLAY_HITBOX_WIDTH * 0.5f) && worldX <= 0.0f + (PLAY_HITBOX_WIDTH * 0.5f) &&
            worldY >= PLAY_BTN_Y - (PLAY_HITBOX_HEIGHT * 0.5f) && worldY <= PLAY_BTN_Y + (PLAY_HITBOX_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            isTransitioningToName = true;                                                                                   // set forward transition flag
            ScreenTransitions::ScreenTransition_Init();                                                                     // start drop animation
        }
        // --- Check Shop Button ---
        else if (worldX >= 0.0f - (SHOP_HITBOX_WIDTH * 0.5f) && worldX <= 0.0f + (SHOP_HITBOX_WIDTH * 0.5f) &&
            worldY >= SHOP_BTN_Y - (SHOP_HITBOX_HEIGHT * 0.5f) && worldY <= SHOP_BTN_Y + (SHOP_HITBOX_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            transitioningToShop = true;
            ScreenTransitions::ScreenTransition_Init();                                                                     // Start the transition timer
        }
        // --- Check Exit Button ---
        else if (worldX >= 0.0f - (EXIT_WIDTH * 0.5f) && worldX <= 0.0f + (EXIT_WIDTH * 0.5f) &&
            worldY >= EXIT_BTN_Y - (EXIT_HEIGHT * 0.5f) && worldY <= EXIT_BTN_Y + (EXIT_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            UiPauseMenu::OpenPopup(UiPauseMenu::POPUP_QUIT);                                                                                      // queue application quit
        }
        // --- Check Settings Button ---
        else if (worldX >= SETTINGS_BTN_X - (SETTINGS_WIDTH * 0.5f) && worldX <= SETTINGS_BTN_X + (SETTINGS_WIDTH * 0.5f) &&
            worldY >= SETTINGS_BTN_Y - (SETTINGS_HEIGHT * 0.5f) && worldY <= SETTINGS_BTN_Y + (SETTINGS_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            inSettings = true;                                                                                              // toggle settings overlay
        }
        // --- Check Left Arrow Button ---
        else if (worldX >= LEFTARROW_BTN_X - (LEFTARROW_WIDTH * 0.5f) && worldX <= LEFTARROW_BTN_X + (LEFTARROW_WIDTH * 0.5f) &&
            worldY >= LEFTARROW_BTN_Y - (LEFTARROW_HEIGHT * 0.5f) && worldY <= LEFTARROW_BTN_Y + (LEFTARROW_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            selected_mode = (selected_mode == "ZEN") ? "TIME" : "ZEN";                                                      // toggle game mode
        }
        // --- Check Right Arrow Button ---
        else if (worldX >= RIGHTARROW_BTN_X - (RIGHTARROW_WIDTH * 0.5f) && worldX <= RIGHTARROW_BTN_X + (RIGHTARROW_WIDTH * 0.5f) &&
            worldY >= RIGHTARROW_BTN_Y - (RIGHTARROW_HEIGHT * 0.5f) && worldY <= RIGHTARROW_BTN_Y + (RIGHTARROW_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            selected_mode = (selected_mode == "ZEN") ? "TIME" : "ZEN";                                                      // toggle game mode
        }
        // --- Check Leaderboard Button ---
        else if (worldX >= LEADERBOARD_BTN_X - (LEADERBOARD_WIDTH * 0.5f) && worldX <= LEADERBOARD_BTN_X + (LEADERBOARD_WIDTH * 0.5f) &&
            worldY >= LEADERBOARD_BTN_Y - (LEADERBOARD_HEIGHT * 0.5f) && worldY <= LEADERBOARD_BTN_Y + (LEADERBOARD_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            leaderboard_mode = "ZEN";
            inLeaderboard = true; //                                                                                              
        }
        // --- Check Tutorial Button ---

        else if (worldX >= 0.0f - (TUTORIAL_HITBOX_WIDTH * 0.5f) && worldX <= 0.0f + (TUTORIAL_HITBOX_WIDTH * 0.5f) &&
            worldY >= TUTORIAL_BTN_Y - (TUTORIAL_HITBOX_HEIGHT * 0.5f) && worldY <= TUTORIAL_BTN_Y + (TUTORIAL_HITBOX_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            isZoomingToGame = true;
            ParallaxZoom::Start(GS_TUTORIAL);
        }
        // --- Check Credits Button ---
        else if (worldX >= CREDITS_BTN_X - (CREDITS_WIDTH * 0.5f) && worldX <= CREDITS_BTN_X + (CREDITS_WIDTH * 0.5f) &&
            worldY >= CREDITS_BTN_Y - (CREDITS_HEIGHT * 0.5f) && worldY <= CREDITS_BTN_Y + (CREDITS_HEIGHT * 0.5f))
        {
            SoundManager::PlayClickBtnSfx();
            gGameStateNext = GS_CREDITS;
        }
    }
}

/*
@function       ResetPlayerName
@params         reset_name (std::string&)
@brief          Overrides the current stored player name string.
@usage          State cleanup, Game Over
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
void ResetPlayerName(std::string& reset_name)
{
    player_name = reset_name;                                                                                               // apply new string to player name
}

/*
@function       Main_Menu_EnterName
@params         worldYOffset (float), ndcYOffset (float), allowInput (bool)
@brief          Handles rendering and input logic for the Name Entry sub-state.
@usage          Main_Menu_Update
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
void Main_Menu_EnterName(float worldYOffset, float ndcYOffset, bool allowInput)
{
    //AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);                                                                              // force clear background color
    s32 mouseX, mouseY;                                                                                                     // raw mouse inputs
    AEInputGetCursorPosition(&mouseX, &mouseY);                                                                             // grab cursor screen pos

    worldX = (float)mouseX - (winW * 0.5f);                                                                                 // convert screen X to world X
    worldY = (winH * 0.5f) - (float)mouseY;                                                                                 // convert screen Y to world Y

    if (allowInput)                                                                                                         // only process input if allowed
    {
        //Separate the input triggers so the Enter key doesn't care where the mouse is
        bool enterPressed = AEInputCheckTriggered(AEVK_RETURN);
        bool enterClicked = AEInputCheckTriggered(AEVK_LBUTTON) &&
            (worldX >= ENTERGAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= ENTERGAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
                worldY >= ENTERGAME_BTN_Y - (ENTERGAME_HEIGHT * 0.5f) && worldY <= ENTERGAME_BTN_Y + (ENTERGAME_HEIGHT * 0.5f));

        bool exitClicked = AEInputCheckTriggered(AEVK_LBUTTON) &&
            (worldX >= EXITNAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= EXITNAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
                worldY >= EXITNAME_BTN_Y - (EXITNAME_HEIGHT * 0.5f) && worldY <= EXITNAME_BTN_Y + (EXITNAME_HEIGHT * 0.5f));

        // Update name-entry button tweens
        float dt = (float)AEFrameRateControllerGetFrameTime();
        bool hEnter = (worldX >= ENTERGAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= ENTERGAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
            worldY >= ENTERGAME_BTN_Y - (ENTERGAME_HEIGHT * 0.5f) && worldY <= ENTERGAME_BTN_Y + (ENTERGAME_HEIGHT * 0.5f));
        bool hBack = (worldX >= EXITNAME_BTN_X - (EXITNAME_WIDTH * 0.5f) && worldX <= EXITNAME_BTN_X + (EXITNAME_WIDTH * 0.5f) &&
            worldY >= EXITNAME_BTN_Y - (EXITNAME_HEIGHT * 0.5f) && worldY <= EXITNAME_BTN_Y + (EXITNAME_HEIGHT * 0.5f));
        ButtonTween_Update(tweenEnterGame, hEnter, enterClicked, dt);
        ButtonTween_Update(tweenBackName, hBack, exitClicked, dt);

        // --- Check Enter Game (Triggered by Mouse Click OR Keyboard Enter) ---
        if ((enterPressed || enterClicked) && !player_name.empty() && selected_mode != "")
        {
            SoundManager::PlayClickBtnSfx();
            isEnteringName = false;
            isZoomingToGame = true;
            int zoomTarget = (selected_mode == "TIME") ? GS_GAME_TIMEMODE : GS_GAME;
            ParallaxZoom::Start(zoomTarget);
        }
        // --Draw no name pop up
        else if ((enterPressed || enterClicked) && player_name.empty() && selected_mode != "")
        {
            SoundManager::PlayClickBtnSfx();
            EMPTY_NAME_WARNING_TIMER = 1.5f;
            EMPTY_NAME_WARNING_X = 0.0f;
            EMPTY_NAME_WARNING_Y = 0.05f;  
        }
        else if (exitClicked)
        {
            SoundManager::PlayClickBtnSfx();
            isTransitioningBack = true;
            ScreenTransitions::ScreenTransition_Init();
            isEnteringName = false;
            selected_mode = "ZEN";
        }

        if (AEInputCheckTriggered(AEVK_ESCAPE))                                                                             // check escape key
        {
            isTransitioningBack = true;                                                                                     // trigger reverse transition
            ScreenTransitions::ScreenTransition_Init();                                                                     // start transition
            isEnteringName = false;                                                                                         // leave sub-state
            selected_mode = "ZEN";                                                                                          // default back to zen
        }

        if (AEInputCheckTriggered(AEVK_BACK) && !player_name.empty())                                                       // check backspace key
        {
            player_name.pop_back();                                                                                         // remove last char
        }

        if (player_name.length() < 12)                                                                                      // cap name length at 12
        {
            for (int k = AEVK_A; k <= AEVK_Z; ++k)                                                                          // loop A-Z keycodes
            {
                if (AEInputCheckTriggered(static_cast<u8>(k)))                                                              // if specific key hit
                {
                    char c = (char)('A' + (k - AEVK_A));                                                                    // convert code to char
                    player_name += c;                                                                                       // append to string
                    std::cout << player_name << "\n";                                                                       // debug print
                }
            }
        }

        cursorTimer += (float)AEFrameRateControllerGetFrameTime();                                                          // accumulate cursor time
        if (cursorTimer >= 0.5f)                                                                                            // toggle every 0.5 sec
        {
            cursorVisible = !cursorVisible;                                                                                 // flip visibility
            cursorTimer = 0.0f;                                                                                             // reset timer
        }
    }

   // AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);                                                                              // clear screen to black

    // Draw Exit Name Button
    if (pTexBackName) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);                                                                                    // set to texture mode
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                     // enable alpha blend
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(pTexBackName, 0.0f, 0.0f);
        Mesh_Renderer(pMeshBackName, EXITNAME_BTN_X, EXITNAME_BTN_Y + worldYOffset, EXITNAME_WIDTH * tweenBackName.scale, EXITNAME_HEIGHT * tweenBackName.scale, 0.0f);                           // render exit button
    }
    // Draw Enter Game Button
    if (pTexEnterName) {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);                                                                                    // set to texture mode
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                     // enable alpha blend
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);                                                                        // tint green
        AEGfxTextureSet(pTexEnterName, 0.0f, 0.0f);
        Mesh_Renderer(pMeshEnterName, ENTERGAME_BTN_X, ENTERGAME_BTN_Y + worldYOffset, ENTERGAME_WIDTH * tweenEnterGame.scale, ENTERGAME_HEIGHT * tweenEnterGame.scale, 0.0f);                         // render enter button
    }
    // Draw Text Elements
    if (Graphics::font_id >= 0)                                                                                             // check if font valid
    {
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // ensure blending active

        const char* promptText = "Enter Name:";                                                                             // prompt string
        float promptWidth = 0.0f, promptHeight = 0.0f;                                                                      // prompt dimensions
        float promptScale = 1.0f;                                                                                           // prompt scale
        float promptY = 0.4f + ndcYOffset;                                                                                  // calculate Y pos

        AEGfxGetPrintSize(Graphics::font_id, promptText, promptScale, &promptWidth, &promptHeight);                         // measure string
        float promptStartX = 0.0f - (promptWidth * 0.5f);                                                                   // center horizontally
        AEGfxPrint(Graphics::font_id, promptText, promptStartX, promptY, promptScale, 0.0f, 0.0f, 0.0f, 1.0f);              // draw prompt

        float modeY = -0.35f + ndcYOffset;                                                                                  // mode string Y pos
        float textY = 0.2f + ndcYOffset;                                                                                    // input string Y pos
        std::string modeDisplay = "Mode: " + selected_mode;                                                                 // concat mode text

        float modeWidth = 0.0f, modeHeight = 0.0f;                                                                          // mode dimensions
        float modeScale = 1.0f;                                                                                             // mode scale

        AEGfxGetPrintSize(Graphics::font_id, modeDisplay.c_str(), modeScale, &modeWidth, &modeHeight);                      // measure mode string
        float modeStartX = 0.0f - (modeWidth * 0.5f);                                                                       // center horizontally

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // re-apply blend mode
        AEGfxPrint(Graphics::font_id, modeDisplay.c_str(), modeStartX, modeY, modeScale, 0.0f, 0.0f, 0.0f, 1.0f);           // render current mode text

        float textWidth = 0.0f, textHeight = 0.0f;                                                                          // name string dimensions
        float textScale = 1.0f;                                                                                             // name string scale
        float startX = 0.0f;                                                                                                // name string start X
        textY = 0.2f;                                                                                                       // explicit overwrite (as in orig)


        if (selected_mode == "ZEN")                                                                                         // specific logic block for ZEN
        {
            AEGfxGetPrintSize(Graphics::font_id, modeDisplay.c_str(), modeScale, &modeWidth, &modeHeight);                  // re-measure mode text
            modeStartX = 0.0f - (modeWidth * 0.5f);                                                                         // center horizontally
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                             // re-apply blend
            AEGfxPrint(Graphics::font_id, modeDisplay.c_str(), modeStartX, modeY, modeScale, 0.0f, 0.0f, 0.0f, 1.0f);       // print zen mode again
        }

        if (selected_mode == "TIME")                                                                                        // specific logic block for TIME
        {
            AEGfxGetPrintSize(Graphics::font_id, modeDisplay.c_str(), modeScale, &modeWidth, &modeHeight);                  // re-measure mode text
            modeStartX = 0.0f - (modeWidth * 0.5f);                                                                         // center horizontally
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                             // re-apply blend
            AEGfxPrint(Graphics::font_id, modeDisplay.c_str(), modeStartX, modeY, modeScale, 0.0f, 0.0f, 0.0f, 1.0f);       // print time mode again
        }

        textY = 0.2f + ndcYOffset;                                                                                          // re-apply transition offset
        if (!player_name.empty())                                                                                           // check if name has chars
        {
            AEGfxGetPrintSize(Graphics::font_id, player_name.c_str(), textScale, &textWidth, &textHeight);                  // measure name string
            startX = 0.0f - (textWidth * 0.5f);                                                                             // center horizontally
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                             // re-apply blend
            AEGfxPrint(Graphics::font_id, player_name.c_str(), startX, textY, textScale, 0.0f, 0.0f, 0.0f, 1.0f);           // print typed name
        }
    // ========================================================================
    // DRAW EMPTY NAME WARNING POPUP
    // ========================================================================
        if (EMPTY_NAME_WARNING_TIMER > 0.0f)
        {
            // 1. Calculate fade-out alpha based on time left
            float alpha = EMPTY_NAME_WARNING_TIMER / 0.5f;
            if (alpha > 1.0f) alpha = 1.0f; // Stays fully solid for 1 second, fades out for 0.5s

            const char* warning_text = "PLEASE ENTER A NAME";
            float tw, th;
            float warningScale = 0.85f;
            AEGfxGetPrintSize(Graphics::font_id, warning_text, warningScale, &tw, &th);

            // 3. Draw the red warning text
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);

            float ndcX = 0.0f - (tw * 0.5f);
            float ndcY = EMPTY_NAME_WARNING_Y + ndcYOffset;

            // Print the text (R, G, B, Alpha)
            AEGfxPrint(Graphics::font_id, warning_text, ndcX, ndcY, warningScale, 1.0f, 0.2f, 0.2f, alpha);
        }

        if (cursorVisible)                                                                                                  // check cursor flag
        {
            float cursorX = startX + textWidth;                                                                             // append cursor to end of word
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                             // re-apply blend
            AEGfxPrint(Graphics::font_id, "_", cursorX, textY, textScale, 0.0f, 0.0f, 0.0f, 1.0f);                          // print underscore cursor
        }
    }
}

/*
@function       Main_Menu_Init
@brief          Initializes variables, fonts, textures, and meshes used by the Main Menu.
@usage          GameStateManager
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
void Main_Menu_Init()
{

    // Add at the top of Main_Menu_Init(), before any TextureLoad:
    if (pTexLogo) { AEGfxTextureUnload(pTexLogo);        pTexLogo = nullptr; }
    if (pTexLeaderboard) { AEGfxTextureUnload(pTexLeaderboard);  pTexLeaderboard = nullptr; }
    if (pTexPlay) { AEGfxTextureUnload(pTexPlay);         pTexPlay = nullptr; }
    if (pTexShop) { AEGfxTextureUnload(pTexShop);         pTexShop = nullptr; }
    if (pTexArrows) { AEGfxTextureUnload(pTexArrows);       pTexArrows = nullptr; }
    if (pTexEnterName) { AEGfxTextureUnload(pTexEnterName);    pTexEnterName = nullptr; }
    if (pTexBackName) { AEGfxTextureUnload(pTexBackName);     pTexBackName = nullptr; }
    if (pTexSettings) { AEGfxTextureUnload(pTexSettings);     pTexSettings = nullptr; }
    if (pTexTutorial) { AEGfxTextureUnload(pTexTutorial);     pTexTutorial = nullptr; }
    if (pTexCredits) { AEGfxTextureUnload(pTexCredits);      pTexCredits = nullptr; }

    if (pMeshLogo) { AEGfxMeshFree(pMeshLogo);         pMeshLogo = nullptr; }
    if (pMeshBtn) { AEGfxMeshFree(pMeshBtn);          pMeshBtn = nullptr; }
    if (pMeshExitBtn) { AEGfxMeshFree(pMeshExitBtn);      pMeshExitBtn = nullptr; }
    if (pMeshEnterName) { AEGfxMeshFree(pMeshEnterName);    pMeshEnterName = nullptr; }
    if (pMeshBackName) { AEGfxMeshFree(pMeshBackName);     pMeshBackName = nullptr; }
    if (pMeshLeaderboardBtn) { AEGfxMeshFree(pMeshLeaderboardBtn); pMeshLeaderboardBtn = nullptr; }
    if (pMeshCreditsBtn) { AEGfxMeshFree(pMeshCreditsBtn);   pMeshCreditsBtn = nullptr; }
    if (pMeshLeftArrow) { AEGfxMeshFree(pMeshLeftArrow);    pMeshLeftArrow = nullptr; }
    if (pMeshRightArrow) { AEGfxMeshFree(pMeshRightArrow);   pMeshRightArrow = nullptr; }
    if (pMeshSettingsBtn) { AEGfxMeshFree(pMeshSettingsBtn);  pMeshSettingsBtn = nullptr; }
    if (pMeshTutorialBtn) { AEGfxMeshFree(pMeshTutorialBtn);  pMeshTutorialBtn = nullptr; }
    uiOffsetX = 0.0f;
    transitioningToShop = false;
    inLeaderboard = false;
    UiPauseMenu::ClosePopup();
    Leaderboard_System::Init();
    Leaderboard_System::Read_ZenLeaderboard();
    Leaderboard_System::Read_TimeLeaderboard();
    isZoomingToGame = false;
    ParallaxZoom::Reset();

    EMPTY_NAME_WARNING_TIMER = 0.0f;
    EMPTY_NAME_WARNING_X = 0.0f;           
    EMPTY_NAME_WARNING_Y = 0.0f;

    SoundManager::Init();
    SysSettings::Init();
    BackgroundParallax::MenuParallax_Init();

    selected_mode = "ZEN";                                                                                                  // default selected mode
    inSettings = false;                                                                                                     // default settings closed
    isTransitioningToName = false;                                                                                          // clear transition flag

    if (Graphics::font_id < 0)                                                                                              // check if font uninitialized
    {
        Graphics::font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 48);                                                   // load UI font
    }

    pTexLogo = AEGfxTextureLoad("Assets/title.png");
    AE_ASSERT_MESG(pTexLogo, "Failed to load title.png");

    pTexLeaderboard = AEGfxTextureLoad("Assets/leaderboardt.png");
    AE_ASSERT_MESG(pTexLeaderboard, "Failed to load leaderboardt.png")                                                  // check texture success

        pTexPlay = AEGfxTextureLoad("Assets/play.png");                                                                         // load play texture
    AE_ASSERT_MESG(pTexPlay, "Failed to load play.png");                                                                    // check texture success

    pTexShop = AEGfxTextureLoad("Assets/shop.png");                                                                         // load shop texture
    AE_ASSERT_MESG(pTexShop, "Failed to load shop.png");                                                                    // check texture success

    pTexArrows = AEGfxTextureLoad("Assets/LeftRightButtons.png");                                                           // load arrow texture
    AE_ASSERT_MESG(pTexArrows, "Failed to load LeftRightButtons.png");                                                      // check texture success

    pTexEnterName = AEGfxTextureLoad("Assets/playbutton.png");
    AE_ASSERT_MESG(pTexEnterName, "Failed to load playbutton.png");

    pTexBackName = AEGfxTextureLoad("Assets/backbutton.png");
    AE_ASSERT_MESG(pTexBackName, "Failed to load backbutton.png");

    pTexSettings = AEGfxTextureLoad("Assets/settingshover.png");
    AE_ASSERT_MESG(pTexSettings, "Failed to load settingshover.png");

    pTexTutorial = AEGfxTextureLoad("Assets/tutorial.png");
    AE_ASSERT_MESG(pTexTutorial, "Failed to load tutorial.png");

    pTexCredits = AEGfxTextureLoad("Assets/credits.png");
    AE_ASSERT_MESG(pTexCredits, "Failed to load credits.png");

    pMeshLogo = Sprite_Mesh(1.0f, 1.0f);                                                            // build logo  
    pMeshBtn = Sprite_Mesh(1.0f, 1.0f);                                                                                     // build standard button mesh
    pMeshExitBtn = Rect_Mesh(EXIT_WIDTH, EXIT_HEIGHT, 0xFFFFFFFF);                                                          // build exit button mesh
    pMeshEnterName = Sprite_Mesh(1.0f, 1.0f);
    pMeshBackName = Sprite_Mesh(1.0f, 1.0f);
    pMeshLeaderboardBtn = Sprite_Mesh(1.0f, 1.0f);
    pMeshCreditsBtn = Sprite_Mesh(1.0f, 1.0f);

    pMeshLeftArrow = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);                                                  // slice left arrow mesh
    pMeshRightArrow = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.5f, 0.5f, 1.0f);                                                 // slice right arrow mesh
    pMeshSettingsBtn = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f);
    pMeshTutorialBtn = Sprite_Mesh(1.0f, 1.0f);

    // just came from the Shop, set up the arrival animation
    if (gGameStatePrev == GS_SHOP) {
        isReturningFromShop = true;
        uiOffsetX = -winW; // Start UI completely off-screen to the left
        ScreenTransitions::ScreenTransition_Init();
    }
    else {
        isReturningFromShop = false;
        uiOffsetX = 0.0f;
    }

    // Reset button tweens
    ButtonTween_Reset(tweenPlay);
    ButtonTween_Reset(tweenShop);
    ButtonTween_Reset(tweenTutorial);
    ButtonTween_Reset(tweenExit);
    ButtonTween_Reset(tweenSettings);
    ButtonTween_Reset(tweenLeaderboard);
    ButtonTween_Reset(tweenLeftArrow);
    ButtonTween_Reset(tweenRightArrow);
    ButtonTween_Reset(tweenCredits);
    ButtonTween_Reset(tweenEnterGame);
    ButtonTween_Reset(tweenBackName);

    // ---------------------------------------------------------------------------
    // SPLASH OVERLAY – only on very first launch (gGameStatePrev == GS_NONE)
    // ---------------------------------------------------------------------------
    splashTimer = 0.0f;
    splashActive = (gGameStatePrev == GS_NONE);

    if (splashActive)
    {
        pTexSplashLogo = AEGfxTextureLoad("Assets/DigiPen_BLACK.png");

        // Full-screen solid black quad (unit quad, scaled by world half-extents in Update)
        AEGfxMeshStart();
        AEGfxTriAdd(-1.f, -1.f, 0xFF000000, 0.f, 1.f,
            1.f, -1.f, 0xFF000000, 1.f, 1.f,
            -1.f, 1.f, 0xFF000000, 0.f, 0.f);
        AEGfxTriAdd(1.f, -1.f, 0xFF000000, 1.f, 1.f,
            1.f, 1.f, 0xFF000000, 1.f, 0.f,
            -1.f, 1.f, 0xFF000000, 0.f, 0.f);
        pMeshSplashQuad = AEGfxMeshEnd();

        // Logo quad
        AEGfxMeshStart();
        AEGfxTriAdd(-1.f, -1.f, 0xFFFFFFFF, 0.f, 1.f,
            1.f, -1.f, 0xFFFFFFFF, 1.f, 1.f,
            -1.f, 1.f, 0xFFFFFFFF, 0.f, 0.f);
        AEGfxTriAdd(1.f, -1.f, 0xFFFFFFFF, 1.f, 1.f,
            1.f, 1.f, 0xFFFFFFFF, 1.f, 0.f,
            -1.f, 1.f, 0xFFFFFFFF, 0.f, 0.f);
        pMeshSplashLogo = AEGfxMeshEnd();
    }
}

/*
@function       Main_Menu_Update
@brief          Calculates dynamic screen coordinates, updates transition math, and dictates current sub-state loop.
@usage          GameStateManager
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
void Main_Menu_Update()
{

    winW = (float)AEGfxGetWindowWidth();                                                                                    // store window width
    winH = (float)AEGfxGetWindowHeight();                                                                                   // store window height

    float dt = (float)AEFrameRateControllerGetFrameTime();
    if (dt > 0.1f) dt = 0.01667f;                                                                                           // dt clamp
    
    if (EMPTY_NAME_WARNING_TIMER > 0.0f)
    {
        EMPTY_NAME_WARNING_TIMER -= dt;
    }

    BackgroundParallax::MenuParallax_Update(dt);

    if (transitioningToShop) {

        ScreenTransitions::ScreenTransition_Update(dt);                                                                     //normalized bounce (0.0 to 1.0)
        float smoothPan = ScreenTransitions::EaseOutExpo(ScreenTransitions::result);
        uiOffsetX = -winW * smoothPan;

        BackgroundParallax::MenuParallax_SetPanOffset(-0.2f * smoothPan);

        // Makes sure the timer doesnt exceed the set amount of seconds
        if (ScreenTransitions::result >= 0.3f)
        {
            transitioningToShop = false; // Stop transition
            uiOffsetX = -winW;           // Lock UI offscreen
            BackgroundParallax::MenuParallax_SetPanOffset(-0.2f);
            gGameStateNext = GS_SHOP;    // Switch to Shop state!
        }
    }

    if (isReturningFromShop) {
        ScreenTransitions::ScreenTransition_Update(dt);
        float smoothPan = ScreenTransitions::EaseOutExpo(ScreenTransitions::result);

        // Slide the UI from the left (-winW) back to the center (0)
        uiOffsetX = -winW * (1.0f - smoothPan);

        // Animate the background offset from -0.2f back to 0.0f
        BackgroundParallax::MenuParallax_SetPanOffset(-0.2f + (0.2f * smoothPan));

        if (ScreenTransitions::result >= 0.3f)
        {
            isReturningFromShop = false;
            uiOffsetX = 0.0f;
            BackgroundParallax::MenuParallax_SetPanOffset(0.0f); // Lock securely to center
        }
    }

    LOGO_Y = winH * 0.35f;                                                                                                  // set logo Y pos
    PLAY_BTN_Y = winH * 0.15f;                                                                                              // set play button Y pos
    TUTORIAL_BTN_Y = winH * -0.09f;                                                                                         // set tutorial button Y pos
    SHOP_BTN_Y = winH * -0.25f;                                                                                             // set shop button Y pos
    EXIT_BTN_Y = winH * -0.43f;                                                                                             // set exit button Y pos

    EXITNAME_BTN_Y = winH * -0.35f;                                                                                         // set exit name Y pos
    EXITNAME_BTN_X = winW * -0.35f;                                                                                         // set exit name X pos

    ENTERGAME_BTN_X = winW * 0.35f;                                                                                         // set enter game X pos
    ENTERGAME_BTN_Y = winH * -0.35f;                                                                                        // set enter game Y pos

    ZENMODE_BTN_X = winW * -0.12f;                                                                                          // set zen mode X pos
    ZENMODE_BTN_Y = winH * 0.0f;                                                                                            // set zen mode Y pos

    TIMEMODE_BTN_X = winW * 0.12f;                                                                                          // set time mode X pos
    TIMEMODE_BTN_Y = winH * 0.0f;                                                                                           // set time mode Y pos

    SETTINGS_BTN_X = winW * -0.35f;                                                                                         // set settings X pos
    SETTINGS_BTN_Y = winH * -0.4f;                                                                                          // set settings Y pos

    LEADERBOARD_BTN_X = winW * -0.23f;
    LEADERBOARD_BTN_Y = winH * -0.4f;

    LEFTARROW_BTN_X = winH * -0.12f;                                                                                        // set left arrow X pos
    LEFTARROW_BTN_Y = PLAY_BTN_Y - 85.0f;                                                                                   // set left arrow Y pos

    RIGHTARROW_BTN_X = winH * 0.12f;                                                                                        // set right arrow X pos
    RIGHTARROW_BTN_Y = PLAY_BTN_Y - 90.0f;                                                                                  // set right arrow Y pos

    CREDITS_BTN_X = winW * 0.23f;
    CREDITS_BTN_Y = winH * -0.4f;



    AEGfxSetCamPosition(0.0f, 0.0f);                                                                                        // reset camera explicitly
    if (isZoomingToGame || ParallaxZoom::IsActive())
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);
    else
        AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    float camY = 0.0f;                                                                                                      // initialize camera Y variable
    float textNdcOffset = 0.0f;                                                                                             // initialize text offset variable

    // --- FORWARD TRANSITION ---
    if (isTransitioningToName)                                                                                              // check drop-down transition
    {
        ScreenTransitions::ScreenTransition_Update((float)AEFrameRateControllerGetFrameTime());                             // advance math timer

        float bounce = ScreenTransitions::EaseOutExpo(ScreenTransitions::result);                                           // fetch normalized bounce

        camY = winH * bounce;                                                                                               // apply bounce to world camera
        AEGfxSetCamPosition(0.0f, camY);                                                                                    // set camera


        textNdcOffset = -(2.0f * bounce);                                                                                   // manually apply bounce to NDC

        if (ScreenTransitions::result >= 0.6f)                                                     // check if math complete
        {
            isTransitioningToName = false;                                                                                  // stop transition
            isEnteringName = true;                                                                                          // finalize state swap
            camY = 0.0f;                                                                                                    // lock camera back
            textNdcOffset = 0.0f;                                                                                           // lock text back
        }
    }
    // --- REVERSE TRANSITION ---
    else if (isTransitioningBack)                                                                                           // check slide-up transition
    {
        ScreenTransitions::ScreenTransition_Update((float)AEFrameRateControllerGetFrameTime());                             // advance math timer
        float bounce = ScreenTransitions::EaseOutExpo(ScreenTransitions::result);                                           // fetch normalized bounce

        float reverseBounce = 1.0f - bounce;                                                                                // invert drop math

        camY = winH * reverseBounce;                                                                                        // apply inverted bounce to cam
        textNdcOffset = -(2.0f * reverseBounce);                                                                            // manually apply inverted bounce to NDC

        if (ScreenTransitions::result >= 0.6f)                                                     // check if math complete
        {
            isTransitioningBack = false;                                                                                    // stop transition
            isEnteringName = false;                                                                                         // return to main sub-state
            camY = 0.0f;                                                                                                    // lock camera back
            textNdcOffset = 0.0f;                                                                                           // lock text back

            player_name = "";                                                                                               // wipe typing buffer safely
            selected_mode = "ZEN";                                                                                          // reset mode selection safely
        }
    }

    AEGfxSetCamPosition(0.0f, camY);                                                                                        // lock camera to frame pos

    float vCamY = 0.0f;
    if (isTransitioningToName) {
        vCamY = winH * ScreenTransitions::EaseOutExpo(ScreenTransitions::result);
    }
    else if (isTransitioningBack) {
        vCamY = winH * (1.0f - ScreenTransitions::EaseOutExpo(ScreenTransitions::result));
    }
    else if (isEnteringName) {
        vCamY = winH; // Keep the background scrolled down while in the sub-menu!
    }

    // Zoom transition runs exclusively - skip all normal drawing while active
    if (isZoomingToGame)
    {
        ParallaxZoom::Update(dt);
        ParallaxZoom::Draw(0.0f);   // <-- always draw from neutral position
        if (!ParallaxZoom::IsActive()) isZoomingToGame = false;
        return;
    }

    BackgroundParallax::MenuParallax_Draw(vCamY);

    // --- RENDER AND INPUT LOGIC ---
    if (inLeaderboard)
    {
        Draw_Main_Menu(0.0f); // Keep the menu rendered in the background

        if (leaderboard_mode == "ZEN") {
            Leaderboard_System::Display_ZenLeaderboard(inLeaderboard);
        }
        else if (leaderboard_mode == "TIME") {
            Leaderboard_System::Display_TimeLeaderboard(inLeaderboard);
        }

        // ==========================================================
        // NEW: DRAW THE SWITCH MODE ARROW BUTTON (Top Right Corner)
        // ==========================================================
        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);
        winW = (float)AEGfxGetWindowWidth();
        winH = (float)AEGfxGetWindowHeight();

        // Calculate mouse in World Coordinates to match the sprite meshes
        float wx = (float)mouseX - (winW * 0.5f);
        float wy = (winH * 0.5f) - (float)mouseY;

        // Pin the arrow to the top right corner of the screen
        float arrowW = 80.0f;
        float arrowH = 80.0f;
        float arrowX = 0.0f;
        float arrowY = 0.0f;

        // Move the arrow to the Left or Right edge depending on the mode
        if (leaderboard_mode == "ZEN") {
            arrowX = (winW * 0.5f) - 380.0f; // Push to the right edge
        }
        else {
            arrowX = -(winW * 0.5f) + 380.0f; // Push to the left edge
        }

        // Hitbox logic using world coordinates
        bool isHovering = (wx >= arrowX - arrowW * 1.5f && wx <= arrowX + arrowW * 1.5f &&
            wy >= arrowY - arrowH * 1.2f && wy <= arrowY + arrowH * 1.2f);

        float drawScale = isHovering ? 1.2f : 1.0f; // Slight pop effect on hover

        // 1. Draw the Arrow Sprite
        if (pTexArrows) {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);

            // Tint yellow on hover, white otherwise
            if (isHovering) AEGfxSetColorToMultiply(1.0f, 1.0f, 0.0f, 1.0f);
            else AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            AEGfxTextureSet(pTexArrows, 0.0f, 0.0f);

            if (leaderboard_mode == "ZEN") {
                // Show RIGHT arrow to switch to TIME mode
                Mesh_Renderer(pMeshRightArrow, arrowX, arrowY, arrowW * drawScale, arrowH * drawScale, 0.0f);
            }
            else {
                // Show LEFT arrow to switch to ZEN mode
                Mesh_Renderer(pMeshLeftArrow, arrowX, arrowY, arrowW * drawScale, arrowH * drawScale, 0.0f);
            }

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Reset color
        }

        // 2. Draw helper text next to the arrow
        if (Graphics::font_id >= 0) {
            const char* lbl = (leaderboard_mode == "ZEN") ? "TIME MODE" : "ZEN MODE";
            float tw, th;
            AEGfxGetPrintSize(Graphics::font_id, lbl, 0.45f, &tw, &th);

            float ndcX = 0.0f;
            if (leaderboard_mode == "ZEN") {
                // Text sits to the LEFT of the right arrow
                ndcX = (arrowX - arrowW * 0.5f) / (winW * 0.5f) - tw - 0.05f;
            }
            else {
                // Text sits to the RIGHT of the left arrow
                ndcX = (arrowX + arrowW * 0.5f) / (winW * 0.5f) + 0.05f;
            }

            float ndcY = (arrowY - 10.0f) / (winH * 0.5f);

            if (isHovering) {
                AEGfxPrint(Graphics::font_id, lbl, ndcX, ndcY, 0.45f, 1.0f, 1.0f, 0.0f, 1.0f);
            }
            else {
                AEGfxPrint(Graphics::font_id, lbl, ndcX, ndcY, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

        // 3. Click Logic
        if (isHovering && AEInputCheckTriggered(AEVK_LBUTTON)) {
            SoundManager::PlayClickBtnSfx();
            leaderboard_mode = (leaderboard_mode == "ZEN") ? "TIME" : "ZEN";
        }
    }
    else if (inSettings)                                                                                                         // process settings UI
    {
        if (SysSettings::DisplaySettings() == false)                                                                        // run settings loop
        {
            inSettings = false;                                                                                             // close if toggled false
        }
    }
    else if (isEnteringName)                                                                                                // process entering name state
    {
        Main_Menu_EnterName(0.0f, 0.0f, true);                                                                              // normal input and draw
    }
    else if (isZoomingToGame) {
        vCamY = winH;
    }
    else if (isTransitioningToName || isTransitioningBack)                                                                  // process transition states
    {
        Draw_Main_Menu(textNdcOffset);                                                                                      // draw menu moving

        float nameWorldY = winH;                                                                                            // calc relative Y
        float nameNdcY = 2.0f + textNdcOffset;                                                                              // calc relative NDC
        Main_Menu_EnterName(nameWorldY, nameNdcY, false);                                                                   // draw input screen moving
    }
    else                                                                                                                    // process default menu
    {
        Draw_Main_Menu(0.0f);                                                                                               // render standard visual
        Main_Menu_Input();                                                                                                  // scan for normal button clicks
        UiPauseMenu::Ui_DrawConfirmationPopup(pMeshBtn, nullptr);
    }

    // ---------------------------------------------------------------------------
    // SPLASH OVERLAY – drawn on top of everything else, fades out then disables
    // ---------------------------------------------------------------------------
    if (splashActive)
    {
        splashTimer += dt;

        // Hold fully opaque, then linearly fade to 0
        float alpha = 1.0f;
        if (splashTimer > SPLASH_HOLD)
        {
            alpha = 1.0f - (splashTimer - SPLASH_HOLD) / SPLASH_FADE;
            if (alpha < 0.0f) alpha = 0.0f;
        }

        if (alpha <= 0.0f)
        {
            // Fade complete – free resources
            splashActive = false;
            if (pMeshSplashQuad) { AEGfxMeshFree(pMeshSplashQuad); pMeshSplashQuad = nullptr; }
            if (pMeshSplashLogo) { AEGfxMeshFree(pMeshSplashLogo); pMeshSplashLogo = nullptr; }
            if (pTexSplashLogo) { AEGfxTextureUnload(pTexSplashLogo); pTexSplashLogo = nullptr; }
        }
        else
        {
            // Camera must be at origin so the overlay isn't offset
            AEGfxSetCamPosition(0.0f, 0.0f);

            // --- Full-screen black quad ---
            AEMtx33 scaleMat, transform;
            AEMtx33Scale(&scaleMat, winW * 0.5f, winH * 0.5f);
            AEMtx33Trans(&transform, 0.0f, 0.0f);
            AEMtx33Concat(&transform, &transform, &scaleMat);

            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(alpha);
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, alpha);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxSetTransform(transform.m);
            AEGfxMeshDraw(pMeshSplashQuad, AE_GFX_MDM_TRIANGLES);

            // --- DigiPen logo centred, 50% screen width, aspect 3.5:1 ---
            if (pTexSplashLogo && pMeshSplashLogo)
            {
                const float logoW = winW * 0.5f;
                const float logoH = logoW / 3.5f;

                AEMtx33 logoScale, logoTrans, logoTransform;
                AEMtx33Scale(&logoScale, logoW * 0.5f, logoH * 0.5f);
                AEMtx33Trans(&logoTrans, 0.0f, 0.0f);
                AEMtx33Concat(&logoTransform, &logoTrans, &logoScale);

                AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(alpha);
                AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, alpha);
                AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
                AEGfxTextureSet(pTexSplashLogo, 0.0f, 0.0f);
                AEGfxSetTransform(logoTransform.m);
                AEGfxMeshDraw(pMeshSplashLogo, AE_GFX_MDM_TRIANGLES);
            }
        }

        // Restore render state for next frame
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

/*
@function       Main_Menu_Exit
@brief          Cleans up all textures and meshes allocated during Main_Menu_Init.
@usage          GameStateManager
*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
void Main_Menu_Exit()
{
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);
    SysSettings::Exit();
    if (gGameStateNext != GS_SHOP && gGameStateNext != GS_CREDITS) {
        BackgroundParallax::MenuParallax_Exit();
        SoundManager::Exit();
    }
    ScreenTransitions::isTransitioning = false;                                                                             // safety clean transition
    AEGfxSetCamPosition(0.0f, 0.0f);                                                                                        // safety reset camera

    // Free Meshes
    if (pMeshLogo)
    {
        AEGfxMeshFree(pMeshLogo);                                                                                           // dealloc memory
        pMeshLogo = nullptr;                                                                                                // strip pointer
    }
    if (pMeshBtn)
    {
        AEGfxMeshFree(pMeshBtn);                                                                                            // dealloc memory
        pMeshBtn = nullptr;                                                                                                 // strip pointer
    }
    if (pMeshExitBtn)
    {
        AEGfxMeshFree(pMeshExitBtn);                                                                                        // dealloc memory
        pMeshExitBtn = nullptr;                                                                                             // strip pointer
    }
    if (pMeshSettingsBtn)
    {
        AEGfxMeshFree(pMeshSettingsBtn);                                                                                    // dealloc memory
        pMeshSettingsBtn = nullptr;                                                                                         // strip pointer
    }
    if (pMeshLeftArrow)
    {
        AEGfxMeshFree(pMeshLeftArrow);                                                                                      // dealloc memory
        pMeshLeftArrow = nullptr;                                                                                           // strip pointer
    }
    if (pMeshRightArrow)
    {
        AEGfxMeshFree(pMeshRightArrow);                                                                                     // dealloc memory
        pMeshRightArrow = nullptr;                                                                                          // strip pointer
    }
    if (pMeshLeaderboardBtn)
    {
        AEGfxMeshFree(pMeshLeaderboardBtn);                                                                                     // dealloc memory
        pMeshLeaderboardBtn = nullptr;
    }

    if (pMeshEnterName)
    {
        AEGfxMeshFree(pMeshEnterName);
        pMeshEnterName = nullptr;
    }
    if (pMeshBackName)
    {
        AEGfxMeshFree(pMeshBackName);
        pMeshBackName = nullptr;
    }
    if (pMeshCreditsBtn)
    {
        AEGfxMeshFree(pMeshCreditsBtn);
        pMeshCreditsBtn = nullptr;
    }
    if (pMeshTutorialBtn) {
        AEGfxMeshFree(pMeshTutorialBtn);
        pMeshTutorialBtn = nullptr;
    }

    // Free Textures
    if (pTexPlay)
    {
        AEGfxTextureUnload(pTexPlay);                                                                                       // unload from graphics
        pTexPlay = nullptr;                                                                                                 // strip pointer
    }
    if (pTexShop)
    {
        AEGfxTextureUnload(pTexShop);                                                                                       // unload from graphics
        pTexShop = nullptr;                                                                                                 // strip pointer
    }
    if (pTexArrows)
    {
        AEGfxTextureUnload(pTexArrows);                                                                                     // unload from graphics
        pTexArrows = nullptr;                                                                                               // strip pointer
    }
    if (pTexLogo) {
        AEGfxTextureUnload(pTexLogo);                                                                                     // unload from graphics
        pTexLogo = nullptr;                                                                                               // strip pointer
    }
    if (pTexEnterName) {
        AEGfxTextureUnload(pTexEnterName);                                                                                     // unload from graphics
        pTexEnterName = nullptr;
    }
    if (pTexBackName) {
        AEGfxTextureUnload(pTexBackName);                                                                                     // unload from graphics
        pTexBackName = nullptr;
    }
    if (pTexLeaderboard) {
        AEGfxTextureUnload(pTexLeaderboard);
        pTexLeaderboard = nullptr;
    }
    if (pTexSettings) {
        AEGfxTextureUnload(pTexSettings);
        pTexSettings = nullptr;
    }
    if (pTexTutorial) {
        AEGfxTextureUnload(pTexTutorial);
        pTexTutorial = nullptr;
    }

    if (pTexCredits)
    {
        AEGfxTextureUnload(pTexCredits);
        pTexCredits = nullptr;
    }

    // Splash overlay safety cleanup (normally freed after fade, guard anyway)
    if (pMeshSplashQuad) { AEGfxMeshFree(pMeshSplashQuad); pMeshSplashQuad = nullptr; }
    if (pMeshSplashLogo) { AEGfxMeshFree(pMeshSplashLogo); pMeshSplashLogo = nullptr; }
    if (pTexSplashLogo) { AEGfxTextureUnload(pTexSplashLogo); pTexSplashLogo = nullptr; }
    splashActive = false;

    ParallaxZoom::Reset();
    isZoomingToGame = false;
    Leaderboard_System::Free();


}