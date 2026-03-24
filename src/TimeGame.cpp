#include "pch.h"
#include "TimeGame.h"


namespace
{
    AEGfxTexture* playerTexture = nullptr;
    AEGfxVertexList* playerMesh = nullptr;
    AEGfxTexture* bgTexture1 = nullptr;
    AEGfxTexture* bgTexture2 = nullptr;
    AEGfxVertexList* bgMesh = nullptr;

    const float SPRITE_X = -700.0f;
    const float SPRITE_Y = -200.0f;
    const int   SPRITE_COLS = 8;
    const int   SPRITE_ROWS = 1;
    const float SPRITE_UV_WIDTH = 1.0f / SPRITE_COLS;
    const float SPRITE_UV_HEIGHT = 1.0f / SPRITE_ROWS;
    float       animTimer = 0.0f;
    float       animDuration = 0.1f;
    int         currentFrameIndex = 0;
    float       uvOffsetX = 0.0f;
    float       uvOffsetY = 0.0f;

    bool isPaused = false;
    bool wasPaused = false;
    bool isDead = false;
    bool isTimedOut = false;
    bool isGameOver = false;
    std::string temp_name;
    bool scoreWritten = false;
    bool openLeaderboard = false;

    Data::WordBank              word_bank{};
    Data::Health                health{};
    Data::Score                 score{};
    Data::SpriteAnim            enemy_animation{};
    Data::EnemyArea             enemy_area{};
    Data::Meshes                meshes{};
    std::vector<Data::Enemy>    enemy_pool;
    std::deque<int>             free_slot_queue;

    bool  is_enemy_area_box_visible = false;
    bool  is_random_spawning_enabled = true;
    bool  is_difficulty_debug_visible = false;
    float random_spawn_timer_seconds = 9999.0f;

    static const float TOTAL_TIME = 60.0f;
    float time_remaining = TOTAL_TIME;
    static const float TIMED_SPAWN_MULTIPLIER = 0.55f;

    float lb_scroll = 0.0f;
    static const int LB_VISIBLE = 5;

    float flash_timer = 0.0f;
    bool  flash_triggered = false;
    float blink_timer = 0.0f;
    bool  blink_visible = true;
    static const float FLASH_DURATION = 0.35f;
    static const float BLINK_INTERVAL = 0.35f;

    void DrawTimerEffects()
    {
        if (!bgMesh || Graphics::font_id < 0) return;
        float dt = (float)AEFrameRateControllerGetFrameTime();
        float pct = time_remaining / TOTAL_TIME;

        if (!flash_triggered && time_remaining <= 10.0f)
        {
            flash_triggered = true;
            flash_timer = FLASH_DURATION;
        }
        if (flash_timer > 0.0f)
        {
            flash_timer -= dt;
            float alpha = (flash_timer / FLASH_DURATION) * 0.55f;
            if (alpha < 0.0f) alpha = 0.0f;
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(alpha);
            AEGfxSetColorToMultiply(1.0f, 0.1f, 0.1f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
        }

        if (time_remaining <= 10.0f)
        {
            blink_timer += dt;
            if (blink_timer >= BLINK_INTERVAL) { blink_timer = 0.0f; blink_visible = !blink_visible; }
        }
        else { blink_visible = true; blink_timer = 0.0f; }

        if (!blink_visible) return;
        AEGfxSetTransparency(1.0f);
        int secs = (int)time_remaining + 1;
        if (secs > (int)TOTAL_TIME) secs = (int)TOTAL_TIME;
        std::string timeStr = std::to_string(secs);
        float cr = (pct < 0.5f) ? 1.0f : (1.0f - pct) * 2.0f;
        float cg = (pct > 0.5f) ? 1.0f : pct * 2.0f;
        float scale = (time_remaining <= 10.0f) ? 0.85f : 0.7f;
        float tw, th;
        AEGfxGetPrintSize(Graphics::font_id, timeStr.c_str(), scale, &tw, &th);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::font_id, timeStr.c_str(), -tw * 0.5f, 0.88f, scale, cr, cg, 0.1f, 1.0f);
    }

} // anonymous namespace


