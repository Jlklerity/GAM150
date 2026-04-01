#include "pch.h"
#include "TimeGame.h"


namespace
{
    AEGfxTexture* player_texture = nullptr;
    AEGfxVertexList* player_mesh = nullptr;
    AEGfxTexture* bg_texture1 = nullptr;
    AEGfxTexture* bg_texture2 = nullptr;
    AEGfxVertexList* bg_mesh = nullptr;

    const float SPRITE_X = -700.0f;
    const float SPRITE_Y = -200.0f;
    const int   SPRITE_COLS = 8;
    const int   SPRITE_ROWS = 1;
    const float SPRITE_UV_WIDTH = 1.0f / SPRITE_COLS;
    const float SPRITE_UV_HEIGHT = 1.0f / SPRITE_ROWS;
    float       anim_timer = 0.0f;
    float       anim_duration = 0.1f;
    int         current_frame_index = 0;
    float       uv_offset_x = 0.0f;
    float       uv_offset_y = 0.0f;

    bool is_paused = false;
    bool was_paused = false;
    bool is_dead = false;
    bool is_timed_out = false;
    bool is_game_over = false;
    std::string temp_name;
    bool score_written = false;
    bool open_leaderboard = false;

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

    // draws the countdown timer in the centre top of the screen, colour shifts from green to red as time runs out,
    // triggers a red screen flash when the timer first hits 10 seconds, and makes the number blink in the last 10 seconds
    void DrawTimerEffects()
    {
        if (!bg_mesh || Graphics::g_font_id < 0) return;
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
            Mesh_Renderer(bg_mesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
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
        std::string time_str = std::to_string(secs);
        float cr = (pct < 0.5f) ? 1.0f : (1.0f - pct) * 2.0f;
        float cg = (pct > 0.5f) ? 1.0f : pct * 2.0f;
        float scale = (time_remaining <= 10.0f) ? 0.85f : 0.7f;
        float tw, th;
        AEGfxGetPrintSize(Graphics::g_font_id, time_str.c_str(), scale, &tw, &th);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::g_font_id, time_str.c_str(), -tw * 0.5f, 0.88f, scale, cr, cg, 0.1f, 1.0f);
    }

} // anonymous namespace


