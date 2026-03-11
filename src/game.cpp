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
#include "Particles.h"
#include "GameOver.h"
#include "Pause.h"
#include "Weather.h"
#include "Boss.h"


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
    @overview   Enemy and Boss
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/

    // Enemy
    Data::WordBank      word_bank{};                                                                                        // create word bank container
    Data::Health        health{};                                                                                           // create health state
    Data::Score         score{};                                                                                            // create score state
    Data::SpriteAnim    enemy_animation{};                                                                                  // create enemy animation state
    Data::EnemyArea     enemy_area{};                                                                                       // create enemy area limits
    Data::Meshes        meshes{};                                                                                           // create meshes

    std::vector<Data::Enemy> enemy_pool((size_t)EnemyConfig::MAX_ENEMIES);                                                  // create enemy pool
    std::deque<int> free_slot_queue;                                                                                        // create free slot queue

    bool is_enemy_area_box_visible = false;                                                                                 // toggle for debug box drawing
    bool is_random_spawning_enabled = true;                                                                                 // toggle for auto spawning
    bool is_difficulty_debug_visible = false;                                                                               // toggle for difficulty text

    float random_spawn_timer_seconds = 9999.0f;                                                                             // start high to spawn immediately

    // Boss
    bool boss_spawning_blocked = false;                                                                                     // tracks whether the boss is blocking normal enemy spawning
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
    DifficultySystem::Difficulty_Reset();
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
    @overview   Enemy and Boss
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/

    // Enemy
    EnemyManage::EnemyManage_InitEngine(word_bank);                                                                         // start engine and load words
    HealthSystem::Health_Init(health);                                                                                      //Start game with full health
    meshes = EnemyManage::EnemyManage_LoadMeshes(enemy_animation);                                                          // load textures and meshes
    enemy_area.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f);                                      // place spawn line above the screen

    enemy_pool.resize((size_t)EnemyConfig::MAX_ENEMIES);                                                                    // allocate enemy pool to the configured maximum size
    free_slot_queue.clear();                                                                                                // clear any previous free-slot indices before repopulating

    for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)                                                                      // loop through pool slots
    {
        enemy_pool[(size_t)i].enemy_is_active = false;                                                                      // mark slot inactive
        free_slot_queue.push_back(i);                                                                                       // push slot index into free queue
    }

    // Boss
    boss_spawning_blocked = false;                                                                                          // make sure normal spawning is allowed at the start
    BossSystem::Boss_Init(meshes, word_bank);                                                                               // load boss assets and reset all boss state
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
    Particles_Init();
    Weather_Init();

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
        float dt = (float)AEFrameRateControllerGetFrameTime();                                               // read delta time for this frame
        if (dt > 0.1f)
        {
            // Force it to a safe standard frame time (~60 FPS)
            dt = 0.01667f;
        }
        dt *= PowerUpSystem::GetTimeScale(); // azfar code

        /*
        @overview   Boss update and enemy spawning
        @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        DifficultySystem::Difficulty_UpdateOverTime(dt);                                                                              // increase difficulty over time

        BossSystem::Boss_Update(enemy_pool, free_slot_queue, enemy_area, word_bank, dt, boss_spawning_blocked);                       // run boss logic for this frame

        if (boss_spawning_blocked)                                                                                                    // check if the boss is preventing normal spawns
            random_spawn_timer_seconds = 0.0f;                                                                                        // reset the spawn timer so no enemy spawns while boss is active

        if (is_random_spawning_enabled && !boss_spawning_blocked)                                                                     // only spawn normal enemies when spawning is on and the boss is not active
        {
            random_spawn_timer_seconds += dt;                                                                                         // count up toward the next spawn
            float interval = DifficultySystem::Difficulty_GetSpawnIntervalSeconds();                                                  // get how many seconds to wait between spawns based on current difficulty

            if (random_spawn_timer_seconds >= interval)                                                                               // check if enough time has passed to spawn the next enemy
            {
                random_spawn_timer_seconds = 0.0f;                                                                                    // reset the timer after a spawn
                int roll = Utility::Utility_RandInt(0, 99);                                                                           // roll a random number to decide which enemy type to spawn
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();                                                // get the longest word allowed at the current difficulty level

                if (roll < 15)                                                                                                        // 15% chance: spawn a friendly enemy
                {
                    EnemySystem::Enemy_SpawnFriendlyFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.15f);
                }
                else if (roll < 30)                                                                                                   // 15% chance: spawn a shielded enemy
                {
                    EnemySystem::Enemy_SpawnShieldedFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.05f);
                }
                else                                                                                                                  // 70% chance: spawn a normal enemy
                {
                    int len = DifficultySystem::Difficulty_GetRandomWordLength();                                                     // pick a random word length within the current difficulty range
                    EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, len);
                }
            }
        }

        for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)                                                                            // loop through all pool slots
        {
            Data::Enemy& e = enemy_pool[(size_t)i];                                                                                   // reference enemy slot
            bool was_active = e.enemy_is_active;                                                                                      // remember if this enemy was alive before the update
            bool was_damage_done = e.enemy_did_damage;                                                                                // remember if this enemy already dealt its damage before the update

            if (BossSystem::Boss_GetKillWordSlot() >= 0 && i == BossSystem::Boss_GetKillWordSlot()) continue;                         // skip the boss kill word slot so Boss_Update stays in full control of it

            EnemySystem::Enemy_Update(e, enemy_area, dt, health);                                                                     // run movement, animation, landing and damage logic for this enemy

            if (was_active && !e.enemy_is_active)                                                                                     // check if the enemy just became inactive this frame
            {
                free_slot_queue.push_back(i);                                                                                         // return this slot to the free list so it can be reused

                if (!was_damage_done)                                                                                                 // check if the enemy was killed by the player before landing
                    Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);                                              // spawn a damage hit effect since the enemy was killed by the player
                else                                                                                                                  // enemy landed and dealt damage before being killed
                    Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);                           // spawn a landing impact effect since the enemy reached the bottom
            }
        }
        EnemySystem::Enemy_ResolveLaneStacking(enemy_pool, enemy_area);                                                               // fix spacing in lanes
        /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

        PowerUpSystem::Update(dt, enemy_pool, enemy_area);
        Particles_Update(dt);
        Weather_Update(dt, enemy_pool);
        // mouse click -> convert to world coords
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;
            float wy = 450.0f - (float)my;
            PowerUpSystem::OnMouseClick(wx, wy, enemy_pool);
        } // azfar code

        // MOVED TYPING LOGIC HERE (so it pauses)
        // Gate typing while player is frozen (snow) or stunned (thunder)
        if (!Weather_IsPlayerFrozen() && !Weather_IsPlayerStunned())
            Typing_UpdateAndApplyToEnemies(enemy_pool, score, health);

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
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    if (is_enemy_area_box_visible)                                                                                          // check debug draw flag
    {
        UiSystem::Debug_DrawEnemyAreaBox(meshes, enemy_area);                                                               // draw enemy area box
        UiSystem::Debug_DrawEnemyLaneLines(meshes, enemy_area);                                                             // draw lane lines
    }

    for (int di = 0; di < (int)enemy_pool.size(); ++di)                                                                     // loop through enemies for sprite draw
    {
        if (BossSystem::Boss_GetKillWordSlot() >= 0 && di == BossSystem::Boss_GetKillWordSlot()) continue;                  // skip the boss kill word slot since Boss_Draw handles it separately
        EnemySystem::Enemy_Draw(meshes, enemy_pool[(size_t)di]);                                                            // draw enemy sprite
    }

    for (int di = 0; di < (int)enemy_pool.size(); ++di)                                                                     // loop through enemies for word draw
    {
        if (BossSystem::Boss_GetKillWordSlot() >= 0 && di == BossSystem::Boss_GetKillWordSlot()) continue;                  // skip the boss kill word slot since Boss_Draw handles it separately
        EnemySystem::Enemy_DrawWord(enemy_pool[(size_t)di]);                                                                // draw enemy word
    }

    BossSystem::Boss_Draw(meshes);                                                                                          // draw the boss sprite, kill word label and progress counter

    Particles_Draw();

    UiSystem::Debug_DrawDifficultyTopRight(is_difficulty_debug_visible, is_random_spawning_enabled, health);                // draw debug difficulty ui with health
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    UiSystem::Ui_DrawScoreAndTyped(score); // draw score and typed buffer

    PowerUpSystem::Draw(); // azfar code

    Weather_Draw();

    UiSystem::Ui_DrawHealthBar(health, meshes); // jeremy code

    /*
    @overview   Boss flash overlay draw
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    BossSystem::Boss_DrawFlashOverlay(meshes);                                                                              // draw the white flash overlay on top of everything else
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    // PAUSE OVERLAY
    UiPauseMenu::Check_Paused(isPaused, isDead, bgMesh);

    // Game Over & Leaderboard OVERLAY
    UiGameOver::Check_IfDead(isDead, isGameOver, openLeaderboard, bgMesh, temp_name, score);

    DebugConsole::Render();
}

void Game_Exit()
{
    /*
    @overview   Boss and enemy cleanup
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    BossSystem::Boss_Free(meshes, free_slot_queue, enemy_pool);                                                             // unload boss assets and return the reserved pool slot
    EnemyManage::EnemyPool_Free(enemy_pool, free_slot_queue);                                                               // deactivate all enemy slots and rebuild the free slot queue
    EnemyManage::EnemyManage_FreeMeshes(meshes);                                                                            // unload all enemy textures and meshes from memory
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    Typing_Exit();
    Leaderboard_System::Free();
    PowerUpSystem::Free();
    Particles_Free();
    Weather_Exit();
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

    if (playerTexture1)
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