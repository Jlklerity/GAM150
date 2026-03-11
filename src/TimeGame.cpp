#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "TimeGame.h"
#include "GameStateManager.h"
#include "AEEngine.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Typing.h"
#include "Enemy.h"
#include "PowerUp.h"
#include "MainMenu.h"
#include "DebugConsole.h"
#include "Particles.h"
#include "GameOver.h"
#include "Pause.h"
#include "Weather.h"
#include "Leaderboard.h"
#include "Shop.h"

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
    AEGfxTexture* playerTexture1 = nullptr;
    AEGfxTexture* playerTexture2 = nullptr;
    AEGfxVertexList* playerMesh = nullptr;

    // ---- Background visuals ----
    AEGfxTexture* bgTexture1 = nullptr;
    AEGfxTexture* bgTexture2 = nullptr;
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
    float       animDuration = 0.1f;                                  // seconds per frame
    int         currentFrameIndex = 0;
    float       uvOffsetX = 0.0f;
    float       uvOffsetY = 0.0f;

    // ---- Game flow flags ----
    bool isPaused = false;
    bool isTimedOut = false;                                          // true once countdown hits zero
    bool scoreWritten = false;                                          // guard so we only write the score once
    bool openLeaderboard = false;                                          // true when the timed leaderboard overlay is showing

    // ---- Enemy / game systems (same pattern as game.cpp) ----
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

    float random_spawn_timer_seconds = 9999.0f;                            // start high so first enemy spawns immediately

    // ---- Countdown timer ----
    static const float TOTAL_TIME = 60.0f;                                 // seconds for timed mode
    float time_remaining = TOTAL_TIME;

    // ---- Timed-mode spawn interval multiplier ----
    // Faster than normal mode to keep pressure up during the short window
    static const float TIMED_SPAWN_MULTIPLIER = 0.55f;

    // ---- Leaderboard overlay scroll ----
    float lb_scroll = 0.0f;
    static const int LB_VISIBLE = 5;

    // ---- Timer display state ----
    float flash_timer = 0.0f;                                         // counts down after the 10s screen flash triggers
    bool  flash_triggered = false;                                        // guard so the 10s flash only fires once
    float blink_timer = 0.0f;                                         // drives the sub-10s number blink
    bool  blink_visible = true;                                         // current blink on/off state
    static const float FLASH_DURATION = 0.35f;                              // how long the screen flash lasts
    static const float BLINK_INTERVAL = 0.35f;                              // how fast the number blinks under 10s

   
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
        float pct = time_remaining / TOTAL_TIME;                            // 1.0 = full, 0.0 = empty

        // ================================================================
        // 1. ONE-SHOT SCREEN FLASH AT 10s
        // When time crosses 10s for the first time, fire a brief red flash
        // over the whole screen to signal "last chance"
        // ================================================================
        if (!flash_triggered && time_remaining <= 10.0f)
        {
            flash_triggered = true;                                         // only fire once per round
            flash_timer = FLASH_DURATION;                               // start the flash countdown
        }

        if (flash_timer > 0.0f)
        {
            flash_timer -= dt;
            float alpha = (flash_timer / FLASH_DURATION) * 0.55f;          // fade out over FLASH_DURATION seconds
            if (alpha < 0.0f) alpha = 0.0f;

            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(alpha);
            AEGfxSetColorToMultiply(1.0f, 0.1f, 0.1f, 1.0f);              // red flash
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);     // full screen quad
        }

        // ================================================================
        // 2. BLINK LOGIC (sub-10s only)
        // Toggle blink_visible on a fixed interval so the number flashes
        // ================================================================
        if (time_remaining <= 10.0f)
        {
            blink_timer += dt;
            if (blink_timer >= BLINK_INTERVAL)
            {
                blink_timer = 0.0f;
                blink_visible = !blink_visible;                             // toggle on/off
            }
        }
        else
        {
            blink_visible = true;                                           // always visible above 10s
            blink_timer = 0.0f;
        }

        // ================================================================
        // 3. COUNTDOWN NUMBER — top-center, green -> yellow -> red
        // Scale grows slightly under 10s for extra urgency
        // ================================================================
        if (!blink_visible) return;                                         // skip drawing when in blink-off phase

        AEGfxSetTransparency(1.0f);

        int secs = (int)time_remaining + 1;
        if (secs > (int)TOTAL_TIME) secs = (int)TOTAL_TIME;
        std::string timeStr = std::to_string(secs);

        float cr = (pct < 0.5f) ? 1.0f : (1.0f - pct) * 2.0f;            // ramp red up in second half
        float cg = (pct > 0.5f) ? 1.0f : pct * 2.0f;                      // ramp green down in second half

        float scale = (time_remaining <= 10.0f) ? 0.85f : 0.7f;           // slightly bigger under 10s

        float tw, th;
        AEGfxGetPrintSize(Graphics::font_id, timeStr.c_str(), scale, &tw, &th);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::font_id, timeStr.c_str(),
            -tw * 0.5f,                                                     // centered horizontally
            0.88f,                                                          // top of screen
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
    isPaused = false;
    isTimedOut = false;
    scoreWritten = false;                                              // allow score to be written once this round
    openLeaderboard = false;
    lb_scroll = 0.0f;
    time_remaining = TOTAL_TIME;                                         // reset the countdown to full 60s
    score.score_value = 0;
    random_spawn_timer_seconds = 9999.0f;                                   // start high so first enemy spawns on frame 1
    flash_triggered = false;                                              // allow the 10s flash to fire again
    flash_timer = 0.0f;
    blink_timer = 0.0f;
    blink_visible = true;

    DifficultySystem::Difficulty_Reset();

    // ---- Player textures and sprite mesh ----
    playerTexture1 = AEGfxTextureLoad("Assets/characterSheet.png");
    playerTexture2 = AEGfxTextureLoad("Assets/characterSheet.png");        // placeholder for alt skin slot
    AE_ASSERT_MESG(playerTexture1, "Failed to load characterSheet!");

    if (playerMesh) AEGfxMeshFree(playerMesh);                             // free old mesh if restarting
    playerMesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);           // build sprite sheet mesh

    // ---- Background textures and quad mesh ----
    bgTexture1 = AEGfxTextureLoad("Assets/background.png");
    bgTexture2 = AEGfxTextureLoad("Assets/background.png");                // placeholder for alt bg slot
    AE_ASSERT_MESG(bgTexture1, "Failed to load background!");

    bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);                           // 1x1 unit white quad, scaled at render time

    // ---- Enemy systems ----
    EnemyManage::EnemyManage_InitEngine(word_bank);                         // load word bank from file
    HealthSystem::Health_Init(health);                                      // start at full health
    meshes = EnemyManage::EnemyManage_LoadMeshes(enemy_animation);          // load enemy sprite sheet and meshes
    enemy_area.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f); // spawn line just above the visible screen

    enemy_pool.resize((size_t)EnemyConfig::MAX_ENEMIES);
    free_slot_queue.clear();
    for (int i = 0; i < EnemyConfig::MAX_ENEMIES; ++i)
    {
        enemy_pool[(size_t)i].enemy_is_active = false;                      // all slots start inactive
        free_slot_queue.push_back(i);                                       // all slots go into the free queue
    }

    // ---- Subsystems ----
    Typing_Init();
    Leaderboard_System::Init();                                             // init overlay mesh (used if player somehow dies)
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
}


