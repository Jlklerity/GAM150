#include "pch.h"
#include "Game.h"


namespace {
    //avatar mesh and texture
    AEGfxTexture* s_player_texture = nullptr;
    AEGfxVertexList* s_player_mesh = nullptr;

    //background mesh and texture
    AEGfxTexture* s_bg_texture_1 = nullptr; // Default BG
    AEGfxTexture* s_bg_texture_2 = nullptr; // Alternate BG
    AEGfxVertexList* s_bg_mesh = nullptr;

    const float SPRITE_X = -700.0f;
    const float SPRITE_Y = -200.0f;

    // Animation State
    float s_anim_timer = 0.0f;
    float s_anim_duration = 0.0f; // Speed of animation
    int   s_current_frame_index = 0;
    float s_uv_offset_x = 0.0f;
    float s_uv_offset_y = 0.0f;

    // Constants for the spritesheet
    const int   SPRITE_COLS = 8;
    const int   SPRITE_ROWS = 1;
    const float SPRITE_UV_WIDTH = 1.0f / SPRITE_COLS;
    const float SPRITE_UV_HEIGHT = 1.0f / SPRITE_ROWS;

    bool s_is_paused = false;
    bool s_was_paused = false;
    bool s_is_dead = false;
    bool s_open_leaderboard = false;

    static bool s_day_transition_fired = false;

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

    bool s_is_enemy_area_box_visible = false;                                                                              // toggle for debug box drawing
    bool s_is_random_spawning_enabled = true;                                                                               // toggle for auto spawning
    bool s_is_difficulty_debug_visible = false;                                                                             // toggle for difficulty text

    float s_random_spawn_timer_seconds = 9999.0f;                                                                          // start high to spawn immediately
    float s_frame_dt = 0.0f;                                                                                               // delta time for current frame, shared between update and render

    // Boss
    bool s_boss_spawning_blocked = false;                                                                                   // tracks whether the boss is blocking normal enemy spawning
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    // Typing state
    int         s_typed_target_index = -1;
    bool        s_is_game_over = false;
    std::string s_temp_name;
}

