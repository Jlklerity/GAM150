#include "Game.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Typing.h"
#include "Shop.h"
#include "GameStateManager.h"
#include "Enemy.h"
#include "PowerUp.h"

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
  
	// Toggle for Shop state
	bool isShopOpen = false;

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

	float random_spawn_timer_seconds = 999.0f;                                                     // start high to spawn immediately

	// Typing state (To be edited by Peng Fei)
	std::string typed_buffer;                                                                      // typed letters buffer
	int typed_target_index = -1;                                                                   // current target enemy index
	/*_______________________________________________________________________________________________________________________________________________________________________________________________*/
}

void Game_Init()
{
	animTimer = 0.0f;
	animDuration = 0.1f; 
	currentFrameIndex = 0;
	uvOffsetX = 0.0f;
	uvOffsetY = 0.0f;
	
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
	meshes = EnemyManage::EnemyManage_LoadMeshes(enemy_animation);                                 // load textures and meshes
	enemy_area.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f) + 400.0f;    // place spawn line above the screen
	
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
	Shop_Init();
	PowerUpSystem::Init(); // azfar code ver 1

}


void Game_Update()
{
	if (AEInputCheckTriggered(AEVK_TAB)) {
		isShopOpen = !isShopOpen; //toggle shop menu and resume gameplay
	}

	if (AEInputCheckTriggered(AEVK_ESCAPE)) {//press q to return to main menu
		gGameStateNext = GS_MAIN_MENU;
	}

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

	// Notes for Peng Fei
	// To kill enemy use "EnemySystem::Enemy_Killed(target)", target being the specific enemy to kill
	// To add score after killing enemy, "score.score_value += score", score being the specific points to give, can just do "+= 1"
	/*_______________________________________________________________________________________________________________________________________________________________________________________________*/
	/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
	/*_______________________________________________________________________________________________________________________________________________________________________________________________*/
	if (AEInputCheckTriggered(AEVK_RETURN))                                                              // enter pressed check
	{
		typed_buffer.clear();                                                                            // clear typed letters
		typed_target_index = -1;                                                                         // clear target lock
	}

	if (AEInputCheckTriggered(AEVK_BACK))                                                                // backspace pressed check
	{
		if (!typed_buffer.empty())                                                                       // check if there is something to delete
			typed_buffer.pop_back();                                                                     // remove last letter

		if (typed_buffer.empty())                                                                        // check if buffer is now empty
			typed_target_index = -1;                                                                     // clear target lock
	}

	char typed_char = 0;                                                                                 // typed character storage
	if (Utility::Input_GetTriggeredLetter(typed_char))                                                   // check for A-Z key press
	{
		typed_buffer.push_back(typed_char);                                                              // add letter to buffer

		if (typed_target_index >= 0)                                                                     // check if a target is already locked
		{
			if (typed_target_index >= (int)enemy_pool.size()                                             // check bounds
				|| !enemy_pool[(size_t)typed_target_index].enemy_is_active                               // check active
				|| enemy_pool[(size_t)typed_target_index].enemy_is_dying                                 // check not dying
				|| !Utility::String_StartsWith(enemy_pool[(size_t)typed_target_index].enemy_word_text,
					typed_buffer))                                                                       // check prefix still matches
			{
				typed_target_index = -1;                                                                 // unlock invalid target
			}
		}

		if (typed_target_index < 0)                                                                      // check if no target is locked
			typed_target_index = EnemySystem::Enemy_FindBestTargetByPrefix(enemy_pool, typed_buffer);    // find best target by prefix

		if (typed_target_index >= 0)                                                                     // check if target exists
		{
			Data::Enemy& target = enemy_pool[(size_t)typed_target_index];                                // reference target enemy
			if (target.enemy_is_active && !target.enemy_is_dying
				&& target.enemy_word_text == typed_buffer)                                               // check full word match
			{
				EnemySystem::Enemy_Killed(target);                                                       // start death drop

				score.score_value += target.enemy_word_length * 10;                                      // add score based on word length

				typed_buffer.clear();                                                                    // clear typed letters
				typed_target_index = -1;                                                                 // clear target lock
			}
		}
		else                                                                                             // handle no match case
		{
			typed_buffer.clear();                                                                        // clear typed letters on mismatch
		}
	}
	/*_______________________________________________________________________________________________________________________________________________________________________________________________*/
	/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
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

			// Notes for Peng Fei
			// Have to compare target index to i, then clear it by doing -1
			/*_________________________________________________________________________________________________________________________________*/
			/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
			/*_________________________________________________________________________________________________________________________________*/
			if (typed_target_index == i)                                                                 // check if target index points to this slot
				typed_target_index = -1;                                                                 // clear target lock
			/*_________________________________________________________________________________________________________________________________*/
			/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
			/*_________________________________________________________________________________________________________________________________*/
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
	/*_________________________________________________________________________________________________________________________________________*/

	//background
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_NONE);
	AEGfxSetTransparency(1.0f);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Reset Color

	AEGfxTexture* activeBg = (gShopBgID == 0) ? bgTexture1 : bgTexture2;
	AEGfxTextureSet(activeBg, 0.0f, 0.0f);
	// Optional tint for demo purposes if textures are identical
	if (gShopBgID == 1) AEGfxSetColorToMultiply(0.5f, 0.5f, 0.7f, 1.0f); // change background color
	Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

	//player
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

	AEGfxTexture* activePlayer = (gShopSkinID == 0) ? playerTexture1 : playerTexture2;
	AEGfxTextureSet(activePlayer, uvOffsetX, uvOffsetY);

	if (gShopSkinID == 1) AEGfxSetColorToMultiply(1.0f, 0.6f, 0.6f, 1.0f); // change player skin color
	Mesh_Renderer(playerMesh, spriteX, spriteY, 150.0f, 150.0f, 0.0f);

	if (isShopOpen)
	{
		// If shop is open, just run the Shop Module
		Shop_Update();
	}
	else 
	{
		Typing_Update();
		Typing_Render();

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


	/*_________________________________________________________________________________________________________________________________________*/
	/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
	/*_________________________________________________________________________________________________________________________________________*/
	UiSystem::Ui_DrawScoreAndTyped(score, typed_buffer);                                                 // draw score and typed buffer
	/*_________________________________________________________________________________________________________________________________________*/
	/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
	/*_________________________________________________________________________________________________________________________________________*/

	PowerUpSystem::Draw(); // azfar code
}

void Game_Exit()
{
	EnemyManage::EnemyManage_FreeMeshes(meshes);

	Typing_Exit();
	Shop_Exit();
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