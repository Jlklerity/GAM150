#include "pch.h"
#include "Game.h"


namespace {
    //avatar mesh and texture
    AEGfxTexture* playerTexture = nullptr;
    AEGfxVertexList* playerMesh = nullptr;

    //background mesh and texture
    AEGfxTexture* bgTexture1 = nullptr; // Default BG
    AEGfxTexture* bgTexture2 = nullptr; // Alternate BG
    AEGfxVertexList* bgMesh = nullptr;

    const float SPRITE_X = -700.0f;
    const float SPRITE_Y = -200.0f;

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
    bool wasPaused = false;
    bool isDead = false;
    bool openLeaderboard = false;

    static bool s_dayTransitionFired = false;

    /*
    @overview   Enemy and Boss
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/

    // Enemy
    Data::WordBank      word_bank{};                                                                                        // create word bank container
    Data::Health        health{};                                                                                           // create health state
    Data::Score         score{};                                                                                            // create score state
    Data::SpriteAnim    enemy_animation{};                                                                                  // create enemy animation state
    Data::SpriteAnim    heart_anim{};                                                                                       // create heart animation state

    Data::EnemyArea     enemy_area{};                                                                                       // create enemy area limits
    Data::Meshes        meshes{};                                                                                           // create meshes

    std::vector<Data::Enemy> enemy_pool((size_t)EnemyConfig::MAX_ENEMIES);                                                  // create enemy pool
    std::deque<int> free_slot_queue;                                                                                        // create free slot queue

    bool is_enemy_area_box_visible = false;                                                                                 // toggle for debug box drawing
    bool is_random_spawning_enabled = true;                                                                                 // toggle for auto spawning
    bool is_difficulty_debug_visible = false;                                                                               // toggle for difficulty text

    float random_spawn_timer_seconds = 9999.0f;                                                                             // start high to spawn immediately
    float frame_dt = 0.0f;                                                                                                  // delta time for current frame, shared between update and render

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
    wasPaused = false;
    isPaused = false;
    isDead = false;
    frame_dt = 0.0f;
    DifficultySystem::Difficulty_Reset();

    // Initialise shared heart animation state (6 frames, single row sprite sheet)
    heart_anim = {};
    heart_anim.animation_columns = 6;
    heart_anim.animation_rows = 1;
    heart_anim.animation_frame_duration_seconds = 0.10f;

    if (bgTexture1) { AEGfxTextureUnload(bgTexture1); bgTexture1 = nullptr; }
    if (bgTexture2) { AEGfxTextureUnload(bgTexture2); bgTexture2 = nullptr; }
    if (bgMesh) { AEGfxMeshFree(bgMesh);          bgMesh = nullptr; }

    GameClouds::Exit();
    DayCycle::Exit();
    LaserBeam_Free();
    StreakSystem::Exit();
    SoundManager::Exit();
    DeathTransition::Reset();

    s_dayTransitionFired = false;

    if (playerTexture) { AEGfxTextureUnload(playerTexture); playerTexture = nullptr; }


    //player texture
    const char* skinPath = Shop_GetActiveSkinPath();
    playerTexture = AEGfxTextureLoad(skinPath);
    AE_ASSERT_MESG(playerTexture, "Failed to load player skin!");

    if (playerMesh != nullptr) AEGfxMeshFree(playerMesh);
    playerMesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

    // Background
    bgTexture1 = AEGfxTextureLoad("Assets/transbackground.png");
    AE_ASSERT_MESG(bgTexture1, "Failed to load background!");
    bgTexture2 = AEGfxTextureLoad("Assets/floorground2.png"); //for ltr when we have more bg skins
    AE_ASSERT_MESG(bgTexture2, "Failed to load background!");

    /*
    @overview   Enemy and Boss
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    UpgradeSystem::Upgrade_Reset();                                                                                          // clear all upgrade bonuses
    GameEnemyBoss::Init(meshes, word_bank, health, enemy_animation, enemy_area, enemy_pool, free_slot_queue, boss_spawning_blocked);
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    GameClouds::Init();
    DayCycle::Init();
    Typing_Init();
    Leaderboard_System::Init();
    //Shop_Init();
    PowerUpSystem::Init(); // azfar code ver 1
    UpgradeSystem::Upgrade_Init();                                                                                          // load upgrade card assets and reset all bonuses for a fresh session

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


    LaserBeam_Init();
    LaserBeam_SetOrigin(SPRITE_X + 75.0f, SPRITE_Y);
    SoundManager::Init();
    StreakSystem::Init();
    DeathTransition::Reset();
}

void Game_Update()
{

    DebugConsole::Update();
    if (DebugConsole::IsOpen()) goto render_only; // freeze everything while console open
    // ------------------------------------------------------------------------
    // 1. INPUTS
    // ------------------------------------------------------------------------
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {

        if (isPaused)
        {
            // If a sub-menu is open, let the UI handle the ESC key! Do NOT unpause!
            if (UiPauseMenu::IsSettingsOpen() || UiPauseMenu::IsPopupOpen())
            {
                // Do nothing here. (Pause.cpp and Settings.cpp will close themselves)
            }
            else
            {
                // Only the main pause menu is open, safely unpause
                isPaused = false;
            }
        }
        else
        {
            // Pause the game normally
            isPaused = true;
        }

    }

    //if (AEInputCheckTriggered(AEVK_TAB)) {
    //    isShopOpen = !isShopOpen; // toggle shop menu and resume gameplay
    //}


    // ------------------------------------------------------------------------
    // 2. GAMEPLAY LOGIC (Run only when active)
    // ------------------------------------------------------------------------
    if (health.health_current <= 0.0f && !isDead)
    {
        isDead = true;
        DeathTransition::Start();
        SoundManager::PlayGameOverSfx();
    }

    if (!isPaused && !isDead && !UpgradeSystem::Upgrade_IsScreenOpen())
    {
        float healthBefore = health.health_current; // for pf streak
        frame_dt = (float)AEFrameRateControllerGetFrameTime();                                                                        // read delta time for this frame
        if (frame_dt > 0.1f)
        {
            // Force it to a safe standard frame time (~60 FPS)
            frame_dt = 0.01667f;
        }
        frame_dt *= PowerUpSystem::GetTimeScale(); // azfar code
        float dt = frame_dt;                                                                                                          // local alias used by the rest of the update block

        GameClouds::Update(dt);
        DayCycle::Update(dt);
        /*
        @overview   Boss update and enemy spawning
        @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        DifficultySystem::Difficulty_UpdateOverTime(dt);                                                                              // increase difficulty over time
        BossSystem::Boss_Update(enemy_pool, free_slot_queue, enemy_area, word_bank, dt, boss_spawning_blocked, is_random_spawning_enabled);                       // run boss logic for this frame
        GameEnemyBoss::Update(dt, enemy_pool, free_slot_queue, enemy_area, word_bank, health, boss_spawning_blocked, is_random_spawning_enabled, random_spawn_timer_seconds);
        if (boss_spawning_blocked && !s_dayTransitionFired)
        {
            DayCycle::TriggerTransition();
            s_dayTransitionFired = true;
        }
        /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

        PowerUpSystem::Update(dt, enemy_pool, enemy_area, health);
        Particles_Update(dt);
        Weather_Update(dt, enemy_pool);
        LaserBeam_Update(dt);
        StreakSystem::Update(dt);
        // mouse click -> convert to world coords
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;
            float wy = 450.0f - (float)my;
            PowerUpSystem::OnMouseClick(wx, wy, enemy_pool);
        } // azfar code

        // Gate typing while player is frozen (snow), stunned (thunder), or locked (ghost)
        if (!Weather_IsPlayerFrozen() && !Weather_IsPlayerStunned() && !Weather_IsTypingLocked())
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
        if (health.health_current < healthBefore)
            StreakSystem::OnStreakReset();
    }

    // Trigger upgrade screen the frame the boss death animation finishes.
    // Runs outside the gameplay gate so it fires even on the frame the gate closes.
    if (BossSystem::Boss_WasJustDefeated()) {
        s_dayTransitionFired = false; // allow transition to fire again next boss wave
        UpgradeSystem::Upgrade_TriggerScreen();
        DayCycle::TriggerDayTransition();

    }
    // Run upgrade input and card animation while the screen is open.
    // Runs outside the normal gameplay gate so the cards stay interactive.
    if (UpgradeSystem::Upgrade_IsScreenOpen())
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();
        if (dt > 0.1f) dt = 0.01667f;
        UpgradeSystem::Upgrade_Update(health);
    }


    // ------------------------------------------------------------------------
    // 3. RENDERING (Always run so the screen doesn't go black)
    // ------------------------------------------------------------------------

    /*_________________________________________________________________________________________________________________________________________*/
render_only:
    //DebugConsole::Render(); // draw console on top of everything
    //background
    //if (!DeathTransition::IsActive())
   // {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Reset Color