void Game_Init()
{
    s_anim_timer = 0.0f;
    s_anim_duration = 0.1f;
    s_current_frame_index = 0;
    s_uv_offset_x = 0.0f;
    s_uv_offset_y = 0.0f;
    score.score_value = 0;
    s_is_game_over = false;
    s_open_leaderboard = false;
    s_was_paused = false;
    s_is_paused = false;
    s_is_dead = false;
    s_frame_dt = 0.0f;
    DifficultySystem::Difficulty_Reset();

    // Initialise shared heart animation state (6 frames, single row sprite sheet)
    heart_anim = {};
    heart_anim.animation_columns = 6;
    heart_anim.animation_rows = 1;
    heart_anim.animation_frame_duration_seconds = 0.10f;

    if (s_bg_texture_1) { AEGfxTextureUnload(s_bg_texture_1); s_bg_texture_1 = nullptr; }
    if (s_bg_texture_2) { AEGfxTextureUnload(s_bg_texture_2); s_bg_texture_2 = nullptr; }
    if (s_bg_mesh) { AEGfxMeshFree(s_bg_mesh);           s_bg_mesh = nullptr; }

    GameClouds::Exit();
    DayCycle::Exit();
    LaserBeam_Free();
    StreakSystem::Exit();
    SoundManager::Exit();
    DeathTransition::Reset();

    s_day_transition_fired = false;

    if (s_player_texture) { AEGfxTextureUnload(s_player_texture); s_player_texture = nullptr; }


    //player texture
    const char* skinPath = Shop_GetActiveSkinPath();
    s_player_texture = AEGfxTextureLoad(skinPath);
    AE_ASSERT_MESG(s_player_texture, "Failed to load player skin!");

    if (s_player_mesh != nullptr) AEGfxMeshFree(s_player_mesh);
    s_player_mesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

    // Background
    s_bg_texture_1 = AEGfxTextureLoad("Assets/transbackground.png");
    AE_ASSERT_MESG(s_bg_texture_1, "Failed to load background!");
    s_bg_texture_2 = AEGfxTextureLoad("Assets/floorground2.png"); //for ltr when we have more bg skins
    AE_ASSERT_MESG(s_bg_texture_2, "Failed to load background!");

    /*
    @overview   Enemy and Boss
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    UpgradeSystem::Upgrade_Reset();                                                                                          // clear all upgrade bonuses
    GameEnemyBoss::Init(meshes, word_bank, health, enemy_animation, enemy_area, enemy_pool, free_slot_queue, s_boss_spawning_blocked);
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    s_bg_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    GameClouds::Init();
    DayCycle::Init();
    Typing_Init();
    LeaderboardSystem::Init();
    //Shop_Init();
    PowerUpSystem::Init(); // azfar code ver 1
    UpgradeSystem::Upgrade_Init();                                                                                          // load upgrade card assets and reset all bonuses for a fresh session

    DebugConsole::Init(
        s_is_enemy_area_box_visible,
        s_is_random_spawning_enabled,
        s_is_difficulty_debug_visible,
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

        if (s_is_paused)
        {
            // If a sub-menu is open, let the UI handle the ESC key! Do NOT unpause!
            if (UiPauseMenu::IsSettingsOpen() || UiPauseMenu::IsPopupOpen())
            {
                // Do nothing here. (Pause.cpp and Settings.cpp will close themselves)
            }
            else
            {
                // Only the main pause menu is open, safely unpause
                s_is_paused = false;
            }
        }
        else
        {
            // Pause the game normally
            s_is_paused = true;
        }

    }

    //if (AEInputCheckTriggered(AEVK_TAB)) {
    //    isShopOpen = !isShopOpen; // toggle shop menu and resume gameplay
    //}


    // ------------------------------------------------------------------------
    // 2. GAMEPLAY LOGIC (Run only when active)
    // ------------------------------------------------------------------------
    if (health.health_current <= 0.0f && !s_is_dead)
    {
        s_is_dead = true;
        DeathTransition::Start();
        SoundManager::PlayGameOverSfx();
    }

    if (!s_is_paused && !s_is_dead && !UpgradeSystem::Upgrade_IsScreenOpen())
    {
        float healthBefore = health.health_current; // for streak check
        s_frame_dt = (float)AEFrameRateControllerGetFrameTime();                                                                      // read delta time for this frame
        if (s_frame_dt > 0.1f)
        {
            // Force it to a safe standard frame time (~60 FPS)
            s_frame_dt = 0.01667f;
        }
        s_frame_dt *= PowerUpSystem::GetTimeScale(); // azfar code
        float dt = s_frame_dt;                                                                                                        // local alias used by the rest of the update block

        GameClouds::Update(dt);
        DayCycle::Update(dt);
        /*
        @overview   Boss update and enemy spawning
        @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        DifficultySystem::Difficulty_UpdateOverTime(dt);                                                                              // increase difficulty over time
        BossSystem::Boss_Update(enemy_pool, free_slot_queue, enemy_area, word_bank, dt, s_boss_spawning_blocked, s_is_random_spawning_enabled);                   // run boss logic for this frame
        GameEnemyBoss::Update(dt, enemy_pool, free_slot_queue, enemy_area, word_bank, health, s_boss_spawning_blocked, s_is_random_spawning_enabled, s_random_spawn_timer_seconds);
        if (s_boss_spawning_blocked && !s_day_transition_fired)
        {
            DayCycle::TriggerTransition();
            s_day_transition_fired = true;
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
        s_anim_timer += (float)AEFrameRateControllerGetFrameTime();

        if (s_anim_timer >= s_anim_duration)
        {
            s_anim_timer = 0.0f;

            // Move to next frame
            s_current_frame_index++;
            if (s_current_frame_index >= 8) // Max sprites
                s_current_frame_index = 0;

            // Calculate UV Offset (Which "window" of the image to look at)
            int col = s_current_frame_index % SPRITE_COLS;
            int row = s_current_frame_index / SPRITE_COLS;

            s_uv_offset_x = col * SPRITE_UV_WIDTH;
            s_uv_offset_y = row * SPRITE_UV_HEIGHT;
        }
        if (health.health_current < healthBefore)
            StreakSystem::OnStreakReset();
    }

    // Trigger upgrade screen the frame the boss death animation finishes.
    // Runs outside the gameplay gate so it fires even on the frame the gate closes.
    if (BossSystem::Boss_WasJustDefeated()) {
        s_day_transition_fired = false; // allow transition to fire again next boss wave
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

    AEGfxTexture* activeBg = (g_shop_bg_id == 0) ? s_bg_texture_1 : s_bg_texture_2;
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    DayCycle::Draw();
    GameClouds::Draw();



    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    Mesh_Renderer(s_bg_mesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    Weather_DrawBackground(); // Rainbow draws here, behind enemies

    //player (skip once dead - DeathTransition draws the death animation instead)
    if (!s_is_dead)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        AEGfxTextureSet(s_player_texture, s_uv_offset_x, s_uv_offset_y);
        Mesh_Renderer(s_player_mesh, SPRITE_X, SPRITE_Y, 150.0f, 150.0f, 0.0f);
        StreakSystem::Draw();
    }

    // Render typing letters 

    Typing_Render();

    /*
    @overview   Draw enemies and debug overlays
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    GameEnemyBoss::Draw(meshes, enemy_pool, s_is_enemy_area_box_visible, enemy_area);
    Particles_Draw();
    LaserBeam_Draw();

    UiSystem::Debug_DrawDifficultyTopRight(s_is_difficulty_debug_visible, s_is_random_spawning_enabled, health);              // draw debug difficulty ui with health
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    UiSystem::Ui_DrawScoreAndTyped(score); // draw score and typed buffer

    PowerUpSystem::Draw(); // azfar code

    Weather_Draw();

    /*
    @overview   Boss flash overlay and Health bar
    @author     Wong Zi Yuan (w.ziyuan@digipen.edu)
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    UiSystem::Ui_DrawHealthBar(health, meshes, heart_anim, s_is_paused || DebugConsole::IsOpen() ? 0.0f : s_frame_dt);        // draw animated heart health bar
    BossSystem::Boss_DrawFlashOverlay(meshes);                                                                                 // draw the white flash overlay on top of everything else
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    // Draw upgrade card screen on top of the flash overlay when active
    if (UpgradeSystem::Upgrade_IsScreenOpen())
        UpgradeSystem::Upgrade_Draw();

    // PAUSE OVERLAY
    UiPauseMenu::Check_Paused(s_is_paused, s_is_dead, s_bg_mesh);

    // --- Audio Pause Tracking ---
    if (s_is_paused && !s_was_paused) {
        SoundManager::PauseGameplaySounds();
    }
    else if (!s_is_paused && s_was_paused) {
        SoundManager::ResumeGameplaySounds();
    }
    s_was_paused = s_is_paused;

    // Game Over & Leaderboard OVERLAY
    if (s_is_dead)
    {
        float _dt = (float)AEFrameRateControllerGetFrameTime();
        DeathTransition::Update(_dt);
        DeathTransition::Draw(s_player_mesh, s_player_texture, s_uv_offset_x, s_uv_offset_y); // deathPlayer not activePlayer
        if (DeathTransition::IsComplete())
            UiGameOver::Check_IfDead(s_is_dead, s_is_game_over, s_open_leaderboard, s_bg_mesh, s_temp_name, score);
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
    GameEnemyBoss::Free(meshes, enemy_pool, free_slot_queue, s_boss_spawning_blocked);                                        // unload boss assets, free enemy pool and meshes, return the reserved pool slot
    /*_______________________________________________________________________________________________________________________________________________________________________________________________*/

    Typing_Exit();
    LeaderboardSystem::Free();
    PowerUpSystem::Free();
    UpgradeSystem::Upgrade_Free();                                                                                           // unload upgrade card texture and meshes
    Particles_Free();
    Weather_Exit();
    LaserBeam_Free();
    DebugConsole::Exit();
    SoundManager::Exit();
    DeathTransition::Reset();
    GameClouds::Exit();
    DayCycle::Exit();
    StreakSystem::Exit();

    if (s_player_mesh) {
        AEGfxMeshFree(s_player_mesh);
        s_player_mesh = nullptr;
    }

    if (s_bg_mesh) {
        AEGfxMeshFree(s_bg_mesh);
        s_bg_mesh = nullptr;
    }

    if (s_bg_texture_1)
    {
        AEGfxTextureUnload(s_bg_texture_1);
        s_bg_texture_1 = nullptr;
    }
    if (s_bg_texture_2)
    {
        AEGfxTextureUnload(s_bg_texture_2);
        s_bg_texture_2 = nullptr;
    }

    if (s_player_texture)
    {
        AEGfxTextureUnload(s_player_texture);
        s_player_texture = nullptr;
    }

}