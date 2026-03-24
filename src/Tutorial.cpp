/*
@file           Tutorial.cpp
@author         YOU
@brief          Tutorial game state implementation.
*/
#include "pch.h"
#include "Tutorial.h"


static const float PLAYER_X = -700.0f;
static const float PLAYER_Y = -200.0f;
static const float PLAYER_SIZE = 150.0f;

static const float BUBBLE_X = -380.0f;
static const float BUBBLE_Y = -100.0f;
static const float BUBBLE_W = 480.0f;
static const float BUBBLE_H = 210.0f;
static const float TAIL_X = BUBBLE_X - BUBBLE_W * 0.5f - 12.0f;
static const float TAIL_Y = BUBBLE_Y - 25.0f;

static const float NEXT_BTN_W = 120.0f;
static const float NEXT_BTN_H = 45.0f;
static const float NEXT_BTN_X = BUBBLE_X + BUBBLE_W * 0.5f - NEXT_BTN_W * 0.5f - 15.0f;
static const float NEXT_BTN_Y = BUBBLE_Y - BUBBLE_H * 0.5f + NEXT_BTN_H * 0.5f + 10.0f;

enum TutorialStage
{
    STAGE_BASIC_TYPING = 0,
    STAGE_KILL_BEFORE_LANDING,
    STAGE_FRIENDLY,
    STAGE_SHIELDED,
    STAGE_STREAK,
    STAGE_ORBIT,
    STAGE_POWERUP,
    STAGE_WEATHER,
    STAGE_BOSS_WARNING,
    STAGE_COUNT
};

static const char* STAGE_MESSAGES[STAGE_COUNT] =
{
    "Type the word above the\nenemy and press ENTER\nto defeat it!",
    "Watch out! I'll let this\none land so you can see\nwhat damage looks like...",
    "That's a caged princess!\nType her word to free\nher and gain health!",
    "This enemy has a shield!\nType word 1 to break it,\nthen word 2 to finish!",
    "Build a typing streak!\nType 5 words correctly\nto grow your flame!",
    "Some enemies orbit their\nword around them! Type\nthe word to defeat them.",
    "Power ups fall from the sky! Press\nthe NUMBER shown to activate!\nMore types await in game...",
    "Blizzards freeze you! Use SPACEBAR\n to break free when frozen!\nMore weather events await...",
    "After 60 seconds a BOSS\nappears! Type its words\nto deal damage.\nGood luck - go play!"
};

static const char* STAGE_OBJECTIVES[STAGE_COUNT] =
{
    "Objective: Kill the enemy",
    "Objective: Watch the enemy land",
    "Objective: Free the princess",
    "Objective: Break shield, then kill",
    "Objective: Reach a 5 word streak",
    "Objective: Kill the orbiting enemy",
    "Objective: Activate a power up",
    "Objective: Escape the blizzard!",
    ""
};

namespace
{
    TutorialStage s_stage = STAGE_BASIC_TYPING;
    bool          s_objectiveMet = false;
    bool          s_stageStarted = false;
    bool          s_readyClicked = false;
    float         s_stageTimer = 0.0f;
    bool          s_isPaused = false;
    bool          s_wasPaused = false;
    bool          s_isFading = false;
    bool          s_weatherFreezeStarted = false;
    float         s_fadeTimer = 0.0f;
    static const float FADE_DURATION = 1.2f;

    Data::WordBank      s_wordBank{};
    Data::Health        s_health{};
    Data::Score         s_score{};
    Data::SpriteAnim    s_enemyAnim{};
    Data::SpriteAnim    s_heartAnim{};
    Data::EnemyArea     s_enemyArea{};
    Data::Meshes        s_meshes{};
    std::vector<Data::Enemy> s_pool;
    std::deque<int>     s_freeSlots;

    int s_streakWordIdx = 0;
    static const char* s_streakWords[] = { "RUN", "FLY", "HOP", "JAB", "ZAP", "CAT", "DOG" };

