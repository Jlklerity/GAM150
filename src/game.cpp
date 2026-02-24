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
#include "DebugConsole.h"
#include "GameOver.h"
#include "Pause.h"


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
    Leaderboard_System::Init();
	//Shop_Init();
	PowerUpSystem::Init(); // azfar code ver 1

    DebugConsole::Init(
        is_enemy_area_box_visible,
        is_random_spawning_enabled,
        is_difficulty_debug_visible,
        health,
        enemy_pool,
        free_slot_queue,
        enemy_area,
        word_bank
    );
}


void Game_Update()
{

    DebugConsole::Update();
    if (DebugConsole::IsOpen()) goto render_only; // freeze everything while console open
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
       // DebugSystem::Debug_HandleInputs(is_enemy_area_box_visible, is_random_spawning_enabled, is_difficulty_debug_visible, health, enemy_pool, free_slot_queue, enemy_area, word_bank);
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
    

    // ------------------------------------------------------------------------
    // 3. RENDERING (Always run so the screen doesn't go black)
    // ------------------------------------------------------------------------

    /*_________________________________________________________________________________________________________________________________________*/
render_only:
    //DebugConsole::Render(); // draw console on top of everything
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
    UiPauseMenu::Check_Paused(isPaused, isDead, bgMesh);

    // Game Over & Leaderboard OVERLAY
    UiGameOver::Check_IfDead(isDead, isGameOver, openLeaderboard, bgMesh, temp_name, score);
    
    DebugConsole::Render();
}

void Game_Exit()
{
	EnemyManage::EnemyManage_FreeMeshes(meshes);

	Typing_Exit();
    Leaderboard_System::Free();
	PowerUpSystem::Free();
    DebugConsole::Exit();

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