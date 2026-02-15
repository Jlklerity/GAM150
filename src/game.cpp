#include "Game.h"
#include <crtdbg.h> // To check for memory leaks
#include "AEEngine.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Typing.h"
#include "Shop.h"
#include "GameStateManager.h"
#include "Leaderboard.h"
#include "Enemy.h"
#include "PowerUp.h"
#include "MainMenu.h"

namespace {
	//avatar mesh and texture
	AEGfxTexture* playerTexture1 = nullptr; // Default Skin
	AEGfxTexture* playerTexture2 = nullptr; // Alternate Skin
	AEGfxVertexList* playerMesh = nullptr;

	//background mesh and texture
	AEGfxTexture* bgTexture1 = nullptr; // Default BG
	AEGfxTexture* bgTexture2 = nullptr; // Alternate BG
	AEGfxVertexList* bgMesh = nullptr;

	const float spriteX = -700.0f;
	const float spriteY = -200.0f;

	// Animation State
	float animTimer;
	float animDuration; // Speed of animation
	int currentFrameIndex;
	float uvOffsetX;
	float uvOffsetY;

	// Constants for the spritesheet
	const int SPRITE_COLS = 8;
	const int SPRITE_ROWS = 1;
	const float SPRITE_UV_WIDTH = 1.0f / SPRITE_COLS;
	const float SPRITE_UV_HEIGHT = 1.0f / SPRITE_ROWS;
  
	bool isPaused = false;
    bool isDead = false;
    bool openLeaderboard = false;

	/*
	@overview   Enemy
	@author     Wong Zi Yuan (w.ziyuan@digipen.edu)
	*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
	Data::WordBank      word_bank{};                                                               // create word bank container
	Data::Health        health{};                                                                  // create health state
	Data::Score         score{};                                                                   // create score state
	Data::SpriteAnim    enemy_animation{};                                                         // create enemy animation state
	Data::EnemyArea     enemy_area{};                                                              // create enemy area limits
	Data::Meshes        meshes{};                                                                  // create meshes

	std::vector<Data::Enemy> enemy_pool((size_t)EnemyConfig::MAX_ENEMIES);                         // create enemy pool
	std::deque<int> free_slot_queue;                                                               // create free slot queue

	bool is_enemy_area_box_visible = false;                                                        // toggle for debug box drawing
	bool is_random_spawning_enabled = true;                                                        // toggle for auto spawning
	bool is_difficulty_debug_visible = false;                                                      // toggle for difficulty text
    
	float random_spawn_timer_seconds = 9999.0f;                                                    // start high to spawn immediately
	/*_______________________________________________________________________________________________________________________________________________________________________________________________*/

	// Typing state
	int typed_target_index = -1; 
    bool isGameOver;
    std::string temp_name;
}

void Game_Init()
{
	animTimer = 0.0f;
	animDuration = 0.1f; 
	currentFrameIndex = 0;
	uvOffsetX = 0.0f;
	uvOffsetY = 0.0f;
    score.score_value = 0;
    isGameOver = false;
    openLeaderboard = false;
    isPaused = false;      
    isDead = false;

	//player
	playerTexture1 = AEGfxTextureLoad("Assets/characterSheet.png");
	playerTexture2 = AEGfxTextureLoad("Assets/characterSheet.png");//for ltr when we have more player skins
	AE_ASSERT_MESG(playerTexture1, "Failed to load characterSheet!");

	if (playerMesh != nullptr) AEGfxMeshFree(playerMesh);
	playerMesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

	// Background
	bgTexture1 = AEGfxTextureLoad("Assets/background.png");
	bgTexture2 = AEGfxTextureLoad("Assets/background.png"); //for ltr when we have more bg skins
	AE_ASSERT_MESG(bgTexture1, "Failed to load background!");

	/*
	@overview   Enemy
	@author     Wong Zi Yuan (w.ziyuan@digipen.edu)
	*//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
	EnemyManage::EnemyManage_InitEngine(word_bank);                                                // start engine and load words
    HealthSystem::Health_Init(health);              //Start game with full health
	meshes = EnemyManage::EnemyManage_LoadMeshes(enemy_animation);                                 // load textures and meshes
	enemy_area.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f);             // place spawn line above the screen
	
	enemy_pool.resize((size_t)EnemyConfig::MAX_ENEMIES);                                           // allocate enemy pool to the configured maximum size
	free_slot_queue.clear();                                                                       // clear any previous free-slot indices before repopulating

	for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)                                             // loop through pool slots
	{
		enemy_pool[(size_t)i].enemy_is_active = false;                                             // mark slot inactive
		free_slot_queue.push_back(i);                                                              // push slot index into free queue
	}
	/*_______________________________________________________________________________________________________________________________________________________________________________________________*/
	
	bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    
	Typing_Init();

	//Shop_Init();
	PowerUpSystem::Init(); // azfar code ver 1

}