    AEGfxTexture* s_playerTex = nullptr;
    AEGfxVertexList* s_playerMesh = nullptr;
    AEGfxTexture* s_bgTex = nullptr;
    AEGfxVertexList* s_bgMesh = nullptr;
    AEGfxVertexList* s_rectMesh = nullptr;

    float s_winW = 0.0f;
    float s_winH = 0.0f;

    float s_animTimer = 0.0f;
    int   s_animFrame = 0;
    float s_uvX = 0.0f;
    float s_uvY = 0.0f;
    static const float SPRITE_UV_W = 1.0f / 8.0f;
    static const float SPRITE_UV_H = 1.0f;

    // -----------------------------------------------------------------------
    // Enemy update helper - mirrors GameEnemyBoss.cpp particle logic exactly
    // -----------------------------------------------------------------------
    void UpdateEnemies(float dt, bool allowLaser)
    {
        for (int i = 0; i < (int)s_pool.size(); ++i)
        {
            Data::Enemy& e = s_pool[(size_t)i];
            if (!e.enemy_is_active) continue;

            bool was_active = e.enemy_is_active;
            bool was_damage_done = e.enemy_did_damage;

            EnemySystem::Enemy_Update(e, s_enemyArea, dt, s_health);

            if (was_active && !e.enemy_is_active)
            {
                s_freeSlots.push_back(i);
                if (!was_damage_done) // typed kill
                {
                    Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                    if (allowLaser)
                        LaserBeam_Fire(e.enemy_position_x, e.enemy_position_y);
                }
                else // landed
                {
                    Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                    StreakSystem::OnStreakReset();
                }
            }
        }
    }

    void ResetPool()
    {
        s_pool.clear();
        s_pool.resize(40);
        s_freeSlots.clear();
        for (int i = 0; i < 40; ++i)
        {
            s_pool[(size_t)i].enemy_is_active = false;
            s_freeSlots.push_back(i);
        }
    }

    void SpawnNormalEnemy(const std::string& word)
    {
        if (s_freeSlots.empty()) return;
        int slot = s_freeSlots.front();
        s_freeSlots.pop_front();
        Data::Enemy& e = s_pool[(size_t)slot];
        e = {};
        e.enemy_is_active = true;
        e.enemy_word_text = word;
        e.enemy_word_length = (int)word.size();
        e.enemy_position_x = 0.0f;
        e.enemy_position_y = s_enemyArea.area_spawn_y;
        e.enemy_fall_speed_units_per_second = 80.0f;
        e.enemy_lane_id = 4;
        e.enemy_type = Data::EnemyType::NORMAL;
        e.enemy_death_row_index = 1;
        e.enemy_animation.animation_columns = 6;
        e.enemy_animation.animation_rows = 2;
        e.enemy_animation.animation_frame_duration_seconds = 0.1f;
        e.enemy_orbit_initialised = true;                                                                                   // disable orbit so the tutorial word stays in place
    }

    void SpawnFriendlyEnemy(const std::string& word)
    {
        if (s_freeSlots.empty()) return;
        int slot = s_freeSlots.front();
        s_freeSlots.pop_front();
        Data::Enemy& e = s_pool[(size_t)slot];
        e = {};
        e.enemy_is_active = true;
        e.enemy_word_text = word;
        e.enemy_word_length = (int)word.size();
        e.enemy_position_x = 0.0f;
        e.enemy_position_y = s_enemyArea.area_spawn_y;
        e.enemy_fall_speed_units_per_second = 80.0f;
        e.enemy_lane_id = 4;
        e.enemy_type = Data::EnemyType::FRIENDLY;
        e.enemy_death_row_index = 1;
        e.enemy_animation.animation_columns = 6;
        e.enemy_animation.animation_rows = 3;
        e.enemy_animation.animation_frame_duration_seconds = 0.1f;
        e.enemy_orbit_initialised = true;                                                                                   // disable orbit so the tutorial word stays in place
    }

