#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "TimeGame.h"
#include <crtdbg.h>
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
#include "LaserBeam.h"
#include "DeathTransition.h"
#include "GameClouds.h"
#include "StreakSystem.h"

#include <string>
#include <vector>
#include <deque>
#include <fstream>
#include <algorithm>

namespace
{
    // ---------------------------------------------------------------------------
    // All game state for timed mode (mirrors game.cpp anonymous namespace)
    // ---------------------------------------------------------------------------

    // ---- Player visuals ----
    AEGfxTexture* playerTexture1 = nullptr;  // Default Skin
    AEGfxTexture* playerTexture2 = nullptr;  // Alternate Skin
    AEGfxVertexList* playerMesh = nullptr;

    // ---- Background visuals ----
    AEGfxTexture* bgTexture1 = nullptr;  // Default BG
    AEGfxTexture* bgTexture2 = nullptr;  // Alternate BG
    AEGfxVertexList* bgMesh = nullptr;

    // ---- Player sprite position ----
    const float SPRITE_X = -700.0f;
    const float SPRITE_Y = -200.0f;

    // ---- Sprite sheet animation ----
    const int   SPRITE_COLS = 8;
    const int   SPRITE_ROWS = 1;
    const float SPRITE_UV_WIDTH = 1.0f / SPRITE_COLS;
    const float SPRITE_UV_HEIGHT = 1.0f / SPRITE_ROWS;
    float       animTimer = 0.0f;
    float       animDuration = 0.1f;     // seconds per frame
    int         currentFrameIndex = 0;
    float       uvOffsetX = 0.0f;
    float       uvOffsetY = 0.0f;

    // ---- Game flow flags ----
    bool isPaused = false;
    bool isDead = false;            // true once health hits 0
    bool isTimedOut = false;            // true once countdown hits zero
    bool scoreWritten = false;            // guard so we only write the score once
    bool openLeaderboard = false;            // true when the timed leaderboard overlay is showing
    bool isGameOver = false;            // used by UiGameOver
    std::string temp_name;                      // player name buffer for game-over screen

    // ---- Enemy / game systems ----
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

    float random_spawn_timer_seconds = 9999.0f;  // start high so first enemy spawns immediately

    // ---- Countdown timer ----
    static const float TOTAL_TIME = 60.0f;         // seconds for timed mode
    float time_remaining = TOTAL_TIME;

    // ---- Timed-mode spawn interval multiplier ----
    // Faster than normal mode to keep pressure up during the short window
    static const float TIMED_SPAWN_MULTIPLIER = 0.55f;

    // ---- Leaderboard overlay scroll ----
    float lb_scroll = 0.0f;
    static const int LB_VISIBLE = 5;

    // ---- Timer display state ----
    float flash_timer = 0.0f;     // counts down after the 10s screen flash triggers
    bool  flash_triggered = false;    // guard so the 10s flash only fires once
    float blink_timer = 0.0f;     // drives the sub-10s number blink
    bool  blink_visible = true;     // current blink on/off state
    static const float FLASH_DURATION = 0.35f; // how long the screen flash lasts
    static const float BLINK_INTERVAL = 0.35f; // how fast the number blinks under 10s