void GameTimeMode_Update()
{
    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

    // ---- Debug console has first priority; freezes all gameplay while open ----
    DebugConsole::Update();
    if (DebugConsole::IsOpen()) goto render_only;

    // ---- Pause toggle (blocked while timed out so buttons stay clickable) ----
    if (AEInputCheckTriggered(AEVK_ESCAPE) && !isTimedOut)
        isPaused = !isPaused;

    // ---- Gameplay logic (skipped while paused or after time runs out) ----
    if (!isPaused && !isTimedOut)
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();
        // ---- Countdown subtracts BEFORE time-scale so slow power up doesnt affect it ----
        time_remaining -= dt;
        if (time_remaining <= 0.0f)
        {
            time_remaining = 0.0f;                                         // clamp so bar doesnt go negative
            isTimedOut = true;                                             // trigger end-of-round screen next frame
        }
        dt *= PowerUpSystem::GetTimeScale();                                // scale dt AFTER countdown so slow only affects enemies/typing

        DifficultySystem::Difficulty_UpdateOverTime(dt);                   // ramp up difficulty over the 60s

        // ---- Enemy spawning (faster than zen via TIMED_SPAWN_MULTIPLIER) ----
        if (is_random_spawning_enabled)
        {
            random_spawn_timer_seconds += dt;
            float interval = DifficultySystem::Difficulty_GetSpawnIntervalSeconds() * TIMED_SPAWN_MULTIPLIER; // ~2x faster than zen

            if (random_spawn_timer_seconds >= interval)
            {
                random_spawn_timer_seconds = 0.0f;                         // reset spawn timer
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

            if (was_active && !e.enemy_is_active)                          // enemy just became inactive this frame
            {
                free_slot_queue.push_back(i);                               // return slot to free queue
                if (!was_damage_done)
                    Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);  // landed = damage impact
                else
                    Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length); // typed = kill impact
            }
        }

        EnemySystem::Enemy_ResolveLaneStacking(enemy_pool, enemy_area);    // fix spacing between enemies in same lane

        // ---- Power-ups ----
        PowerUpSystem::Update(dt, enemy_pool, enemy_area);
        Particles_Update(dt);
        Weather_Update(dt, enemy_pool);

        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);
            float wx = (float)mx - 800.0f;                                 // convert cursor to world x
            float wy = 450.0f - (float)my;                                 // convert cursor to world y
            PowerUpSystem::OnMouseClick(wx, wy, enemy_pool);               // check if player clicked a power-up drop
        }

        // ---- Typing (gated by weather effects) ----
        if (!Weather_IsPlayerFrozen() && !Weather_IsPlayerStunned())
            Typing_UpdateAndApplyToEnemies(enemy_pool, score, health);

        // ---- Sprite sheet animation ----
        animTimer += (float)AEFrameRateControllerGetFrameTime();
        if (animTimer >= animDuration)
        {
            animTimer = 0.0f;
            currentFrameIndex = (currentFrameIndex + 1) % SPRITE_COLS;     // advance frame, wrap at 8
            int col = currentFrameIndex % SPRITE_COLS;
            int row = currentFrameIndex / SPRITE_COLS;
            uvOffsetX = col * SPRITE_UV_WIDTH;                              // calc UV x offset for this frame
            uvOffsetY = row * SPRITE_UV_HEIGHT;                             // calc UV y offset for this frame
        }
    }

    // ---- Write score once the moment time runs out ----
    if (isTimedOut && !scoreWritten)
    {
        std::string name = Main_Menu_GetPlayerName();
        Leaderboard_System::Write_TimeLeaderboard(name, score.score_value);
        scoreWritten = true;                                                // guard so this only runs once
    }

    // ------------------------------------------------------------------------
    // RENDERING (always runs, even when paused or timed out)
    // ------------------------------------------------------------------------