// initialises the timed game mode - resets all state, loads assets, sets up the enemy pool,
// typing system, leaderboard, powerups, particles, weather, debug console and sound,
// call this in the game state manager when entering the timed mode game state
void GameTimeMode_Init()
{
    anim_timer = 0.0f; anim_duration = 0.1f; current_frame_index = 0;
    uv_offset_x = 0.0f; uv_offset_y = 0.0f;
    is_paused = false; was_paused = false; is_dead = false; is_timed_out = false; is_game_over = false;
    temp_name = ""; score_written = false; open_leaderboard = false; lb_scroll = 0.0f;
    time_remaining = TOTAL_TIME; score.score_value = 0;
    random_spawn_timer_seconds = 9999.0f;
    flash_triggered = false; flash_timer = 0.0f; blink_timer = 0.0f; blink_visible = true;

    DifficultySystem::Difficulty_Reset();

    if (player_texture) { AEGfxTextureUnload(player_texture); player_texture = nullptr; }
    if (bg_texture1) { AEGfxTextureUnload(bg_texture1);    bg_texture1 = nullptr; }
    if (bg_texture2) { AEGfxTextureUnload(bg_texture2);    bg_texture2 = nullptr; }
    if (bg_mesh) { AEGfxMeshFree(bg_mesh);             bg_mesh = nullptr; }
    if (player_mesh) { AEGfxMeshFree(player_mesh);         player_mesh = nullptr; }

    GameClouds::Exit();
    DayCycle::Exit();
    LaserBeam_Free();
    StreakSystem::Exit();
    SoundManager::Exit();
    DeathTransition::Reset();

    const char* skinPath = Shop_GetActiveSkinPath();
    player_texture = AEGfxTextureLoad(skinPath);
    AE_ASSERT_MESG(player_texture, "Failed to load player skin!");
    player_mesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

    bg_texture1 = AEGfxTextureLoad("Assets/transbackground.png");
    bg_texture2 = AEGfxTextureLoad("Assets/background.png");
    AE_ASSERT_MESG(bg_texture1, "Failed to load background!");
    bg_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

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
    LeaderboardSystem::Init();
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


// runs the timed mode game loop each frame - counts down the timer, spawns enemies at an
// accelerated rate (TIMED_SPAWN_MULTIPLIER), handles pause/death/timeout states,
// writes the score to the leaderboard when time runs out, and renders everything,
// uses a goto to skip gameplay logic when the debug console is open or when paused/dead
void GameTimeMode_Update()
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    DebugConsole::Update();
    if (DebugConsole::IsOpen()) goto render_only;

    if (AEInputCheckTriggered(AEVK_ESCAPE) && !is_timed_out && !is_dead)
    {

        if (is_paused)
        {
            // If a sub-menu is open, let the UI handle the ESC key! Do NOT unpause!
            if (UiPauseMenu::IsSettingsOpen() || UiPauseMenu::IsPopupOpen())
            {
                // Do nothing here
            }
            else
            {
                is_paused = false;
            }
        }
        else
        {
            is_paused = true;
        }
    }

    if (!is_paused && !is_timed_out && !is_dead)
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();

        time_remaining -= dt;
        if (time_remaining <= 0.0f)
        {
            time_remaining = 0.0f;
            is_timed_out = true;
            if (!is_dead) { is_dead = true; DeathTransition::Start(); SoundManager::PlayGameOverSfx(); }
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

        float health_before = health.health_current;

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

        if (health.health_current < health_before)
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

        anim_timer += (float)AEFrameRateControllerGetFrameTime();
        if (anim_timer >= anim_duration)
        {
            anim_timer = 0.0f;
            current_frame_index = (current_frame_index + 1) % SPRITE_COLS;
            uv_offset_x = (current_frame_index % SPRITE_COLS) * SPRITE_UV_WIDTH;
            uv_offset_y = (current_frame_index / SPRITE_COLS) * SPRITE_UV_HEIGHT;
        }
    }

    if (is_timed_out && !score_written)
    {
        std::string name = MainMenuGetPlayerName();
        LeaderboardSystem::WriteTimeLeaderboard(name, score.score_value);
        score_written = true;
    }

render_only:

    // 1. Sky color base
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(0.53f, 0.81f, 0.92f, 1.0f);
    Mesh_Renderer(bg_mesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    // 2. DayCycle sky + sun (behind clouds)
    DayCycle::Draw();

    // 3. Clouds
    GameClouds::Draw();

    // 4. Ground texture on top
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTexture* active_bg = (g_shop_bg_id == 0) ? bg_texture1 : bg_texture2;
    AEGfxTextureSet(active_bg, 0.0f, 0.0f);
    Mesh_Renderer(bg_mesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    Weather_DrawBackground();

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTextureSet(player_texture, uv_offset_x, uv_offset_y);
    Mesh_Renderer(player_mesh, SPRITE_X, SPRITE_Y, 150.0f, 150.0f, 0.0f);
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

    UiPauseMenu::Check_Paused(is_paused, is_timed_out, bg_mesh);

    // --- Audio Pause Tracking ---
    if (is_paused && !was_paused) {
        SoundManager::PauseGameplaySounds();
    }
    else if (!is_paused && was_paused) {
        SoundManager::ResumeGameplaySounds();
    }
    was_paused = is_paused;

    if (is_dead)
    {
        float _dt = (float)AEFrameRateControllerGetFrameTime();
        DeathTransition::Update(_dt);
        DeathTransition::Draw(player_mesh, player_texture, uv_offset_x, uv_offset_y);
        if (DeathTransition::IsComplete())
        {
            if (!open_leaderboard)
                UiGameOver::DrawTimeoutScreen(score, bg_mesh, open_leaderboard);
            else
                LeaderboardSystem::DisplayTimeLeaderboard(open_leaderboard);
        }
    }

    DebugConsole::Render();
}

// frees all assets and shuts down all systems used by the timed mode,
// call this in the game state manager when leaving the timed mode game state
void GameTimeMode_Exit()
{
    EnemyManage::EnemyManage_Free(meshes, enemy_pool, free_slot_queue);
    Typing_Exit();
    GameClouds::Exit();
    DayCycle::Exit();
    LaserBeam_Free();
    StreakSystem::Exit();
    DeathTransition::Reset();
    LeaderboardSystem::Free();
    PowerUpSystem::Free();
    Particles_Free();
    Weather_Exit();
    DebugConsole::Exit();
    UiGameOver::Exit();
    SoundManager::Exit();

    if (player_mesh) { AEGfxMeshFree(player_mesh);          player_mesh = nullptr; }
    if (bg_mesh) { AEGfxMeshFree(bg_mesh);              bg_mesh = nullptr; }
    if (bg_texture1) { AEGfxTextureUnload(bg_texture1);     bg_texture1 = nullptr; }
    if (bg_texture2) { AEGfxTextureUnload(bg_texture2);     bg_texture2 = nullptr; }
    if (player_texture) { AEGfxTextureUnload(player_texture);  player_texture = nullptr; }
}