    void SpawnShieldedEnemy(const std::string& word1, const std::string& word2)
    {
        if (s_freeSlots.empty()) return;
        int slot = s_freeSlots.front();
        s_freeSlots.pop_front();
        Data::Enemy& e = s_pool[(size_t)slot];
        e = {};
        e.enemy_is_active = true;
        e.enemy_word_text = word1;
        e.enemy_word_text_alt = word2;
        e.enemy_word_length = (int)word1.size();
        e.enemy_position_x = 0.0f;
        e.enemy_position_y = s_enemyArea.area_spawn_y;
        e.enemy_fall_speed_units_per_second = 80.0f;
        e.enemy_lane_id = 4;
        e.enemy_type = Data::EnemyType::SHIELDED;
        e.enemy_shield_intact = true;
        e.enemy_death_row_index = 2;
        e.enemy_animation.animation_columns = 6;
        e.enemy_animation.animation_rows = 3;
        e.enemy_animation.animation_frame_duration_seconds = 0.1f;
        e.enemy_orbit_initialised = true;                                                                                   // disable orbit so the tutorial word stays in place
    }

    void SpawnStreakEnemy(const std::string& word)
    {
        if (s_freeSlots.empty()) return;
        int slot = s_freeSlots.front();
        s_freeSlots.pop_front();
        Data::Enemy& e = s_pool[(size_t)slot];
        e = {};
        e.enemy_is_active = true;
        e.enemy_word_text = word;
        e.enemy_word_length = (int)word.size();
        e.enemy_position_x = 0.0f;
        e.enemy_position_y = s_enemyArea.area_spawn_y;
        e.enemy_fall_speed_units_per_second = 80.0f;
        e.enemy_lane_id = 4;
        e.enemy_type = Data::EnemyType::NORMAL;
        e.enemy_death_row_index = 1;
        e.enemy_animation.animation_columns = 6;
        e.enemy_animation.animation_rows = 2;
        e.enemy_animation.animation_frame_duration_seconds = 0.1f;
        e.enemy_orbit_initialised = true;                                                                                   // disable orbit so the tutorial word stays in place
    }

    void SpawnOrbitEnemy(const std::string& word)
    {
        if (s_freeSlots.empty()) return;
        int slot = s_freeSlots.front();
        s_freeSlots.pop_front();
        Data::Enemy& e = s_pool[(size_t)slot];
        e = {};
        e.enemy_is_active = true;
        e.enemy_word_text = word;
        e.enemy_word_length = (int)word.size();
        e.enemy_position_x = 0.0f;
        e.enemy_position_y = s_enemyArea.area_spawn_y;
        e.enemy_fall_speed_units_per_second = 80.0f;
        e.enemy_lane_id = 4;
        e.enemy_type = Data::EnemyType::NORMAL;
        e.enemy_death_row_index = 1;
        e.enemy_animation.animation_columns = 6;
        e.enemy_animation.animation_rows = 2;
        e.enemy_animation.animation_frame_duration_seconds = 0.1f;

        // force orbit for tutorial
        e.enemy_orbit_initialised = true;
        e.enemy_has_orbit_effect = true;
        e.enemy_orbit_phase = 0;
        e.enemy_orbit_idle_timer = 2.0f;
        e.enemy_orbit_radius = 0.0f;
        e.enemy_orbit_timer = 0.0f;
    }

    bool AnyEnemyActive()
    {
        for (const auto& e : s_pool)
            if (e.enemy_is_active) return true;
        return false;
    }

    void StartStage(TutorialStage stage)
    {
        s_stage = stage;
        s_objectiveMet = false;
        s_stageStarted = false;
        s_readyClicked = false;
        s_weatherFreezeStarted = false;
        s_stageTimer = 0.0f;
        s_streakWordIdx = 0;
        ResetPool();
        Typing_Init();
        StreakSystem::Init();
    }

    void DrawSpeechBubble(bool objectiveMet)
    {
        if (!s_rectMesh || Graphics::font_id < 0) return;
        if (s_stage < 0 || s_stage >= STAGE_COUNT) return;

        // Gold border
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.9f, 0.85f, 0.3f, 1.0f);
        Mesh_Renderer(s_rectMesh, BUBBLE_X, BUBBLE_Y, BUBBLE_W + 6.0f, BUBBLE_H + 6.0f, 0.0f);