void GameTimeMode_Init()
{
    animTimer = 0.0f; animDuration = 0.1f; currentFrameIndex = 0;
    uvOffsetX = 0.0f; uvOffsetY = 0.0f;
    isPaused = false; wasPaused = false; isDead = false; isTimedOut = false; isGameOver = false;
    temp_name = ""; scoreWritten = false; openLeaderboard = false; lb_scroll = 0.0f;
    time_remaining = TOTAL_TIME; score.score_value = 0;
    random_spawn_timer_seconds = 9999.0f;
    flash_triggered = false; flash_timer = 0.0f; blink_timer = 0.0f; blink_visible = true;

    DifficultySystem::Difficulty_Reset();

    if (playerTexture) { AEGfxTextureUnload(playerTexture); playerTexture = nullptr; }
    if (bgTexture1) { AEGfxTextureUnload(bgTexture1);    bgTexture1 = nullptr; }
    if (bgTexture2) { AEGfxTextureUnload(bgTexture2);    bgTexture2 = nullptr; }
    if (bgMesh) { AEGfxMeshFree(bgMesh);             bgMesh = nullptr; }
    if (playerMesh) { AEGfxMeshFree(playerMesh);         playerMesh = nullptr; }

    GameClouds::Exit();
    DayCycle::Exit();
    LaserBeam_Free();
    StreakSystem::Exit();
    SoundManager::Exit();
    DeathTransition::Reset();

    const char* skinPath = Shop_GetActiveSkinPath();
    playerTexture = AEGfxTextureLoad(skinPath);
    AE_ASSERT_MESG(playerTexture, "Failed to load player skin!");
    playerMesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

    bgTexture1 = AEGfxTextureLoad("Assets/transbackground.png");
    bgTexture2 = AEGfxTextureLoad("Assets/background.png");
    AE_ASSERT_MESG(bgTexture1, "Failed to load background!");
    bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

    GameClouds::Init();
    DayCycle::Init();

    EnemyManage::EnemyManage_InitEngine(word_bank);
    HealthSystem::Health_Init(health);
    meshes = EnemyManage::EnemyManage_LoadMeshes(enemy_animation);
    enemy_area.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f);

    enemy_pool.resize((size_t)EnemyConfig::MAX_ENEMIES);
    free_slot_queue.clear();
    for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)
    {
        enemy_pool[(size_t)i].enemy_is_active = false;
        free_slot_queue.push_back(i);
    }

    Typing_Init();
    Leaderboard_System::Init();
    PowerUpSystem::Init();
    DebugConsole::Init(is_enemy_area_box_visible, is_random_spawning_enabled,
        is_difficulty_debug_visible, health, enemy_pool, free_slot_queue, enemy_area, word_bank);
    Particles_Init();
    Weather_Init();
    LaserBeam_Init();
    LaserBeam_SetOrigin(SPRITE_X + 75.0f, SPRITE_Y);
    StreakSystem::Init();
    DeathTransition::Reset();
    SoundManager::Init();
}


void GameTimeMode_Update()
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    DebugConsole::Update();
    if (DebugConsole::IsOpen()) goto render_only;

    if (AEInputCheckTriggered(AEVK_ESCAPE) && !isTimedOut && !isDead)
    {

        if (isPaused)
        {
            // If a sub-menu is open, let the UI handle the ESC key! Do NOT unpause!
            if (UiPauseMenu::IsSettingsOpen() || UiPauseMenu::IsPopupOpen())
            {
                // Do nothing here
            }
            else
            {
                isPaused = false;
            }
        }
        else
        {
            isPaused = true;
        }
    }

    if (!isPaused && !isTimedOut && !isDead)
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();

        time_remaining -= dt;
        if (time_remaining <= 0.0f)
        {
            time_remaining = 0.0f;
            isTimedOut = true;
            if (!isDead) { isDead = true; DeathTransition::Start(); SoundManager::PlayGameOverSfx(); }
        }

        if (dt > 0.1f) dt = 0.01667f;
        dt *= PowerUpSystem::GetTimeScale();

        GameClouds::Update(dt);
        DayCycle::Update(dt);
        DifficultySystem::Difficulty_UpdateOverTime(dt);

        if (is_random_spawning_enabled)
        {
            random_spawn_timer_seconds += dt;
            float interval = DifficultySystem::Difficulty_GetSpawnIntervalSeconds() * TIMED_SPAWN_MULTIPLIER;
            if (random_spawn_timer_seconds >= interval)
            {
                random_spawn_timer_seconds = 0.0f;
                int roll = Utility::Utility_RandInt(0, 99);
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();
                if (roll < 15)
                    EnemySystem::Enemy_SpawnFriendlyFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.15f);
                else if (roll < 30)
                    EnemySystem::Enemy_SpawnShieldedFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.05f);
                else
                {
                    int len = DifficultySystem::Difficulty_GetRandomWordLength();
                    EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, len);
                }
            }
        }

        float healthBefore = health.health_current;

        for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)
        {
            Data::Enemy& e = enemy_pool[(size_t)i];
            bool was_active = e.enemy_is_active;
            bool was_damage_done = e.enemy_did_damage;
            bool was_dying = e.enemy_is_dying;

            EnemySystem::Enemy_Update(e, enemy_area, dt, health);

            if (was_active && !e.enemy_is_active)
            {
                free_slot_queue.push_back(i);

                if (!was_damage_done && !was_dying) // fresh typed kill — not yet in dying state
                {
                    Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                   // LaserBeam_Fire(e.enemy_position_x, e.enemy_position_y);
                }
                else // landed OR finished dying fall
                    Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
            }
        }

        if (health.health_current < healthBefore)
            StreakSystem::OnStreakReset();

        EnemySystem::Enemy_ResolveLaneStacking(enemy_pool, enemy_area);

        PowerUpSystem::Update(dt, enemy_pool, enemy_area, health);
        Particles_Update(dt);
        Weather_Update(dt, enemy_pool);
        LaserBeam_Update(dt);
        StreakSystem::Update(dt);

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;
            float wy = 450.0f - (float)my;
            PowerUpSystem::OnMouseClick(wx, wy, enemy_pool);
        }

        if (!Weather_IsPlayerFrozen() && !Weather_IsPlayerStunned() && !Weather_IsTypingLocked())
            Typing_UpdateAndApplyToEnemies(enemy_pool, score, health);

        animTimer += (float)AEFrameRateControllerGetFrameTime();
        if (animTimer >= animDuration)
        {
            animTimer = 0.0f;
            currentFrameIndex = (currentFrameIndex + 1) % SPRITE_COLS;
            uvOffsetX = (currentFrameIndex % SPRITE_COLS) * SPRITE_UV_WIDTH;
            uvOffsetY = (currentFrameIndex / SPRITE_COLS) * SPRITE_UV_HEIGHT;
        }
    }

    if (isTimedOut && !scoreWritten)
    {
        std::string name = Main_Menu_GetPlayerName();
        Leaderboard_System::Write_TimeLeaderboard(name, score.score_value);
        scoreWritten = true;
    }