    // ---------------------------------------------------------------------------
    // DrawTimerEffects
    //   1. Top-center countdown number, green -> yellow -> red as time drops
    //   2. One-shot screen flash when time hits 10s ("last chance" moment)
    //   3. Number blinks on/off rapidly when under 10s
    // ---------------------------------------------------------------------------
    void DrawTimerEffects()
    {
        if (!bgMesh || Graphics::font_id < 0) return;

        float dt = (float)AEFrameRateControllerGetFrameTime();
        float pct = time_remaining / TOTAL_TIME;    // 1.0 = full, 0.0 = empty

        // ================================================================
        // 1. ONE-SHOT SCREEN FLASH AT 10s
        // ================================================================
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

        // ================================================================
        // 2. BLINK LOGIC (sub-10s only)
        // ================================================================
        if (time_remaining <= 10.0f)
        {
            blink_timer += dt;
            if (blink_timer >= BLINK_INTERVAL)
            {
                blink_timer = 0.0f;
                blink_visible = !blink_visible;
            }
        }
        else
        {
            blink_visible = true;
            blink_timer = 0.0f;
        }

        // ================================================================
        // 3. COUNTDOWN NUMBER — top-center, green -> yellow -> red
        // ================================================================
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
        AEGfxPrint(Graphics::font_id, timeStr.c_str(),
            -tw * 0.5f,
            0.88f,
            scale, cr, cg, 0.1f, 1.0f);
    }

} // anonymous namespace


// ---------------------------------------------------------------------------
// Public functions called by GameStateManager
// ---------------------------------------------------------------------------

void GameTimeMode_Init()
{
    // ---- Reset all state so restarts start clean ----
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
    isTimedOut = false;
    scoreWritten = false;
    lb_scroll = 0.0f;
    time_remaining = TOTAL_TIME;
    random_spawn_timer_seconds = 9999.0f;
    flash_triggered = false;
    flash_timer = 0.0f;
    blink_timer = 0.0f;
    blink_visible = true;

    DifficultySystem::Difficulty_Reset();



// Free bg resources if reinitialising
    if (bgTexture1) { AEGfxTextureUnload(bgTexture1); bgTexture1 = nullptr; }
    if (bgTexture2) { AEGfxTextureUnload(bgTexture2); bgTexture2 = nullptr; }
    if (bgMesh) { AEGfxMeshFree(bgMesh);          bgMesh = nullptr; }

    // Free player textures if reinitialising
    if (playerTexture1) { AEGfxTextureUnload(playerTexture1); playerTexture1 = nullptr; }
    if (playerTexture2) { AEGfxTextureUnload(playerTexture2); playerTexture2 = nullptr; }

    // Shut down subsystems cleanly before reinitialising them
    GameClouds::Exit();
    LaserBeam_Free();
    StreakSystem::Exit();
    DeathTransition::Reset();

    // ---- Player textures and sprite mesh ----
    playerTexture1 = AEGfxTextureLoad("Assets/characterSheet.png");
    AE_ASSERT_MESG(playerTexture1, "Failed to load characterSheet!");
    playerTexture2 = AEGfxTextureLoad("Assets/character2Sheet.png");
    AE_ASSERT_MESG(playerTexture1, "Failed to load character2Sheet!");

    if (playerMesh != nullptr) AEGfxMeshFree(playerMesh);
    playerMesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

    // ---- Background textures and quad mesh ----
    bgTexture1 = AEGfxTextureLoad("Assets/background.png");
    bgTexture2 = AEGfxTextureLoad("Assets/background.png");        // placeholder for alt bg slot
    AE_ASSERT_MESG(bgTexture1, "Failed to load background!");

    // ---- Enemy systems ----
    EnemyManage::EnemyManage_InitEngine(word_bank);                 // load word bank from file
    HealthSystem::Health_Init(health);                              // start at full health
    meshes = EnemyManage::EnemyManage_LoadMeshes(enemy_animation);  // load enemy sprite sheet and meshes
    enemy_area.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f);

    enemy_pool.resize((size_t)EnemyConfig::MAX_ENEMIES);
    free_slot_queue.clear();
    for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)
    {
        enemy_pool[(size_t)i].enemy_is_active = false;
        free_slot_queue.push_back(i);
    }

    bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    GameClouds::Init();
    Typing_Init();
    Leaderboard_System::Init();
    PowerUpSystem::Init();

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
    StreakSystem::Init();
    DeathTransition::Reset();
}