void Game_Update()
{
    // ------------------------------------------------------------------------
    // 1. INPUTS
    // ------------------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_ESCAPE)) {
        isPaused = !isPaused; // pauses game
    }

    //if (AEInputCheckTriggered(AEVK_TAB)) {
    //    isShopOpen = !isShopOpen; // toggle shop menu and resume gameplay
    //}
    

    // ------------------------------------------------------------------------
    // 2. GAMEPLAY LOGIC (Run only when active)
    // ------------------------------------------------------------------------
    if (health.health_current == 0) isDead = 1; //check if player is dead

    if (!isPaused && !isDead)
    {
        /*
        @overview   Debug section
        @author     Wong Zi Yuan (w.ziyuan@digipen.edu)

        @info       Control Key + "1" -> Toggle debug box
                    Control Key + "2" -> Add health (10.0)
                    Control Key + "3" -> Spawn 3-letter enemy
                    Control Key + "4" -> Spawn 4-letter enemy
                    Control Key + "5" -> Spawn 5-letter enemy
                    Control Key + "6" -> Spawn 6-letter enemy
                    Control Key + "7" -> Spawn 7-letter enemy
                    Control Key + "8" -> Spawn 8-letter enemy
                    Control Key + "9" -> Toggle on/off auto enemy spawn
                    Control Key + "0" -> Toggle difficulty display
                    Control Key + "-" -> Decrease difficulty
                    Control Key + "=" -> Increase difficulty
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        DebugSystem::Debug_HandleInputs(is_enemy_area_box_visible, is_random_spawning_enabled, is_difficulty_debug_visible, health, enemy_pool, free_slot_queue, enemy_area, word_bank);
        /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

        float dt = (float)AEFrameRateControllerGetFrameTime();                                               // read delta time for this frame
        dt *= PowerUpSystem::GetTimeScale(); // azfar code

        DifficultySystem::Difficulty_UpdateOverTime(dt);                                                     // increase difficulty over time

        if (is_random_spawning_enabled)                                                                      // check if auto spawning is on
        {
            random_spawn_timer_seconds += dt;                                                                // add dt to spawn timer
            float interval = DifficultySystem::Difficulty_GetSpawnIntervalSeconds();                         // get current spawn interval

            if (random_spawn_timer_seconds >= interval)                                                      // check if time to spawn
            {
                random_spawn_timer_seconds = 0.0f;                                                           // reset spawn timer
                int len = DifficultySystem::Difficulty_GetRandomWordLength();                                // pick random word length
                EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, len);   // spawn enemy
            }
        }

        for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)                                                   // loop through all pool slots
        {
            Data::Enemy& e = enemy_pool[(size_t)i];                                                          // reference enemy slot
            bool was_active = e.enemy_is_active;                                                             // remember active state before update

            EnemySystem::Enemy_Update(e, enemy_area, dt, health);                                            // update enemy movement and state

            if (was_active && !e.enemy_is_active)                                                            // check if enemy just became inactive
            {
                free_slot_queue.push_back(i);                                                                // return slot index to free queue

            }
        }

        PowerUpSystem::Update(dt, enemy_pool);

        // mouse click -> convert to world coords
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;
            float wy = 450.0f - (float)my;
            PowerUpSystem::OnMouseClick(wx, wy, enemy_pool);
        } // azfar code

        EnemySystem::Enemy_ResolveLaneStacking(enemy_pool, enemy_area);                                      // fix spacing in lanes

        // MOVED TYPING LOGIC HERE (so it pauses)
        Typing_UpdateAndApplyToEnemies(enemy_pool, score);

        // MOVED ANIMATION LOGIC HERE (so it pauses)
        // Update timer
        animTimer += (float)AEFrameRateControllerGetFrameTime();

        if (animTimer >= animDuration)
        {
            animTimer = 0.0f;

            // Move to next frame
            currentFrameIndex++;
            if (currentFrameIndex >= 8) // Max sprites
                currentFrameIndex = 0;

            // Calculate UV Offset (Which "window" of the image to look at)
            int col = currentFrameIndex % SPRITE_COLS;
            int row = currentFrameIndex / SPRITE_COLS;

            uvOffsetX = col * SPRITE_UV_WIDTH;
            uvOffsetY = row * SPRITE_UV_HEIGHT;
        }
    }
    //else if (isShopOpen)
    //{
    //    // If shop is open, just run the Shop Module
    //    Shop_Update();
    //}

    // ------------------------------------------------------------------------
    // 3. RENDERING (Always run so the screen doesn't go black)
    // ------------------------------------------------------------------------

    /*_________________________________________________________________________________________________________________________________________*/

    //background
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Reset Color

    AEGfxTexture* activeBg = (gShopBgID == 0) ? bgTexture1 : bgTexture2;
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    //// Optional tint for demo purposes if textures are identical
    //if (gShopBgID == 1) AEGfxSetColorToMultiply(0.5f, 0.5f, 0.7f, 1.0f); // change background color
    Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    //player
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    AEGfxTexture* activePlayer = (gShopSkinID == 0) ? playerTexture1 : playerTexture2;
    AEGfxTextureSet(activePlayer, uvOffsetX, uvOffsetY);

    if (gShopSkinID == 1) AEGfxSetColorToMultiply(1.0f, 0.6f, 0.6f, 1.0f); // change player skin color
    Mesh_Renderer(playerMesh, spriteX, spriteY, 150.0f, 150.0f, 0.0f);

    // Render typing letters 
    
    Typing_Render();
    

    /*
    @overview   Draw enemies and debug overlays
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_______________________________________________________________________________________________________________________________________*/

    if (is_enemy_area_box_visible)                                                                       // check debug draw flag
    {
        UiSystem::Debug_DrawEnemyAreaBox(meshes, enemy_area);                                            // draw enemy area box
        UiSystem::Debug_DrawEnemyLaneLines(meshes, enemy_area);                                          // draw lane lines
    }

    for (const auto& e : enemy_pool)                                                                     // loop through enemies for sprite draw
        EnemySystem::Enemy_Draw(meshes, e);                                                              // draw enemy sprite

    for (const auto& e : enemy_pool)                                                                     // loop through enemies for word draw
        EnemySystem::Enemy_DrawWord(e);                                                                  // draw enemy word

    UiSystem::Debug_DrawDifficultyTopRight(is_difficulty_debug_visible,
        is_random_spawning_enabled, health);                                                             // draw debug difficulty ui with health
    /*_________________________________________________________________________________________________________________________________________*/

    UiSystem::Ui_DrawScoreAndTyped(score);                                                               // draw score and typed buffer

    PowerUpSystem::Draw(); // azfar code

    UiSystem::Ui_DrawHealthBar(health, meshes); // jeremy code

    // PAUSE OVERLAY
    if (isPaused && !isDead)
    {
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
            float btnH = 60.0f;
            float btnX = 0.0f; // Centered Horizontally

            // Y Positions
            float resumeY = -50.0f;
            float menuY = -150.0f;

            // ========================================================================
            // Draw "RESUME" Button
            // ========================================================================
            // 1. Background
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
            Mesh_Renderer(bgMesh, btnX, resumeY, btnW, btnH, 0.0f);

            // 2. Text
            const char* resumeText = "RESUME";
            float rw, rh;
            AEGfxGetPrintSize(Graphics::font_id, resumeText, 0.6f, &rw, &rh);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            // -0.15f Y is approx -50.0f world units
            AEGfxPrint(Graphics::font_id, resumeText, -rw * 0.5f, -0.15f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

            // ========================================================================
            // Draw "MAIN MENU" Button
            // ========================================================================
            // 1. Background
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f);
            Mesh_Renderer(bgMesh, btnX, menuY, btnW, btnH, 0.0f);

            // 2. Text
            const char* menuText = "MAIN MENU";
            float mw, mh;
            AEGfxGetPrintSize(Graphics::font_id, menuText, 0.6f, &mw, &mh);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            // -0.37f Y is approx -150.0f world units
            AEGfxPrint(Graphics::font_id, menuText, -mw * 0.5f, -0.37f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);

            // ========================================================================
            // Handle Mouse Clicks
            // ========================================================================
            if (AEInputCheckTriggered(AEVK_LBUTTON))
            {
                s32 mx, my;
                AEInputGetCursorPosition(&mx, &my);

                float wx = (float)mx - 800.0f;
                float wy = 450.0f - (float)my;

                float halfW = btnW * 0.5f;
                float halfH = btnH * 0.5f;

                // Check RESUME
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                    wy >= (resumeY - halfH) && wy <= (resumeY + halfH))
                {
                    isPaused = false; // Just unpause
                }

                // Check MAIN MENU
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) &&
                    wy >= (menuY - halfH) && wy <= (menuY + halfH))
                {
                    isPaused = false;
                    temp_name = "";
                    ResetPlayerName(temp_name);
                    gGameStateNext = GS_MAIN_MENU;
                }
            }
        }
    }

    // Game Over & Leaderboard OVERLAY
    if (isDead) {

        // 1. Save the score once
        if (!isGameOver) {
            temp_name = Main_Menu_GetPlayerName();
            Leaderboard_System::Write_Leaderboard(temp_name, score.score_value);
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
                    temp_name = "";
                    ResetPlayerName(temp_name);
                    gGameStateNext = GS_MAIN_MENU;
                }

                // Check LEADERBOARD
                if (wx >= (btnX - halfW) && wx <= (btnX + halfW) && wy >= (leaderY - halfH) && wy <= (leaderY + halfH)) {
                    openLeaderboard = true;
                }
            }
        }
        // ==========================================
            //LEADERBOARD OVERLAY
        // ==========================================
        else
        {
            // 1. Draw a dark translucent background
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(0.85f); // 85% opacity
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // Pitch Black

            // Reusing bgMesh
            Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

            // IMPORTANT: Reset transparency back to 1.0f so the text draws fully opaque!
            AEGfxSetTransparency(1.0f);

            // 2. Draw the actual scores
            Leaderboard_System::Display_Leaderboard();

            // 3. Draw a prompt so they know how to leave
            if (Graphics::font_id >= 0) {
                const char* promptText = "Press ESC or Click to Return";
                float pw, ph;
                AEGfxGetPrintSize(Graphics::font_id, promptText, 0.5f, &pw, &ph);
                AEGfxPrint(Graphics::font_id, promptText, -pw * 0.5f, -0.8f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f);
            }

            // 4. Return to the Game Over buttons if they click or hit Escape
            if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON)) {
                openLeaderboard = false;
            }
        }
    }
}

void Game_Exit()
{
	EnemyManage::EnemyManage_FreeMeshes(meshes);

	Typing_Exit();
	//Shop_Exit();
	PowerUpSystem::Free();
	if (playerMesh) {
		AEGfxMeshFree(playerMesh);
		playerMesh = nullptr;
	}

	if (bgMesh) {
		AEGfxMeshFree(bgMesh);
		bgMesh = nullptr;
	}

	if (bgTexture1)
	{
		AEGfxTextureUnload(bgTexture1);
		bgTexture1 = nullptr;
	}
	if (bgTexture2)
	{
		AEGfxTextureUnload(bgTexture2);
		bgTexture2 = nullptr;
	}

	if(playerTexture1)
	{
		AEGfxTextureUnload(playerTexture1);
		playerTexture1 = nullptr;
	}
	if (playerTexture2)
	{
		AEGfxTextureUnload(playerTexture2);
		playerTexture2 = nullptr;
	}
}