render_only:

    // ---- Background ----
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_NONE);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTexture* activeBg = (gShopBgID == 0) ? bgTexture1 : bgTexture2;  // pick bg based on shop selection
    AEGfxTextureSet(activeBg, 0.0f, 0.0f);
    Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

    // ---- Player sprite ----
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxTexture* activePlayer = (gShopSkinID == 0) ? playerTexture1 : playerTexture2; // pick skin based on shop selection
    AEGfxTextureSet(activePlayer, uvOffsetX, uvOffsetY);
    if (gShopSkinID == 1) AEGfxSetColorToMultiply(1.0f, 0.6f, 0.6f, 1.0f); // tint alt skin pink
    Mesh_Renderer(playerMesh, SPRITE_X, SPRITE_Y, 150.0f, 150.0f, 0.0f);

    Typing_Render();                                                        // draw the current typed buffer on screen

    // ---- Debug overlays ----
    if (is_enemy_area_box_visible)
    {
        UiSystem::Debug_DrawEnemyAreaBox(meshes, enemy_area);
        UiSystem::Debug_DrawEnemyLaneLines(meshes, enemy_area);
    }

    // ---- Enemies ----
    for (const auto& e : enemy_pool)
        EnemySystem::Enemy_Draw(meshes, e);
    Typing_DrawEnemyWords(enemy_pool);                                      // typed portion green, remainder black

    // ---- FX ----
    Particles_Draw();
    Weather_Draw();

    // ---- HUD ----
    UiSystem::Debug_DrawDifficultyTopRight(is_difficulty_debug_visible, is_random_spawning_enabled, health);
    UiSystem::Ui_DrawScoreAndTyped(score);
    PowerUpSystem::Draw();
    UiSystem::Ui_DrawHealthBar(health, meshes);

    DrawTimerEffects();                                                     // countdown number + 10s flash + sub-10s blink

    // ---- Pause overlay ----
    UiPauseMenu::Check_Paused(isPaused, isTimedOut, bgMesh);               // reuse pause menu; isTimedOut passed as isDead to block pause after timeout

    // ---- End-of-round overlay ----
    if (isTimedOut)
    {
        if (!openLeaderboard)
            UiGameOver::DrawTimeoutScreen(score, bgMesh, openLeaderboard);  // show leaderboard when leaderboard clicked 
        else
            Leaderboard_System::Display_TimeLeaderboard(openLeaderboard);   // show timed leaderboard overlay
    }

    DebugConsole::Render();                                                 // console always renders on top of everything
}


void GameTimeMode_Exit()
{
    EnemyManage::EnemyManage_FreeMeshes(meshes);                           // free all enemy sprite meshes and textures

    Typing_Exit();
    Leaderboard_System::Free();
    PowerUpSystem::Free();
    Particles_Free();
    Weather_Exit();
    DebugConsole::Exit();

    if (playerMesh) { AEGfxMeshFree(playerMesh);           playerMesh = nullptr; }
    if (bgMesh) { AEGfxMeshFree(bgMesh);               bgMesh = nullptr; }

    if (bgTexture1) { AEGfxTextureUnload(bgTexture1);      bgTexture1 = nullptr; }
    if (bgTexture2) { AEGfxTextureUnload(bgTexture2);      bgTexture2 = nullptr; }
    if (playerTexture1) { AEGfxTextureUnload(playerTexture1);  playerTexture1 = nullptr; }
    if (playerTexture2) { AEGfxTextureUnload(playerTexture2);  playerTexture2 = nullptr; }
}