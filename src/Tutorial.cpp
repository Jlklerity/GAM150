// File: Tutorial.cpp
// Author: Pengfei
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

static const char* s_STAGE_MESSAGES[STAGE_COUNT] =
{
    "Type the word above the\nenemy and press ENTER\nto defeat it!",
    "Watch out! I'll let this\none land so you can see\nwhat damage looks like...",
    "That's a caged princess!\nType her word to free\nher and gain health!",
    "This enemy has a shield!\nType word 1 to break it,\nthen word 2 to finish!",
    "Build a typing streak!\nType 5 words correctly\nto grow your flame!",
    "Some enemies orbit their\nword around them! Type\nthe word to defeat them.",
    "Power ups fall from the sky!\nPress the NUMBER shown\nto activate it!",
    "Blizzards freeze you! Use SPACEBAR\n to break free when frozen!\nMore weather events await...",
    "After 60 seconds a BOSS\nappears! Type its words\nto deal damage.\nGood luck - go play!"
};

static const char* s_STAGE_OBJECTIVES[STAGE_COUNT] =
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

    // Powerup stage sub-phase state
    int   s_powerupPhase = 0;         // 0=Lightning, 1=Shield, 2=TimeSlow
    float s_powerupPhaseTimer = 0.0f; // counts up after activation
    float s_powerupSpawnTimer = 0.0f; // fallback timer for shield/slow phases

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

    // steps every active enemy forward by dt, recycles its slot on death,
    // spawns a damage-impact particle on a typed kill and optionally fires
    // the laser beam (allowLaser=false for stages where the laser is suppressed),
    // spawns a kill-impact particle and resets the streak when an enemy lands
    void update_enemies(float dt, bool allowLaser)
    {
        for (int i = 0; i < (int)s_pool.size(); ++i)
        {
            Data::Enemy& e = s_pool[(size_t)i];
            if (!e.enemy_is_active) continue;

            bool was_active = e.enemy_is_active;
            bool was_damage_done = e.enemy_did_damage;

            EnemySystem::enemy_update(e, s_enemyArea, dt, s_health);

            if (was_active && !e.enemy_is_active)
            {
                s_freeSlots.push_back(i);
                if (!was_damage_done) // typed kill
                {
                    particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                    if (allowLaser)
                        laser_beam_fire(e.enemy_position_x, e.enemy_position_y);
                }
                else // landed
                {
                    particles_spawn_kill_impact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                    StreakSystem::on_streak_reset();
                }
            }
        }
    }

    // clears s_pool to 40 inactive slots and rebuilds s_freeSlots from scratch,
    // called at the start of every stage to guarantee a clean enemy state
    void reset_pool()
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

    // claims the next free slot and places a standard falling enemy at the top
    // of the enemy area with the given word - orbit is pre-initialised to keep
    // the word stationary so it stays readable during tutorial explanations
    void spawn_normal_enemy(const std::string& word)
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

    // same as SpawnNormalEnemy but sets type to FRIENDLY (princess sprite, 3-row sheet),
    // typing the word frees her and restores health instead of dealing damage
    void spawn_friendly_enemy(const std::string& word)
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

    // places a shielded enemy that requires two words - word1 breaks the shield,
    // word2 finishes the kill; enemy_shield_intact is set true on spawn
    void spawn_shielded_enemy(const std::string& word1, const std::string& word2)
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

    // spawns a normal enemy from the s_streakWords pool for the streak stage,
    // orbit is suppressed so the word stays fixed while the player builds their combo
    void spawn_streak_enemy(const std::string& word)
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

    // spawns a normal enemy with orbit explicitly enabled - sets enemy_has_orbit_effect
    // and seeds the orbit phase/timer so the word visibly circles the sprite from the start,
    // used exclusively in STAGE_ORBIT to demonstrate the mechanic
    void spawn_orbit_enemy(const std::string& word)
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

    // returns true if at least one slot in s_pool is still active,
    // used by stages to gate objective completion until the screen is clear
    bool any_enemy_active()
    {
        for (const auto& e : s_pool)
            if (e.enemy_is_active) return true;
        return false;
    }

    // transitions to a new stage - resets all per-stage flags (objectiveMet,
    // stageStarted, readyClicked, weatherFreezeStarted), clears timers and
    // streak/powerup sub-phase state, calls ResetPool, and re-initialises
    // Typing and StreakSystem so carry-over input never bleeds between stages
    void start_stage(TutorialStage stage)
    {
        s_stage = stage;
        s_objectiveMet = false;
        s_stageStarted = false;
        s_readyClicked = false;
        s_weatherFreezeStarted = false;
        s_stageTimer = 0.0f;
        s_streakWordIdx = 0;
        s_powerupPhase = 0;
        s_powerupPhaseTimer = 0.0f;
        s_powerupSpawnTimer = 0.0f;
        reset_pool();
        typing_init();
        StreakSystem::init();
    }

    // renders the teacher speech bubble in the lower-left quadrant - draws a gold-bordered
    // dark panel with a left-pointing tail, prints the current stage message and objective
    // text (green when met, white when pending), and shows a context-sensitive ENTER hint
    // at the bottom (yellow = not started, blue = complete or boss stage)
    void draw_speech_bubble(bool objectiveMet)
    {
        if (!s_rectMesh || Graphics::g_font_id < 0) return;
        if (s_stage < 0 || s_stage >= STAGE_COUNT) return;

        // Gold border
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.9f, 0.85f, 0.3f, 1.0f);
        mesh_renderer(s_rectMesh, BUBBLE_X, BUBBLE_Y, BUBBLE_W + 6.0f, BUBBLE_H + 6.0f, 0.0f);

        // Dark background
        AEGfxSetTransparency(0.92f);
        AEGfxSetColorToMultiply(0.08f, 0.08f, 0.12f, 1.0f);
        mesh_renderer(s_rectMesh, BUBBLE_X, BUBBLE_Y, BUBBLE_W, BUBBLE_H, 0.0f);

        // Tail pointing left toward player
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.9f, 0.85f, 0.3f, 1.0f);
        mesh_renderer(s_rectMesh, TAIL_X, TAIL_Y, 28.0f, 22.0f, 0.0f);
        AEGfxSetTransparency(0.92f);
        AEGfxSetColorToMultiply(0.08f, 0.08f, 0.12f, 1.0f);
        mesh_renderer(s_rectMesh, TAIL_X + 4.0f, TAIL_Y, 20.0f, 16.0f, 0.0f);

        // Message text
        float ndc_x = (BUBBLE_X - BUBBLE_W * 0.5f + 18.0f) / (s_winW * 0.5f);
        float ndc_y = (BUBBLE_Y + BUBBLE_H * 0.5f - 35.0f) / (s_winH * 0.5f);
        float spacing = 0.085f;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        std::string msg = s_STAGE_MESSAGES[s_stage];
        float line_y = ndc_y;
        std::string line;
        for (size_t i = 0; i <= msg.size(); ++i)
        {
            if (i == msg.size() || msg[i] == '\n')
            {
                if (!line.empty())
                {
                    AEGfxPrint(Graphics::g_font_id, line.c_str(), ndc_x, line_y, 0.42f, 1.0f, 1.0f, 1.0f, 1.0f);
                    line_y -= spacing;
                }
                line.clear();
            }
            else line += msg[i];
        }

        // Objective text
        if (s_stage < STAGE_BOSS_WARNING)
        {
            float ndc_obj_x = (BUBBLE_X - BUBBLE_W * 0.5f + 18.0f) / (s_winW * 0.5f);
            float ndc_obj_y = (BUBBLE_Y - BUBBLE_H * 0.5f + 55.0f) / (s_winH * 0.5f);
            float obj_r = objectiveMet ? 0.2f : 1.0f;
            float obj_g = objectiveMet ? 1.0f : 0.85f;
            float obj_b = objectiveMet ? 0.2f : 0.1f;
            AEGfxPrint(Graphics::g_font_id,
                objectiveMet ? "Objective complete!" : s_STAGE_OBJECTIVES[s_stage],
                ndc_obj_x, ndc_obj_y, 0.38f, obj_r, obj_g, obj_b, 1.0f);
        }

        // ENTER hint text at bottom of bubble
        if (Graphics::g_font_id >= 0)
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
                AEGfxGetPrintSize(Graphics::g_font_id, hint, 0.40f, &hw, &hh);
                float hndc_x = (BUBBLE_X - BUBBLE_W * 0.5f + 18.0f) / (s_winW * 0.5f);
                float hndc_y = (BUBBLE_Y - BUBBLE_H * 0.5f + 18.0f) / (s_winH * 0.5f);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::g_font_id, hint, hndc_x, hndc_y, 0.40f, hr, hg, hb, 1.0f);
            }
        }


    }

    // thin wrapper around AEInputCheckTriggered(AEVK_RETURN) - centralises
    // the key query so stages and the bubble hint always agree on the same input
    bool enter_pressed()
    {
        return AEInputCheckTriggered(AEVK_RETURN);
    }

    // returns true when the left mouse button is triggered and the cursor falls
    // within the NEXT/PLAY button bounds (world-space AABB converted from screen coords),
    // only used on the final boss-warning stage where the button ends the tutorial
    bool play_button_clicked()
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