        // Dark background
        AEGfxSetTransparency(0.92f);
        AEGfxSetColorToMultiply(0.08f, 0.08f, 0.12f, 1.0f);
        Mesh_Renderer(s_rectMesh, BUBBLE_X, BUBBLE_Y, BUBBLE_W, BUBBLE_H, 0.0f);

        // Tail pointing left toward player
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.9f, 0.85f, 0.3f, 1.0f);
        Mesh_Renderer(s_rectMesh, TAIL_X, TAIL_Y, 28.0f, 22.0f, 0.0f);
        AEGfxSetTransparency(0.92f);
        AEGfxSetColorToMultiply(0.08f, 0.08f, 0.12f, 1.0f);
        Mesh_Renderer(s_rectMesh, TAIL_X + 4.0f, TAIL_Y, 20.0f, 16.0f, 0.0f);

        // Message text
        float ndcX = (BUBBLE_X - BUBBLE_W * 0.5f + 18.0f) / (s_winW * 0.5f);
        float ndcY = (BUBBLE_Y + BUBBLE_H * 0.5f - 35.0f) / (s_winH * 0.5f);
        float spacing = 0.085f;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        std::string msg = STAGE_MESSAGES[s_stage];
        float lineY = ndcY;
        std::string line;
        for (size_t i = 0; i <= msg.size(); ++i)
        {
            if (i == msg.size() || msg[i] == '\n')
            {
                if (!line.empty())
                {
                    AEGfxPrint(Graphics::font_id, line.c_str(), ndcX, lineY, 0.42f, 1.0f, 1.0f, 1.0f, 1.0f);
                    lineY -= spacing;
                }
                line.clear();
            }
            else line += msg[i];
        }

        // Objective text
        if (s_stage < STAGE_BOSS_WARNING)
        {
            float ndcObjX = (BUBBLE_X - BUBBLE_W * 0.5f + 18.0f) / (s_winW * 0.5f);
            float ndcObjY = (BUBBLE_Y - BUBBLE_H * 0.5f + 55.0f) / (s_winH * 0.5f);
            float or_ = objectiveMet ? 0.2f : 1.0f;
            float og_ = objectiveMet ? 1.0f : 0.85f;
            float ob_ = objectiveMet ? 0.2f : 0.1f;
            AEGfxPrint(Graphics::font_id,
                objectiveMet ? "Objective complete!" : STAGE_OBJECTIVES[s_stage],
                ndcObjX, ndcObjY, 0.38f, or_, og_, ob_, 1.0f);
        }

        // ENTER hint text at bottom of bubble
        if (Graphics::font_id >= 0)
        {
            const char* hint = nullptr;
            float hr = 0.0f, hg = 0.0f, hb = 0.0f;

            if (s_stage == STAGE_BOSS_WARNING)
            {
                hint = "[ Press ENTER to continue ]";
                hr = 0.2f; hg = 0.7f; hb = 1.0f;
            }
            else if (objectiveMet)
            {
                hint = "[ Press ENTER to continue ]";
                hr = 0.2f; hg = 0.7f; hb = 1.0f; // blue when done
            }
            else if (!s_readyClicked)
            {
                hint = "[ Press ENTER when ready ]";
                hr = 1.0f; hg = 0.85f; hb = 0.1f; // yellow before start
            }

            if (hint)
            {
                float hw, hh;
                AEGfxGetPrintSize(Graphics::font_id, hint, 0.40f, &hw, &hh);
                float hndcX = (BUBBLE_X - BUBBLE_W * 0.5f + 18.0f) / (s_winW * 0.5f);
                float hndcY = (BUBBLE_Y - BUBBLE_H * 0.5f + 18.0f) / (s_winH * 0.5f);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::font_id, hint, hndcX, hndcY, 0.40f, hr, hg, hb, 1.0f);
            }
        }


    }

    bool EnterPressed()
    {
        return AEInputCheckTriggered(AEVK_RETURN);
    }

    bool PlayButtonClicked()
    {
        // PLAY > button on final stage � kept as mouse click
        if (!AEInputCheckTriggered(AEVK_LBUTTON)) return false;
        s32 mx, my;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - s_winW * 0.5f;
        float wy = s_winH * 0.5f - (float)my;
        return (wx >= NEXT_BTN_X - NEXT_BTN_W * 0.5f &&
            wx <= NEXT_BTN_X + NEXT_BTN_W * 0.5f &&
            wy >= NEXT_BTN_Y - NEXT_BTN_H * 0.5f &&
            wy <= NEXT_BTN_Y + NEXT_BTN_H * 0.5f);
    }
}