void GameTimeMode_Update()
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // ---- Debug console has first priority; freezes all gameplay while open ----
    DebugConsole::Update();
    if (DebugConsole::IsOpen()) goto render_only;

    // ---- Pause toggle (blocked while timed out or dead so buttons stay clickable) ----
    if (AEInputCheckTriggered(AEVK_ESCAPE) && !isTimedOut && !isDead)
        isPaused = !isPaused;

    // ------------------------------------------------------------------------
    // GAMEPLAY LOGIC
    // ------------------------------------------------------------------------

    // ---- Trigger death transition when health hits 0 ----
    if (health.health_current == 0 && !isDead)
    {
        isDead = true;
        DeathTransition::Start();
    }

    if (!isPaused && !isDead && !isTimedOut)
    {
        float healthBefore = health.health_current;     // for streak reset check

        float dt = (float)AEFrameRateControllerGetFrameTime();
        if (dt > 0.1f)
            dt = 0.01667f;                              // clamp to safe frame time (~60 FPS)

        // ---- Countdown subtracts BEFORE time-scale so slow power-up doesn't affect it ----
        time_remaining -= dt;
        if (time_remaining <= 0.0f)
        {
            time_remaining = 0.0f;
            isTimedOut = true;
        }

        dt *= PowerUpSystem::GetTimeScale();            // scale dt AFTER countdown

        GameClouds::Update(dt);
        DifficultySystem::Difficulty_UpdateOverTime(dt);

        // ---- Enemy spawning (faster than normal via TIMED_SPAWN_MULTIPLIER) ----
        if (is_random_spawning_enabled)
        {
            random_spawn_timer_seconds += dt;
            float interval = DifficultySystem::Difficulty_GetSpawnIntervalSeconds() * TIMED_SPAWN_MULTIPLIER;

            if (random_spawn_timer_seconds >= interval)
            {
                random_spawn_timer_seconds = 0.0f;
                int roll = Utility::Utility_RandInt(0, 99);
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();

                if (roll < 15)                                              // 15% chance: friendly enemy
                    EnemySystem::Enemy_SpawnFriendlyFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.15f);
                else if (roll < 30)                                         // 15% chance: shielded enemy
                    EnemySystem::Enemy_SpawnShieldedFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3, max_len, 1.05f);
                else                                                        // 70% chance: normal enemy
                {
                    int len = DifficultySystem::Difficulty_GetRandomWordLength();
                    EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, len);
                }
            }
        }

        // ---- Enemy update loop ----
        for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)
        {
            Data::Enemy& e = enemy_pool[(size_t)i];
            bool was_active = e.enemy_is_active;
            bool was_damage_done = e.enemy_did_damage;

            EnemySystem::Enemy_Update(e, enemy_area, dt, health);

            if (was_active && !e.enemy_is_active)                           // enemy just became inactive this frame
            {
                free_slot_queue.push_back(i);
                if (!was_damage_done)
                    Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                else
                    Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
            }
        }

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

        // ---- Typing (gated by weather effects) ----
        if (!Weather_IsPlayerFrozen() && !Weather_IsPlayerStunned())
            Typing_UpdateAndApplyToEnemies(enemy_pool, score, health);

        // ---- Sprite sheet animation ----
        animTimer += (float)AEFrameRateControllerGetFrameTime();
        if (animTimer >= animDuration)
        {
            animTimer = 0.0f;

            currentFrameIndex++;
            if (currentFrameIndex >= 8)
                currentFrameIndex = 0;

            int col = currentFrameIndex % SPRITE_COLS;
            int row = currentFrameIndex / SPRITE_COLS;
            uvOffsetX = col * SPRITE_UV_WIDTH;
            uvOffsetY = row * SPRITE_UV_HEIGHT;
        }

        if (health.health_current < healthBefore)
            StreakSystem::OnStreakReset();
    }

    // ---- Write score once the moment time runs out (timed leaderboard) ----
    if (isTimedOut && !scoreWritten)
    {
        std::string name = Main_Menu_GetPlayerName();
        Leaderboard_System::Write_TimeLeaderboard(name, score.score_value);
        scoreWritten = true;
    }

    // ------------------------------------------------------------------------
    // RENDERING (always runs, even when paused, timed out, or dead)
    // ------------------------------------------------------------------------