// initialises all tutorial subsystems from a clean slate - unloads any leftover
// state from a previous run (textures, meshes, pools, subsystem exits), then loads
// the background and player sprite sheet, sets up the enemy area spawn boundary,
// initialises PowerUpSystem, Weather, Particles, Streaks, LaserBeam, and clouds,
// configures the heart animation, and kicks off STAGE_BASIC_TYPING via StartStage
void tutorial_init()
{
    s_winW = (float)AEGfxGetWindowWidth();
    s_winH = (float)AEGfxGetWindowHeight();

    AEGfxSetCamPosition(0.0f, 0.0f);
    AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);

    typing_exit();
    PowerUpSystem::free();
    weather_exit();
    particles_free();
    StreakSystem::cleanup();
    BackgroundParallax::menu_parallax_exit();
    GameClouds::cleanup();
    laser_beam_free();
    EnemyManage::enemy_manage_free(s_meshes, s_pool, s_freeSlots);


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

    EnemyManage::enemy_manage_init_engine(s_wordBank);
    HealthSystem::health_init(s_health);
    s_meshes = EnemyManage::enemy_manage_load_meshes(s_enemyAnim);

    // Match GameEnemyBoss::Init exactly - only set spawn_y, leave landing_y at default -250
    s_enemyArea.area_spawn_y = AEGfxGetWinMaxY() + (EnemyConfig::ENEMY_SIZE_PX * 0.5f);

    reset_pool();
    typing_init();
    PowerUpSystem::init();
    weather_init();
    particles_init();
    StreakSystem::init();
    BackgroundParallax::menu_parallax_init();
    GameClouds::init();
    laser_beam_init();
    laser_beam_set_origin(PLAYER_X + 75.0f, PLAYER_Y);

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

    start_stage(STAGE_BASIC_TYPING);
}