void Tutorial_Init()
{
    s_winW = (float)AEGfxGetWindowWidth();
    s_winH = (float)AEGfxGetWindowHeight();

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    Typing_Exit();
    PowerUpSystem::Free();
    Weather_Exit();
    Particles_Free();
    StreakSystem::Exit();
    BackgroundParallax::MenuParallax_Exit();
    GameClouds::Exit();
    LaserBeam_Free();
    EnemyManage::EnemyManage_Free(s_meshes, s_pool, s_freeSlots);


    // for when restarting tutorial
    s_score = {};
    s_health = {};
    s_wordBank = {};
    s_enemyArea = {};

    if (s_bgTex) { AEGfxTextureUnload(s_bgTex); s_bgTex = nullptr; }
    if (s_bgMesh) { AEGfxMeshFree(s_bgMesh); s_bgMesh = nullptr; }
    if (s_playerTex) { AEGfxTextureUnload(s_playerTex); s_playerTex = nullptr; }
    if (s_playerMesh) { AEGfxMeshFree(s_playerMesh); s_playerMesh = nullptr; }
    if (s_rectMesh) { AEGfxMeshFree(s_rectMesh); s_rectMesh = nullptr; }

    s_rectMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    s_playerTex = AEGfxTextureLoad("Assets/characterSheet.png");
    s_playerMesh = Sprite_Mesh(SPRITE_UV_W, SPRITE_UV_H);
    s_bgTex = AEGfxTextureLoad("Assets/background.png");
    s_bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

    EnemyManage::EnemyManage_InitEngine(s_wordBank);
    HealthSystem::Health_Init(s_health);
    s_meshes = EnemyManage::EnemyManage_LoadMeshes(s_enemyAnim);

    // Match GameEnemyBoss::Init exactly - only set spawn_y, leave landing_y at default -250
    s_enemyArea.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f);

    ResetPool();
    Typing_Init();
    PowerUpSystem::Init();
    Weather_Init();
    Particles_Init();
    StreakSystem::Init();
    BackgroundParallax::MenuParallax_Init();
    GameClouds::Init();
    LaserBeam_Init();
    LaserBeam_SetOrigin(PLAYER_X + 75.0f, PLAYER_Y);

    s_animTimer = 0.0f;
    s_animFrame = 0;
    s_uvX = 0.0f;
    s_uvY = 0.0f;
    s_isPaused = false;
    s_wasPaused = false;
    s_isFading = false;
    s_fadeTimer = 0.0f;

    // Initialise shared heart animation state (6 frames, single row sprite sheet)
    s_heartAnim = {};
    s_heartAnim.animation_columns = 6;
    s_heartAnim.animation_rows = 1;
    s_heartAnim.animation_frame_duration_seconds = 0.10f;

    StartStage(STAGE_BASIC_TYPING);
}