    // Only draw world if not in death transition

    AEGfxTexture* activeBg = (gShopBgID == 0) ? bgTexture1 : bgTexture2;
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    DayCycle::Draw();
    GameClouds::Draw();



    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    Weather_DrawBackground(); // Rainbow draws here, behind enemies

    //player (skip once dead - DeathTransition draws the death animation instead)
    if (!isDead)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        AEGfxTextureSet(playerTexture, uvOffsetX, uvOffsetY);
        Mesh_Renderer(playerMesh, SPRITE_X, SPRITE_Y, 150.0f, 150.0f, 0.0f);
        StreakSystem::Draw();
    }

    // Render typing letters 

    Typing_Render();

    /*
    @overview   Draw enemies and debug overlays
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    GameEnemyBoss::Draw(meshes, enemy_pool, is_enemy_area_box_visible, enemy_area);
    Particles_Draw();
    LaserBeam_Draw();

    UiSystem::Debug_DrawDifficultyTopRight(is_difficulty_debug_visible, is_random_spawning_enabled, health);                // draw debug difficulty ui with health
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    UiSystem::Ui_DrawScoreAndTyped(score); // draw score and typed buffer

    PowerUpSystem::Draw(); // azfar code

    Weather_Draw();

    /*
    @overview   Boss flash overlay and Health bar
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    UiSystem::Ui_DrawHealthBar(health, meshes, heart_anim, isPaused || DebugConsole::IsOpen() ? 0.0f : frame_dt);           // draw animated heart health bar
    BossSystem::Boss_DrawFlashOverlay(meshes);                                                                              // draw the white flash overlay on top of everything else
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    // Draw upgrade card screen on top of the flash overlay when active
    if (UpgradeSystem::Upgrade_IsScreenOpen())
        UpgradeSystem::Upgrade_Draw();

    // PAUSE OVERLAY
    UiPauseMenu::Check_Paused(isPaused, isDead, bgMesh);

    // --- Audio Pause Tracking ---
    if (isPaused && !wasPaused) {
        SoundManager::PauseGameplaySounds();
    }
    else if (!isPaused && wasPaused) {
        SoundManager::ResumeGameplaySounds();
    }
    wasPaused = isPaused;

    // Game Over & Leaderboard OVERLAY
    if (isDead)
    {
        float _dt = (float)AEFrameRateControllerGetFrameTime();
        DeathTransition::Update(_dt);
        DeathTransition::Draw(playerMesh, playerTexture, uvOffsetX, uvOffsetY); // deathPlayer not activePlayer
        if (DeathTransition::IsComplete())
            UiGameOver::Check_IfDead(isDead, isGameOver, openLeaderboard, bgMesh, temp_name, score);
    }
    DebugConsole::Render();

}

void Game_Exit()
{
    UiGameOver::Exit();

    /*
    @overview   Boss and enemy cleanup
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    GameEnemyBoss::Free(meshes, enemy_pool, free_slot_queue, boss_spawning_blocked);                                        // unload boss assets, free enemy pool and meshes, return the reserved pool slot
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    Typing_Exit();
    Leaderboard_System::Free();
    PowerUpSystem::Free();
    UpgradeSystem::Upgrade_Free();                                                                                          // unload upgrade card texture and meshes
    Particles_Free();
    Weather_Exit();
    LaserBeam_Free();
    DebugConsole::Exit();
    SoundManager::Exit();
    DeathTransition::Reset();
    GameClouds::Exit();
    DayCycle::Exit();
    StreakSystem::Exit();

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

    if (playerTexture)
    {
        AEGfxTextureUnload(playerTexture);
        playerTexture = nullptr;
    }

}