// runs the tutorial game loop each frame - handles the fade-to-black transition on
// completion, toggles pause via ESC (respecting settings/popup sub-menus), advances
// the player sprite animation, and dispatches per-stage logic through a switch on
// s_stage; each stage waits for ENTER (s_readyClicked) before spawning its enemy,
// runs the appropriate typing/enemy/weather update, and sets s_objectiveMet when
// its condition is satisfied; pressing ENTER again after completion calls StartStage
// for the next stage, or begins the fade on STAGE_BOSS_WARNING; renders background,
// clouds, enemies, words, power-ups, weather, particles, laser, streak, player sprite,
// HUD, and the speech bubble every frame; syncs audio pause state with s_isPaused
void tutorial_update()
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
            mesh_renderer(s_bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
        }

        // Black overlay that grows from 0 to fully opaque
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        mesh_renderer(s_rectMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

        if (s_fadeTimer >= FADE_DURATION)
            g_game_state_next = GS_MAIN_MENU;
        return;
    }

    // ESC pause
    if (AEInputCheckTriggered(AEVK_ESCAPE))
    {
        if (s_isPaused)
        {
            // If a sub-menu is open, let the UI handle the ESC key! Do NOT unpause!
            if (UiPauseMenu::is_settings_open() || UiPauseMenu::is_popup_open())
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
        BackgroundParallax::menu_parallax_update(dt);
        GameClouds::update(dt);
        particles_update(dt);
        StreakSystem::update(dt);
        laser_beam_update(dt);

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
            if (enter_pressed())
                s_readyClicked = true;
        }

        switch (s_stage)
        {
            // Stage 1: Basic typing - kill the enemy
        case STAGE_BASIC_TYPING:
            if (s_readyClicked && !s_stageStarted) { spawn_normal_enemy("CAT"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                typing_update_and_apply_to_enemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::enemy_update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying) // typed kill finishing fall
                        {
                            particles_spawn_kill_impact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);

                            //AE_ASSERT_MESG(false, "BRANCH: was_dying - SpawnDamageImpact");

                        }
                        else if (!was_damage_done) // fresh first frame of typed kill
                        {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::on_streak_reset();
                            HealthSystem::health_init(s_health);
                            spawn_normal_enemy("CAT");
                            break;

                        }
                        else // landed and dealt damage
                        {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);

                            //AE_ASSERT_MESG(false, "BRANCH: landed - SpawnKillImpact");

                            StreakSystem::on_streak_reset();
                            HealthSystem::health_init(s_health);
                            spawn_normal_enemy("CAT");
                            break;
                        }
                    }
                }
                if (!any_enemy_active() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 2: Enemy lands on purpose - no typing allowed
        case STAGE_KILL_BEFORE_LANDING:
            if (s_readyClicked && !s_stageStarted)
            {
                spawn_normal_enemy("OUCH");
                // Make it fall faster so the demo doesn't drag
                for (auto& e : s_pool)
                    if (e.enemy_is_active) { e.enemy_fall_speed_units_per_second = 200.0f; break; }
                s_stageStarted = true;
            }
            if (s_stageStarted)
            {
                update_enemies(dt, false);
                if (!any_enemy_active() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 3: Friendly NPC
        case STAGE_FRIENDLY:
            if (s_readyClicked && !s_stageStarted) { spawn_friendly_enemy("BIRD"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                typing_update_and_apply_to_enemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::enemy_update(e, s_enemyArea, dt, s_health);
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
                            particles_spawn_kill_impact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                            //  AE_ASSERT_MESG(false, "2");
                        }
                        else if (!was_damage_done) {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::on_streak_reset();
                            spawn_friendly_enemy("BIRD");
                            break;
                        }
                        else
                        {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::on_streak_reset();
                            spawn_friendly_enemy("BIRD");
                            break;
                        }
                    }
                }
                if (!any_enemy_active() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 4: Shielded enemy
        case STAGE_SHIELDED:
            if (s_readyClicked && !s_stageStarted) { spawn_shielded_enemy("ICE", "FIRE"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                typing_update_and_apply_to_enemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::enemy_update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying)
                            particles_spawn_kill_impact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                        else if (!was_damage_done) {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::on_streak_reset();
                            HealthSystem::health_init(s_health);
                            spawn_shielded_enemy("ICE", "FIRE");
                            break;
                        }
                        else
                        {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::on_streak_reset();
                            HealthSystem::health_init(s_health);
                            spawn_shielded_enemy("ICE", "FIRE");
                            break;
                        }
                    }
                }
                if (!any_enemy_active() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 5: Streak of 5 - staggered spawns
        case STAGE_STREAK:
        {
            if (s_readyClicked && !s_stageStarted) { s_stageStarted = true; typing_init(); }

            if (s_stageStarted)
            {
                if (!any_enemy_active() && s_stageTimer > 0.3f)
                {
                    spawn_streak_enemy(s_streakWords[s_streakWordIdx % 7]);
                    s_streakWordIdx++;
                    s_stageTimer = 0.0f;
                }

                typing_update_and_apply_to_enemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::enemy_update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying)
                            particles_spawn_kill_impact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                        else if (!was_damage_done) {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            HealthSystem::health_init(s_health);
                        }
                        else
                        {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            HealthSystem::health_init(s_health);
                        }
                    }
                }

                if (StreakSystem::get_streak() >= 5 && !any_enemy_active())
                    s_objectiveMet = true;
            }
            break;
        }

        // Stage 6: Orbit - show an enemy whose word orbits around it
        case STAGE_ORBIT:
            if (s_readyClicked && !s_stageStarted) { spawn_orbit_enemy("SPIN"); s_stageStarted = true; }
            if (s_stageStarted)
            {
                typing_update_and_apply_to_enemies(s_pool, s_score, s_health);
                for (int i = 0; i < (int)s_pool.size(); ++i)
                {
                    Data::Enemy& e = s_pool[(size_t)i];
                    if (!e.enemy_is_active) continue;
                    bool was_active = e.enemy_is_active;
                    bool was_dying = e.enemy_is_dying;
                    bool was_damage_done = e.enemy_did_damage;
                    EnemySystem::enemy_update(e, s_enemyArea, dt, s_health);
                    if (was_active && !e.enemy_is_active)
                    {
                        s_freeSlots.push_back(i);
                        if (was_dying)
                            particles_spawn_kill_impact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
                        else if (!was_damage_done) {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::on_streak_reset();
                            HealthSystem::health_init(s_health);
                            spawn_orbit_enemy("SPIN");
                            break;
                        }
                        else
                        {
                            particles_spawn_damage_impact(e.enemy_position_x, e.enemy_position_y);
                            StreakSystem::on_streak_reset();
                            HealthSystem::health_init(s_health);
                            spawn_orbit_enemy("SPIN");
                            break;
                        }
                    }
                }
                if (!any_enemy_active() && s_stageTimer > 0.5f)
                    s_objectiveMet = true;
            }
            break;

            // Stage 8: Power up - 3 sub-phases: Lightning, Shield, TimeSlow
        case STAGE_POWERUP:
            if (s_readyClicked && !s_stageStarted)
            {
                // Disable auto-spawning so random drops never steal slots
                PowerUpSystem::set_drop_spawn_interval(9999.0f, 9999.0f);
                PowerUpSystem::update(0.0f, s_pool, s_enemyArea, s_health);
                spawn_normal_enemy("FOE");
                PowerUpSystem::debug_spawn(PowerUpSystem::PowerType::LIGHTNING);
                s_stageStarted = true;
            }
            if (s_stageStarted)
            {
                float scaled_dt = dt * PowerUpSystem::get_time_scale();
                typing_update_and_apply_to_enemies(s_pool, s_score, s_health);
                update_enemies(scaled_dt, false);
                PowerUpSystem::update(dt, s_pool, s_enemyArea, s_health);

                // Phase 0: Lightning - advance once enemy is gone
                if (s_powerupPhase == 0)
                {
                    if (!any_enemy_active() && s_stageTimer > 1.0f)
                    {
                        s_powerupPhaseTimer += dt;
                        if (s_powerupPhaseTimer >= 1.5f)
                        {
                            s_powerupPhase = 1;
                            s_powerupPhaseTimer = 0.0f;
                            PowerUpSystem::debug_spawn(PowerUpSystem::PowerType::SHIELD);
                        }
                    }
                }
                // Phase 1: Shield - advance 3s after activation, 10s fallback
                else if (s_powerupPhase == 1)
                {
                    s_powerupSpawnTimer += dt;
                    if (PowerUpSystem::is_shield_active())
                        s_powerupPhaseTimer += dt;
                    if (s_powerupPhaseTimer >= 3.0f || s_powerupSpawnTimer >= 10.0f)
                    {
                        s_powerupPhase = 2;
                        s_powerupPhaseTimer = 0.0f;
                        s_powerupSpawnTimer = 0.0f;
                        spawn_normal_enemy("SLOW");
                        PowerUpSystem::debug_spawn(PowerUpSystem::PowerType::TIME_SLOW);
                    }
                }
                // Phase 2: TimeSlow - advance 3s after activation, 10s fallback
                else if (s_powerupPhase == 2)
                {
                    s_powerupSpawnTimer += dt;
                    if (PowerUpSystem::get_time_scale() < 1.0f)
                        s_powerupPhaseTimer += dt;
                    if (s_powerupPhaseTimer >= 3.0f || s_powerupSpawnTimer >= 10.0f)
                        s_objectiveMet = true;
                }
            }
            break;

            // Stage 8: Weather - trigger on READY
        case STAGE_WEATHER:
            if (s_readyClicked && !s_stageStarted) { weather_force_event(0, s_pool); s_stageStarted = true; }
            if (s_stageStarted)
            {
                weather_update(dt, s_pool);
                // Mark that freeze has started
                if (weather_is_player_frozen())
                    s_weatherFreezeStarted = true;
                // Only complete once freeze has started AND ended
                if (s_weatherFreezeStarted && !weather_is_player_frozen())
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
    if (!s_isPaused && s_objectiveMet && enter_pressed() && s_stage != STAGE_BOSS_WARNING)
    {
        start_stage((TutorialStage)(s_stage + 1));
    }

    if (!s_isPaused && s_stage == STAGE_BOSS_WARNING && s_objectiveMet && enter_pressed())
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
        mesh_renderer(s_bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
    }

    GameClouds::draw();

    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

    for (const auto& e : s_pool)
        EnemySystem::enemy_draw(s_meshes, e);
    typing_draw_enemy_words(s_pool);

    PowerUpSystem::draw();

    if (s_stage == STAGE_POWERUP && s_stageStarted && Graphics::g_font_id >= 0)
    {
        // Arrow only for Shield and TimeSlow phases
        const char* label = nullptr;
        float arrow_world_x = -s_winW * 0.5f + 140.0f + sinf(s_stageTimer * 5.0f) * 6.0f;
        float arrow_world_y = -s_winH * 0.5f + 35.0f;

        if (s_powerupPhase == 1 && PowerUpSystem::is_shield_active())
            label = "Shield! Immune to damage!";
        else if (s_powerupPhase == 2 && PowerUpSystem::get_time_scale() < 1.0f)
            label = "Time Slow! Enemies move slower!";

        if (label)
        {
            float aw, ah;
            AEGfxGetPrintSize(Graphics::g_font_id, "<", 0.55f, &aw, &ah);
            float ax = (arrow_world_x / (s_winW * 0.5f)) - aw * 0.5f;
            float ay = (arrow_world_y / (s_winH * 0.5f)) - ah * 0.5f;
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::g_font_id, "<", ax, ay, 0.55f, 0.9f, 0.85f, 0.3f, 1.0f);

            // Left-anchor label so it never overflows screen edge
            float lx = (-s_winW * 0.5f + 20.0f) / (s_winW * 0.5f);
            float ly = ((arrow_world_y + 45.0f) / (s_winH * 0.5f));
            AEGfxPrint(Graphics::g_font_id, label, lx, ly, 0.35f, 0.9f, 0.85f, 0.3f, 1.0f);
        }

        // Per-phase instruction drawn in speech bubble area (override message)
        const char* phase_msg = nullptr;
        if (s_powerupPhase == 0) phase_msg = "1/3 LIGHTNING: Activate\nthe drop to wipe all\nenemies instantly!";
        else if (s_powerupPhase == 1) phase_msg = "2/3 SHIELD: Activate\nto become immune\nto damage briefly!";
        else if (s_powerupPhase == 2) phase_msg = "3/3 TIME SLOW: Activate\nto slow all enemies\nand their words!";

        if (phase_msg && s_rectMesh)
        {
            float ndc_x = (BUBBLE_X - BUBBLE_W * 0.5f + 18.0f) / (s_winW * 0.5f);
            float ndc_y = (BUBBLE_Y + BUBBLE_H * 0.5f - 35.0f) / (s_winH * 0.5f);
            float spacing = 0.085f;
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            // Draw dark background over the existing message first
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetTransparency(0.92f);
            AEGfxSetColorToMultiply(0.08f, 0.08f, 0.12f, 1.0f);
            mesh_renderer(s_rectMesh, BUBBLE_X, BUBBLE_Y + 20.0f, BUBBLE_W - 4.0f, BUBBLE_H * 0.55f, 0.0f);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            std::string msg = phase_msg;
            float line_y = ndc_y;
            std::string line;
            for (size_t ci = 0; ci <= msg.size(); ++ci)
            {
                if (ci == msg.size() || msg[ci] == '\n')
                {
                    if (!line.empty())
                    {
                        AEGfxPrint(Graphics::g_font_id, line.c_str(), ndc_x, line_y, 0.42f, 1.0f, 1.0f, 1.0f, 1.0f);
                        line_y -= spacing;
                    }
                    line.clear();
                }
                else line += msg[ci];
            }
        }
    }
    weather_draw();
    particles_draw();
    laser_beam_draw();
    StreakSystem::draw();

    if (s_playerTex && s_playerMesh)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_playerTex, s_uvX, s_uvY);
        mesh_renderer(s_playerMesh, PLAYER_X, PLAYER_Y, PLAYER_SIZE, PLAYER_SIZE, 0.0f);
    }

    typing_render();
    UiSystem::ui_draw_score_and_typed(s_score);
    if (s_stage == STAGE_KILL_BEFORE_LANDING || s_stage == STAGE_FRIENDLY)
        UiSystem::ui_draw_health_bar(s_health, s_meshes, s_heartAnim, dt);

    draw_speech_bubble(s_objectiveMet);

    bool isDead = false;
    UiPauseMenu::check_paused(s_isPaused, isDead, s_rectMesh);

    // --- Audio Pause Tracking ---
    if (s_isPaused && !s_wasPaused) {
        SoundManager::pause_gameplay_sounds();
    }
    else if (!s_isPaused && s_wasPaused) {
        SoundManager::resume_gameplay_sounds();
    }
    s_wasPaused = s_isPaused;
}