void Tutorial_Update()
{
    s_winW = (float)AEGfxGetWindowWidth();
    s_winH = (float)AEGfxGetWindowHeight();

    float dt = (float)AEFrameRateControllerGetFrameTime();
    if (dt > 0.1f) dt = 0.01667f;

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    // Fade to black transition to main menu
    if (s_isFading)
    {
        s_fadeTimer += dt;
        float alpha = s_fadeTimer / FADE_DURATION;
        if (alpha > 1.0f) alpha = 1.0f;

        // Draw the last frame of the tutorial underneath
        if (s_bgTex && s_bgMesh)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_NONE);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(s_bgTex, 0.0f, 0.0f);
            Mesh_Renderer(s_bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
        }

        // Black overlay that grows from 0 to fully opaque
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        Mesh_Renderer(s_rectMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

        if (s_fadeTimer >= FADE_DURATION)
            gGameStateNext = GS_MAIN_MENU;
        return;
    }

    // ESC pause
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        if (s_isPaused)
        {
            // If a sub-menu is open, let the UI handle the ESC key! Do NOT unpause!
            if (UiPauseMenu::IsSettingsOpen() || UiPauseMenu::IsPopupOpen())
            {
                // Do nothing here
            }
            else
            {
                s_isPaused = false;
            }
        }
        else
        {
            s_isPaused = true;
        }
    }

    if (!s_isPaused)
    {
        BackgroundParallax::MenuParallax_Update(dt);
        GameClouds::Update(dt);
        Particles_Update(dt);
        StreakSystem::Update(dt);
        LaserBeam_Update(dt);

        // Player animation
        s_animTimer += dt;
        if (s_animTimer >= 0.1f)
        {
            s_animTimer = 0.0f;
            s_animFrame = (s_animFrame + 1) % 8;
            s_uvX = s_animFrame * SPRITE_UV_W;
        }

        s_stageTimer += dt;
    }

    if (!s_isPaused && !s_objectiveMet)
    {
        // Handle ENTER to start stage
        if (!s_readyClicked && s_stage != STAGE_BOSS_WARNING)
        {
            if (EnterPressed())
                s_readyClicked = true;
        }

        switch (s_stage)
        {
            // Stage 1: Basic typing - kill the enemy
        case STAGE_BASIC_TYPING:
            if (s_readyClicked && !s_stageStarted) { SpawnNormalEnemy("CAT"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                Typing_UpdateAndApplyToEnemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::Enemy_Update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying) // typed kill finishing fall
                        {
                            Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);

                            //AE_ASSERT_MESG(false, "BRANCH: was_dying - SpawnDamageImpact");

                        }
                        else if (!was_damage_done) // fresh first frame of typed kill
                        {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::OnStreakReset();
                            HealthSystem::Health_Init(s_health);
                            SpawnNormalEnemy("CAT");
                            break;

                        }
                        else // landed and dealt damage
                        {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);

                            //AE_ASSERT_MESG(false, "BRANCH: landed - SpawnKillImpact");

                            StreakSystem::OnStreakReset();
                            HealthSystem::Health_Init(s_health);
                            SpawnNormalEnemy("CAT");
                            break;
                        }
                    }
                }
                if (!AnyEnemyActive() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 2: Enemy lands on purpose - no typing allowed
        case STAGE_KILL_BEFORE_LANDING:
            if (s_readyClicked && !s_stageStarted) { SpawnNormalEnemy("OUCH"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                UpdateEnemies(dt, false);
                if (!AnyEnemyActive() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 3: Friendly NPC
        case STAGE_FRIENDLY:
            if (s_readyClicked && !s_stageStarted) { SpawnFriendlyEnemy("BIRD"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                Typing_UpdateAndApplyToEnemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::Enemy_Update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (e.enemy_type == Data::EnemyType::FRIENDLY && !e.enemy_has_landed)
                        {
                            //AE_ASSERT_MESG(false, "BRANCH: was_dying - SpawnDamageImpact");
                            // friendly flew away after being typed � already handled by Typing.cpp
                            // Particles_SpawnFriendlyHeal already called there, just set objective
                            s_objectiveMet = true;
                        }
                        else if (was_dying) {
                            Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                            //  AE_ASSERT_MESG(false, "2");
                        }
                        else if (!was_damage_done) {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::OnStreakReset();
                            SpawnFriendlyEnemy("BIRD");
                            break;
                        }
                        else
                        {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::OnStreakReset();
                            SpawnFriendlyEnemy("BIRD");
                            break;
                        }
                    }
                }
                if (!AnyEnemyActive() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 4: Shielded enemy
        case STAGE_SHIELDED:
            if (s_readyClicked && !s_stageStarted) { SpawnShieldedEnemy("ICE", "FIRE"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                Typing_UpdateAndApplyToEnemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::Enemy_Update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying)
                            Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                        else if (!was_damage_done) {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::OnStreakReset();
                            HealthSystem::Health_Init(s_health);
                            SpawnShieldedEnemy("ICE", "FIRE");
                            break;
                        }
                        else
                        {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::OnStreakReset();
                            HealthSystem::Health_Init(s_health);
                            SpawnShieldedEnemy("ICE", "FIRE");
                            break;
                        }
                    }
                }
                if (!AnyEnemyActive() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 5: Streak of 5 - staggered spawns
        case STAGE_STREAK:
        {
            if (s_readyClicked && !s_stageStarted) { s_stageStarted = true; Typing_Init(); }

            if (s_stageStarted)
            {
                if (!AnyEnemyActive() && s_stageTimer > 0.3f)
                {
                    SpawnStreakEnemy(s_streakWords[s_streakWordIdx % 7]);
                    s_streakWordIdx++;
                    s_stageTimer = 0.0f;
                }

                Typing_UpdateAndApplyToEnemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::Enemy_Update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying)
                            Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                        else if (!was_damage_done) {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            HealthSystem::Health_Init(s_health);
                        }
                        else
                        {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            HealthSystem::Health_Init(s_health);
                        }
                    }
                }

                if (StreakSystem::GetStreak() >= 5 && !AnyEnemyActive())
                    s_objectiveMet = true;
            }
            break;
        }

        // Stage 6: Orbit - show an enemy whose word orbits around it
        case STAGE_ORBIT:
            if (s_readyClicked && !s_stageStarted) { SpawnOrbitEnemy("SPIN"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                Typing_UpdateAndApplyToEnemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::Enemy_Update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying)
                            Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                        else if (!was_damage_done) {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::OnStreakReset();
                            HealthSystem::Health_Init(s_health);
                            SpawnOrbitEnemy("SPIN");
                            break;
                        }
                        else
                        {
                            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::OnStreakReset();
                            HealthSystem::Health_Init(s_health);
                            SpawnOrbitEnemy("SPIN");
                            break;
                        }
                    }
                }
                if (!AnyEnemyActive() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 7: Power up - spawn on READY
        case STAGE_POWERUP:
            if (s_readyClicked && !s_stageStarted)
            {
                PowerUpSystem::Update(0.0f, s_pool, s_enemyArea, s_health);
                PowerUpSystem::DebugSpawn(PowerUpSystem::PowerType::Shield);
                s_stageStarted = true;
            }
            if (s_stageStarted)
            {
                PowerUpSystem::Update(dt, s_pool, s_enemyArea, s_health);

                static bool shieldWasActive = false;

                if (PowerUpSystem::IsShieldActive())
                {
                    shieldWasActive = true;
                }

                if (shieldWasActive && !PowerUpSystem::IsShieldActive())
                {
                    s_objectiveMet = true;
                    shieldWasActive = false; // reset for safety
                }
            }
            break;

            // Stage 8: Weather - trigger on READY
        case STAGE_WEATHER:
            if (s_readyClicked && !s_stageStarted) { Weather_ForceEvent(0, s_pool); s_stageStarted = true; }
            if (s_stageStarted)
            {
                Weather_Update(dt, s_pool);
                // Mark that freeze has started
                if (Weather_IsPlayerFrozen())
                    s_weatherFreezeStarted = true;
                // Only complete once freeze has started AND ended
                if (s_weatherFreezeStarted && !Weather_IsPlayerFrozen())
                    s_objectiveMet = true;
            }
            break;

            // Stage 9: Boss warning - informational only
        case STAGE_BOSS_WARNING:
            s_objectiveMet = true;
            break;

        default: break;
        }
    }

    // NEXT / PLAY button
    if (!s_isPaused && s_objectiveMet && EnterPressed() && s_stage != STAGE_BOSS_WARNING)
    {
        StartStage((TutorialStage)(s_stage + 1));
    }

    if (!s_isPaused && s_stage == STAGE_BOSS_WARNING && s_objectiveMet && EnterPressed())
    {
        s_isFading = true;
        s_fadeTimer = 0.0f;
    }

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------
    if (s_bgTex && s_bgMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_bgTex, 0.0f, 0.0f);
        Mesh_Renderer(s_bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
    }

    GameClouds::Draw();

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    for (const auto& e : s_pool)
        EnemySystem::Enemy_Draw(s_meshes, e);
    Typing_DrawEnemyWords(s_pool);

    PowerUpSystem::Draw();

    if (s_stage == STAGE_POWERUP && PowerUpSystem::IsShieldActive() && Graphics::font_id >= 0)
    {
        float bob = sinf(s_stageTimer * 5.0f) * 6.0f;

        float arrowWorldX = -s_winW * 0.5f + 140.0f + bob;
        float arrowWorldY = -s_winH * 0.5f + 35.0f;

        float ax, ay, aw, ah;
        const char* arrow = "<";
        AEGfxGetPrintSize(Graphics::font_id, arrow, 0.55f, &aw, &ah);
        ax = (arrowWorldX / (s_winW * 0.5f)) - aw * 0.5f;
        ay = (arrowWorldY / (s_winH * 0.5f)) - ah * 0.5f;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::font_id, arrow, ax, ay, 0.55f, 0.9f, 0.85f, 0.3f, 1.0f);

        const char* label = "Power-up active!";
        float lw, lh;
        AEGfxGetPrintSize(Graphics::font_id, label, 0.35f, &lw, &lh);
        float lx = (arrowWorldX / (s_winW * 0.5f)) - lw * 0.5f;
        float ly = ((arrowWorldY + 45.0f) / (s_winH * 0.5f)) - lh * 0.5f;
        AEGfxPrint(Graphics::font_id, label, lx, ly, 0.35f, 0.9f, 0.85f, 0.3f, 1.0f);
    }
    Weather_Draw();
    Particles_Draw();
    LaserBeam_Draw();
    StreakSystem::Draw();

    if (s_playerTex && s_playerMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_playerTex, s_uvX, s_uvY);
        Mesh_Renderer(s_playerMesh, PLAYER_X, PLAYER_Y, PLAYER_SIZE, PLAYER_SIZE, 0.0f);
    }

    Typing_Render();
    UiSystem::Ui_DrawScoreAndTyped(s_score);
    if (s_stage == STAGE_KILL_BEFORE_LANDING || s_stage == STAGE_FRIENDLY)
        UiSystem::Ui_DrawHealthBar(s_health, s_meshes, s_heartAnim, dt);

    DrawSpeechBubble(s_objectiveMet);

    bool isDead = false;
    UiPauseMenu::Check_Paused(s_isPaused, isDead, s_rectMesh);

    // --- Audio Pause Tracking ---
    if (s_isPaused && !s_wasPaused) {
        SoundManager::PauseGameplaySounds();
    }
    else if (!s_isPaused && s_wasPaused) {
        SoundManager::ResumeGameplaySounds();
    }
    s_wasPaused = s_isPaused;
}

void Tutorial_Exit()
{
    Typing_Exit();
    PowerUpSystem::Free();
    Weather_Exit();
    Particles_Free();
    StreakSystem::Exit();
    BackgroundParallax::MenuParallax_Exit();
    GameClouds::Exit();
    LaserBeam_Free();

    EnemyManage::EnemyManage_Free(s_meshes, s_pool, s_freeSlots);

    if (s_bgTex) { AEGfxTextureUnload(s_bgTex);     s_bgTex = nullptr; }
    if (s_bgMesh) { AEGfxMeshFree(s_bgMesh);          s_bgMesh = nullptr; }
    if (s_playerTex) { AEGfxTextureUnload(s_playerTex);  s_playerTex = nullptr; }
    if (s_playerMesh) { AEGfxMeshFree(s_playerMesh);      s_playerMesh = nullptr; }
    if (s_rectMesh) { AEGfxMeshFree(s_rectMesh);        s_rectMesh = nullptr; }

    AEGfxSetCamPosition(0.0f, 0.0f);
}