render_only:

    // ---- Background ----
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTexture* activeBg = (gShopBgID == 0) ? bgTexture1 : bgTexture2;
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    GameClouds::Draw();

    // ---- Player sprite ----
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTexture* activePlayer = (gShopSkinID == 0) ? playerTexture1 : playerTexture2;
    AEGfxTextureSet(activePlayer, uvOffsetX, uvOffsetY);
    Mesh_Renderer(playerMesh, SPRITE_X, SPRITE_Y, 150.0f, 150.0f, 0.0f);

    StreakSystem::Draw();
    Typing_Render();

    // ---- Enemies ----
    for (const auto& e : enemy_pool)
        EnemySystem::Enemy_Draw(meshes, e);
    Typing_DrawEnemyWords(enemy_pool);

    // ---- FX ----
    Particles_Draw();
    LaserBeam_Draw();

    // ---- HUD ----
    UiSystem::Debug_DrawDifficultyTopRight(is_difficulty_debug_visible, is_random_spawning_enabled, health);
    UiSystem::Ui_DrawScoreAndTyped(score);
    PowerUpSystem::Draw();
    Weather_Draw();
    UiSystem::Ui_DrawHealthBar(health, meshes);

    DrawTimerEffects();                                 // countdown number + 10s flash + sub-10s blink

    // ---- Pause overlay ----
    UiPauseMenu::Check_Paused(isPaused, isDead, bgMesh);

    // ---- Death transition + Game Over overlay (same as normal mode, routes to timed leaderboard) ----
    if (isDead)
    {
        float _dt = (float)AEFrameRateControllerGetFrameTime();
        AEGfxTexture* deathPlayer = (gShopSkinID == 0) ? playerTexture1 : playerTexture2;
        DeathTransition::Update(_dt);
        DeathTransition::Draw(playerMesh, deathPlayer, uvOffsetX, uvOffsetY);
        if (DeathTransition::IsComplete())
        {
            UiGameOver::Check_IfDead(isDead, isGameOver, openLeaderboard, bgMesh, temp_name, score);
            if (openLeaderboard)
                Leaderboard_System::Display_TimeLeaderboard(openLeaderboard);
        }
    }

    // ---- End-of-round overlay (timer ran out) ----
    if (isTimedOut)
    {
        if (!openLeaderboard)
            UiGameOver::DrawTimeoutScreen(score, bgMesh, openLeaderboard);
        else
            Leaderboard_System::Display_TimeLeaderboard(openLeaderboard);
    }

    DebugConsole::Render();                             // console always renders on top of everything
}


void GameTimeMode_Exit()
{
    EnemyManage::EnemyManage_Free(meshes, enemy_pool, free_slot_queue);

    Typing_Exit();
    Leaderboard_System::Free();
    PowerUpSystem::Free();
    Particles_Free();
    Weather_Exit();
    LaserBeam_Free();
    DebugConsole::Exit();
    DeathTransition::Reset();
    GameClouds::Exit();
    StreakSystem::Exit();

    if (playerMesh) { AEGfxMeshFree(playerMesh);           playerMesh = nullptr; }
    if (bgMesh) { AEGfxMeshFree(bgMesh);               bgMesh = nullptr; }

    if (bgTexture1) { AEGfxTextureUnload(bgTexture1);       bgTexture1 = nullptr; }
    if (bgTexture2) { AEGfxTextureUnload(bgTexture2);       bgTexture2 = nullptr; }
    if (playerTexture1) { AEGfxTextureUnload(playerTexture1);   playerTexture1 = nullptr; }
    if (playerTexture2) { AEGfxTextureUnload(playerTexture2);   playerTexture2 = nullptr; }
}