// shuts down every subsystem initialised by Tutorial_Init in reverse order -
// exits Typing, PowerUpSystem, Weather, Particles, Streaks, parallax, clouds,
// and LaserBeam; frees the enemy pool via EnemyManage; unloads all textures
// and meshes (bg, player, rect); resets the camera to the origin
void tutorial_exit()
{
    typing_exit();
    PowerUpSystem::free();
    weather_exit();
    particles_free();
    StreakSystem::cleanup();
    BackgroundParallax::menu_parallax_exit();
    GameClouds::cleanup();
    laser_beam_free();

    EnemyManage::enemy_manage_free(s_meshes, s_pool, s_freeSlots);

    if (s_bgTex) { AEGfxTextureUnload(s_bgTex);     s_bgTex = nullptr; }
    if (s_bgMesh) { AEGfxMeshFree(s_bgMesh);          s_bgMesh = nullptr; }
    if (s_playerTex) { AEGfxTextureUnload(s_playerTex);  s_playerTex = nullptr; }
    if (s_playerMesh) { AEGfxMeshFree(s_playerMesh);      s_playerMesh = nullptr; }
    if (s_rectMesh) { AEGfxMeshFree(s_rectMesh);        s_rectMesh = nullptr; }

    AEGfxSetCamPosition(0.0f, 0.0f);
}