render_only:

    // 1. Sky color base
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.53f, 0.81f, 0.92f, 1.0f);
    Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    // 2. DayCycle sky + sun (behind clouds)
    DayCycle::Draw();

    // 3. Clouds
    GameClouds::Draw();

    // 4. Ground texture on top
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTexture* activeBg = (gShopBgID == 0) ? bgTexture1 : bgTexture2;
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    Weather_DrawBackground();

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(playerTexture, uvOffsetX, uvOffsetY);
    Mesh_Renderer(playerMesh, SPRITE_X, SPRITE_Y, 150.0f, 150.0f, 0.0f);
    StreakSystem::Draw();

    Typing_Render();

    if (is_enemy_area_box_visible)
    {
        UiSystem::Debug_DrawEnemyAreaBox(meshes, enemy_area);
        UiSystem::Debug_DrawEnemyLaneLines(meshes, enemy_area);
    }

    for (const auto& e : enemy_pool)
        EnemySystem::Enemy_Draw(meshes, e);
    Typing_DrawEnemyWords(enemy_pool);

    Particles_Draw();
    LaserBeam_Draw();
    Weather_Draw();

    UiSystem::Debug_DrawDifficultyTopRight(is_difficulty_debug_visible, is_random_spawning_enabled, health);
    UiSystem::Ui_DrawScoreAndTyped(score);
    PowerUpSystem::Draw();
    DrawTimerEffects();

    UiPauseMenu::Check_Paused(isPaused, isTimedOut, bgMesh);

    // --- Audio Pause Tracking ---
    if (isPaused && !wasPaused) {
        SoundManager::PauseGameplaySounds();
    }
    else if (!isPaused && wasPaused) {
        SoundManager::ResumeGameplaySounds();
    }
    wasPaused = isPaused;

    if (isDead)
    {
        float _dt = (float)AEFrameRateControllerGetFrameTime();
        DeathTransition::Update(_dt);
        DeathTransition::Draw(playerMesh, playerTexture, uvOffsetX, uvOffsetY);
        if (DeathTransition::IsComplete())
        {
            if (!openLeaderboard)
                UiGameOver::DrawTimeoutScreen(score, bgMesh, openLeaderboard);
            else
                Leaderboard_System::Display_TimeLeaderboard(openLeaderboard);
        }
    }

    DebugConsole::Render();
}

void GameTimeMode_Exit()
{
    EnemyManage::EnemyManage_Free(meshes, enemy_pool, free_slot_queue);
    Typing_Exit();
    GameClouds::Exit();
    DayCycle::Exit();
    LaserBeam_Free();
    StreakSystem::Exit();
    DeathTransition::Reset();
    Leaderboard_System::Free();
    PowerUpSystem::Free();
    Particles_Free();
    Weather_Exit();
    DebugConsole::Exit();
    UiGameOver::Exit();
    SoundManager::Exit();

    if (playerMesh) { AEGfxMeshFree(playerMesh);          playerMesh = nullptr; }
    if (bgMesh) { AEGfxMeshFree(bgMesh);              bgMesh = nullptr; }
    if (bgTexture1) { AEGfxTextureUnload(bgTexture1);     bgTexture1 = nullptr; }
    if (bgTexture2) { AEGfxTextureUnload(bgTexture2);     bgTexture2 = nullptr; }
    if (playerTexture) { AEGfxTextureUnload(playerTexture);  playerTexture = nullptr; }
}