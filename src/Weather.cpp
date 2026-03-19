/*
@file       Weather.cpp
@brief      Weather system implementation.

            Implements the Weather interface declared in Weather.h.

            Snow event flow:
              IDLE  ->  (random timer expires)  ->  FREEZING  ->  FROZEN  ->  THAWING  ->  IDLE
                        blizzard ramps up              player             overlay fades out
                                                    spams SPACE

            Ambient snowflakes (pool of AMBIENT_COUNT) always fall in IDLE/FREEZING/FROZEN/THAWING.
            During FREEZING and FROZEN an extra blizzard overlay and heavier particle burst appear.

@author     (your name)
*/
#include "pch.h"
#include "Weather.h"


// ============================================================
//  Internal constants
// ============================================================
namespace
{
    // Screen dimensions
    constexpr float SCREEN_W = 1600.0f;
    constexpr float SCREEN_H = 900.0f;
    constexpr float HALF_W = 800.0f;
    constexpr float HALF_H = 450.0f;

    // Trigger timing 
    constexpr float TRIGGER_MIN_SEC = 25.0f;   // earliest
    constexpr float TRIGGER_MAX_SEC = 30.0f;   // latest

    // Phase durations
    constexpr float FREEZE_IN_DUR = 1.8f;
    constexpr float THAW_DUR = 1.2f;

    // Spacebar break free mechanic
    constexpr int   SPACES_NEEDED = 15;

    // Particle counts
    constexpr int   AMBIENT_COUNT = 80;
    constexpr int   BLIZZARD_COUNT = 140;

    // Particle speed range
    constexpr float AMB_SPEED_MIN = 60.0f;
    constexpr float AMB_SPEED_MAX = 160.0f;

    // Snow blizzard particle speed range
    constexpr float BLZ_SPEED_MIN = 180.0f;
    constexpr float BLZ_SPEED_MAX = 380.0f;

    // Particle size range
    constexpr float FLAKE_SIZE_MIN = 3.0f;
    constexpr float FLAKE_SIZE_MAX = 10.0f;

    // Horizontal drift
    constexpr float DRIFT_RANGE = 30.0f;   // max horizontal speed

    // Rain particle constants
    constexpr int   RAIN_COUNT = 200;      // number of raindrops
    constexpr float RAIN_SPEED_MIN = 500.0f;   // downward fall 
    constexpr float RAIN_SPEED_MAX = 800.0f;
    constexpr float RAIN_DRIFT = 80.0f;    // -ve for left, +ve for right
    constexpr float RAIN_W_MIN = 2.0f;     // width
    constexpr float RAIN_W_MAX = 3.0f;
    constexpr float RAIN_H_MIN = 14.0f;    // height
    constexpr float RAIN_H_MAX = 24.0f;

    // Overlay (RGB, Alpha)
    constexpr float OVL_R = 0.55f;
    constexpr float OVL_G = 0.75f;
    constexpr float OVL_B = 1.00f;
    constexpr float OVL_ALPHA_MAX = 0.55f;

    // Progress bar dimensions (snowy)
    constexpr float BAR_W = 400.0f;
    constexpr float BAR_H = 28.0f;
    constexpr float BAR_Y = -250.0f;

    // Flash when a space is pressed
    constexpr float FLASH_DUR = 0.08f;

    // ================================================================
    //  Thunderstorm constants
    // ================================================================

    constexpr float STORM_DURATION = 20.0f;

    constexpr float BOLT_INTERVAL = 5.0f; // lightning strike

    constexpr float BRACE_WARNING_DUR = 1.0f; // BRACE! warning shown x seconds before the strike

    constexpr float STRIKE_FLASH_DUR = 0.15f; // White flash duration on strike

    constexpr float STUN_DUR = 0.5f; // Input stun duration on strike

    constexpr float LUNGE_DUR = 2.5f; // How long the struck enemy falls at boosted speed 

    constexpr float LUNGE_SPEED_MUL = 3.5f;

    constexpr float STORM_OVL_ALPHA_MAX = 0.25f;

    // Storm overlay colour (dark grey)
    constexpr float STORM_OVL_R = 0.10f;
    constexpr float STORM_OVL_G = 0.08f;
    constexpr float STORM_OVL_B = 0.18f;

    // Thunderstorm sprite animation (thunderstorm.png — 4 frames horizontal)
    constexpr int   TSTRIKE_FRAME_COUNT = 4;
    constexpr float TSTRIKE_FRAME_TIME = 0.08f;   // ~12 fps, snappy
    constexpr float TSTRIKE_UV_W = 1.0f / (float)TSTRIKE_FRAME_COUNT;
    constexpr float TSTRIKE_DISPLAY_W = 120.0f;
    constexpr float TSTRIKE_DISPLAY_H = 200.0f;

    struct TStrikeAnim
    {
        bool  active = false;
        float x = 0.0f;
        float y = 0.0f;
        int   frame = 0;
        float frameTimer = 0.0f;
    };

    static TStrikeAnim      s_tStrike{};
    static AEGfxTexture* s_tStrikeTex = nullptr;
    static AEGfxVertexList* s_tStrikeMesh = nullptr;   // single-frame-width UV quad

    // ================================================================
    //  Rainbow constants
    // ================================================================
    constexpr float RAINBOW_DURATION = 15.0f;   // total event length
    constexpr float RAINBOW_POWERUP_INTERVAL = 3.0f;  // spawn a power-up every N seconds
    constexpr int   RAINBOW_FRAME_COUNT = 9;        // frames in the sprite sheet
    constexpr float RAINBOW_FRAME_TIME = 0.12f;   // seconds per frame (~8 fps)
    constexpr float RAINBOW_DISPLAY_W = 1600.0f; // stretch to full screen width
    // Height derived from frame aspect ratio: each frame is a square slice of the sheet,
    // so display height = display_width / frame_count to preserve pixel proportions.
    // e.g. if the sheet is 900x100 px (9 frames x 100px wide, 100px tall) each frame is square.
    // We want it to fill the top ~1/5 of the screen without covering the play area.
    constexpr float RAINBOW_DISPLAY_H = 80.0f;
    constexpr float RAINBOW_Y = 250.0f;          // upper sky area, well above enemies
    constexpr float RAINBOW_FADE_TIME = 1.5f;    // fade-in / fade-out duration

    // ================================================================
    //  Ghost weather constants
    // ================================================================
    constexpr int   GHOST_COUNT = 5;
    constexpr float GHOST_LIFETIME = 10.0f;   // seconds before lock triggers
    constexpr float GHOST_LOCK_DUR = 3.0f;    // typing locked duration
    constexpr float GHOST_SPEED_MIN = 80.0f;
    constexpr float GHOST_SPEED_MAX = 160.0f;
    constexpr float GHOST_SIZE = 120.0f;   // display size (px)
    // Sprite sheet: 16 frames in one row (walk cycle)
    constexpr int   GHOST_FRAME_COUNT = 16;
    constexpr float GHOST_FRAME_TIME = 0.07f;   // seconds per frame
    constexpr float GHOST_LABEL_OFFSET_Y = 38.0f;   // px above ghost centre for digit label
    constexpr float GHOST_SPAWN_MARGIN = 80.0f;   // off-screen spawn margin

    // Inline helpers
    static inline float RandF(float lo, float hi)
    {
        return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX); // lower + (higher - lower) * cast to float (random num / random max num)
    }
    static inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); } // if less than 0, clamp to 0 else if more than 1, clamp to 1
}

// ============================================================
//  Data types
// ============================================================
namespace
{
    // Which weather event is currently active
    enum class WeatherType { SNOW, THUNDER, RAINBOW, GHOST };

    // Weather state machine (shared by both types)
    enum class WeatherState
    {
        IDLE,       // no event active
        FREEZING,   // blizzard building up (FREEZE_IN_DUR seconds)
        FROZEN,     // player is fully frozen
        THAWING     // event over, overlay fading out
    };

    // Thunderstrom specific bolt phases 
    enum class BoltPhase
    {
        WAITING,    // counting down to next bolt
        WARNING,    // BRACE! shown
        STRIKING,   // white flash + stun active
    };

    // Snow particle
    struct Snowflake { float x, y, speedY, driftX, size, alpha; bool isBlizzard; };

    // Rain particle
    struct Raindrop { float x, y, speedY, w, h, alpha; };

    // Ghost particle (ghost weather)
    struct Ghost
    {
        float x, y;          // world-space position
        float velX, velY;    // velocity (px/s)
        int   digit;         // 5–9 assigned digit
        bool  alive;         // false once killed by player
        int   frame;         // current animation frame
        float frameTimer;    // accumulator for frame advance
    };

    // Shared state
    static WeatherState s_state = WeatherState::IDLE; // curent state, changes based on Update()
    static WeatherType  s_activeType = WeatherType::SNOW; // placeholder, activate state will change based on Update()
    static float        s_triggerTimer = 0.0f;
    static float        s_triggerTarget = 0.0f; // when triggertimer > triggertarget, fire weather event
    static float        s_phaseTimer = 0.0f; // timer resets to 0 when entering new phase, used to check how long each phase lasts

    // Snow state
    static int   s_spacePresses = 0;
    static float s_flashTimer = 0.0f;
    static bool  s_spaceWasHeld = false; // prevent player from holding space

    // Thunderstorm state
    static float     s_stormTimer = 0.0f;    // total time remaining in storm
    static float     s_boltTimer = 0.0f;    // time until next bolt
    static BoltPhase s_boltPhase = BoltPhase::WAITING;
    static float     s_boltPhaseTimer = 0.0f;    // timer within current bolt phase
    static float     s_stunTimer = 0.0f;    // input stun remaining
    static float     s_strikeFlash = 0.0f;
    static int       s_lungeEnemyIdx = -1;       // index of enemy currently lunging, sentinel val of -1 indicate no enemy lunging
    static float     s_lungeTimer = 0.0f;    // lunge boost duration remaining
    static float     s_lungeOrigSpeed = 0.0f;    // original speed before lunge boost

    // Rainbow state
    static float            s_rainbowTimer = 0.0f;  // seconds remaining in event
    static float            s_rainbowPowerupTimer = 0.0f;  // countdown to next power-up spawn
    static int              s_rainbowFrame = 0;     // current animation frame
    static float            s_rainbowFrameTimer = 0.0f;  // time accumulator for frame advance
    static AEGfxTexture* s_rainbowTex = nullptr;
    static AEGfxVertexList* s_rainbowMesh = nullptr; // single-frame UV quad

    // Running pulse timer (always increments, used for sine-wave text pulsing)
    static float            s_weatherPulseTimer = 0.0f;

    // Short popup timers: text shows briefly on event start then disappears
    constexpr float WEATHER_POPUP_DUR = 1.8f;
    constexpr float WEATHER_POPUP_FADE = 0.5f;
    static float            s_rainbowPopupTimer = 0.0f;
    static float            s_ghostPopupTimer = 0.0f;

    // Ghost weather state
    static Ghost            s_ghosts[GHOST_COUNT]{};
    static float            s_ghostLifeTimer = 0.0f; // counts up; triggers lock at GHOST_LIFETIME
    static float            s_ghostLockTimer = 0.0f; // counts down; >0 means typing locked
    static bool             s_ghostEventActive = false;
    static AEGfxTexture* s_ghostTex = nullptr;
    static AEGfxVertexList* s_ghostMesh = nullptr;   // single-frame UV quad (1/GHOST_FRAME_COUNT wide)

    // ---- Particles ----
    static Snowflake s_ambient[AMBIENT_COUNT]{};
    static Snowflake s_blizzard[BLIZZARD_COUNT]{};
    static Raindrop  s_rain[RAIN_COUNT]{};

    // Shared quad mesh
    static AEGfxVertexList* s_quad = nullptr;
}


// ============================================================
//  Internal helpers (forward declarations)
// ============================================================
namespace
{
    static void  ResetTriggerTimer();
    static void  SpawnAmbientFlake(Snowflake& f);
    static void  SpawnBlizzardFlake(Snowflake& f);
    static void  UpdateFlake(Snowflake& f, float dt, float speedMul);
    static float GetBlizzardIntensity();
    static void  DrawFullscreenOverlay(float r, float g, float b, float alpha); // shared overlay helper
    static void  Snow_DrawParticles(float blizzardIntensity);
    static void  DrawFreezeHUD();
    static void  DrawProgressBar();

    // Rain
    static void  SpawnRaindrop(Raindrop& r);
    static void  UpdateRaindrop(Raindrop& r, float dt);
    static void  Thunder_DrawRain();

    // Thunder helpers
    static void  Thunder_StartStorm();
    static void  Thunder_Update(float dt, std::vector<Data::Enemy>& enemy_pool);
    static void  Thunder_FireBolt(std::vector<Data::Enemy>& enemy_pool);
    static void  Thunder_RestoreLunge(std::vector<Data::Enemy>& enemy_pool); // require enemy vector to strike random enemy
    static void  Thunder_DrawHUD();
    static void  Thunder_DrawStrikeFlash();
    static void  Thunder_DrawStrikeAnim();

    // Rainbow helpers
    static void  Rainbow_BuildMesh();
    static void  Rainbow_Start();
    static void  Rainbow_Update(float dt);
    static void  Rainbow_DrawSprite();
    static void  Rainbow_DrawHUD();

    // Ghost weather helpers
    static void  Ghost_BuildMesh();
    static void  Ghost_Start();
    static void  Ghost_Update(float dt);
    static void  Ghost_Draw();
    static void  Ghost_DrawHUD();
    static void  Ghost_SpawnOne(Ghost& g, int digit);

    static void  DrawCentredText(const char* text, float ndcY, float scale,
        float r, float g, float b, float alpha);
}

void Weather_Init()
{
    if (s_quad) { AEGfxMeshFree(s_quad); s_quad = nullptr; }
    s_quad = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

    // Shared reset
    s_state = WeatherState::IDLE;
    s_phaseTimer = 0.0f;

    // Snow reset
    s_spacePresses = 0;
    s_flashTimer = 0.0f;
    s_spaceWasHeld = false;

    // Thunder reset
    s_stormTimer = 0.0f;
    s_boltTimer = 0.0f;
    s_boltPhase = BoltPhase::WAITING;
    s_boltPhaseTimer = 0.0f;
    s_stunTimer = 0.0f;
    s_strikeFlash = 0.0f;
    s_lungeEnemyIdx = -1;
    s_lungeTimer = 0.0f;
    s_lungeOrigSpeed = 0.0f;

    // Rainbow reset – free then reload to avoid double-load on re-init
    if (s_rainbowTex) { AEGfxTextureUnload(s_rainbowTex);  s_rainbowTex = nullptr; }
    if (s_rainbowMesh) { AEGfxMeshFree(s_rainbowMesh);      s_rainbowMesh = nullptr; }
    s_rainbowTimer = 0.0f;
    s_rainbowPowerupTimer = 0.0f;
    s_rainbowFrame = 0;
    s_rainbowFrameTimer = 0.0f;
    s_rainbowTex = AEGfxTextureLoad("Assets/rainbowvfx.png");
    Rainbow_BuildMesh();

    // Thunderstorm strike animation
    if (s_tStrikeTex) { AEGfxTextureUnload(s_tStrikeTex);  s_tStrikeTex = nullptr; }
    if (s_tStrikeMesh) { AEGfxMeshFree(s_tStrikeMesh);      s_tStrikeMesh = nullptr; }
    s_tStrikeTex = AEGfxTextureLoad("Assets/thunderstorm.png");
    s_tStrikeMesh = Sprite_Mesh(TSTRIKE_UV_W, 1.0f);
    s_tStrike = {};
    s_weatherPulseTimer = 0.0f;
    s_rainbowPopupTimer = 0.0f;
    s_ghostPopupTimer = 0.0f;

    // Ghost weather init
    s_ghostTex = AEGfxTextureLoad("Assets/ghost.png");
    Ghost_BuildMesh();
    s_ghostEventActive = false;
    s_ghostLifeTimer = 0.0f;
    s_ghostLockTimer = 0.0f;
    for (int i = 0; i < GHOST_COUNT; ++i) s_ghosts[i].alive = false;

    ResetTriggerTimer();

    for (int i = 0; i < AMBIENT_COUNT; ++i)
    {
        SpawnAmbientFlake(s_ambient[i]);
        s_ambient[i].y = RandF(-HALF_H, HALF_H);  // scatter on startup
    }

    for (int i = 0; i < BLIZZARD_COUNT; ++i)
        SpawnBlizzardFlake(s_blizzard[i]);

    // Scatter rain
    for (int i = 0; i < RAIN_COUNT; ++i)
    {
        SpawnRaindrop(s_rain[i]);
        s_rain[i].y = RandF(-HALF_H, HALF_H);  // scatter vertically
    }
}

void Weather_Update(float dt, std::vector<Data::Enemy>& enemy_pool)
{
    // Always tick the pulse timer (used for sine-wave text effects)
    s_weatherPulseTimer += dt;
    if (s_rainbowPopupTimer > 0.0f) { s_rainbowPopupTimer -= dt; if (s_rainbowPopupTimer < 0.0f) s_rainbowPopupTimer = 0.0f; }
    if (s_ghostPopupTimer > 0.0f) { s_ghostPopupTimer -= dt; if (s_ghostPopupTimer < 0.0f) s_ghostPopupTimer = 0.0f; }

    // Advance thunderstorm on-enemy strike animation
    if (s_tStrike.active)
    {
        s_tStrike.frameTimer += dt;
        if (s_tStrike.frameTimer >= TSTRIKE_FRAME_TIME)
        {
            s_tStrike.frameTimer -= TSTRIKE_FRAME_TIME;
            ++s_tStrike.frame;
            if (s_tStrike.frame >= TSTRIKE_FRAME_COUNT)
                s_tStrike.active = false;
        }
    }

    // Advance trigger timer when no weather active
    if (s_state == WeatherState::IDLE)
    {
        s_triggerTimer += dt;
        if (s_triggerTimer >= s_triggerTarget)
        {
            // Weighted pick: Snow 30%, Thunder 30%, Ghost 30%, Rainbow 10%
            int pick = rand() % 10;
            if (pick < 3) s_activeType = WeatherType::SNOW;
            else if (pick < 6) s_activeType = WeatherType::THUNDER;
            else if (pick < 9) s_activeType = WeatherType::GHOST;
            else               s_activeType = WeatherType::RAINBOW;

            if (s_activeType == WeatherType::SNOW)
            {
                s_state = WeatherState::FREEZING;
                s_phaseTimer = 0.0f;
                s_spacePresses = 0;

                SoundManager::PlaySnowAmbient();
            }
            else if (s_activeType == WeatherType::THUNDER)
            {
                Thunder_StartStorm();
            }
            else if (s_activeType == WeatherType::RAINBOW)
            {
                Rainbow_Start();
            }
            else
            {
                Ghost_Start();
            }
        }
    }

    // ================================================================
    //  SNOW path
    // ================================================================
    if (s_activeType == WeatherType::SNOW)
    {
        // FREEZING phase
        if (s_state == WeatherState::FREEZING)
        {
            s_phaseTimer += dt;
            if (s_phaseTimer >= FREEZE_IN_DUR) { s_state = WeatherState::FROZEN; s_phaseTimer = 0.0f; }
        }

        // FROZEN phase count space presses
        if (s_state == WeatherState::FROZEN)
        {
            bool spaceHeld = (AEInputCheckCurr(AEVK_SPACE) != 0); // is space pressed this frame?
            bool spaceEdge = spaceHeld && !s_spaceWasHeld; // true only if it is pressed this frame and not pressed last frame
            s_spaceWasHeld = spaceHeld; // track last frame

            if (spaceEdge)
            {
                ++s_spacePresses;
                s_flashTimer = FLASH_DUR;
                if (s_spacePresses >= SPACES_NEEDED) { s_state = WeatherState::THAWING; s_phaseTimer = 0.0f; SoundManager::StopSnowAmbient(); }
            }
            if (s_flashTimer > 0.0f) s_flashTimer -= dt;
        }

        // THAWING phase
        if (s_state == WeatherState::THAWING)
        {
            s_phaseTimer += dt;
            if (s_phaseTimer >= THAW_DUR) { s_state = WeatherState::IDLE; ResetTriggerTimer(); }
        }

        // Update snow particles
        for (int i = 0; i < AMBIENT_COUNT; ++i) UpdateFlake(s_ambient[i], dt, 1.0f);
        float blzMul = 0.3f + GetBlizzardIntensity() * 1.4f;
        for (int i = 0; i < BLIZZARD_COUNT; ++i) UpdateFlake(s_blizzard[i], dt, blzMul);
    }

    // ================================================================
    //  THUNDER path
    // ================================================================
    if (s_activeType == WeatherType::THUNDER && s_state != WeatherState::IDLE)
    {
        Thunder_Update(dt, enemy_pool);
        for (int i = 0; i < RAIN_COUNT; ++i) UpdateRaindrop(s_rain[i], dt);
    }

    // ================================================================
    //  RAINBOW path
    // ================================================================
    if (s_activeType == WeatherType::RAINBOW && s_state != WeatherState::IDLE)
    {
        Rainbow_Update(dt);
    }

    // ================================================================
    //  GHOST path
    // ================================================================
    if (s_activeType == WeatherType::GHOST && s_ghostEventActive)
    {
        Ghost_Update(dt);
    }
}

// ----------------------------------------------------------------
void Weather_DrawBackground()
{
    // Rainbow is drawn BEFORE enemies so it sits behind them in the scene
    if (s_activeType == WeatherType::RAINBOW && s_state != WeatherState::IDLE)
        Rainbow_DrawSprite();
}

// ----------------------------------------------------------------
void Weather_Draw()
{
    // Ghost weather has its own active flag, not bound to the shared state machine
    bool ghostVisible = (s_activeType == WeatherType::GHOST && s_ghostEventActive);
    if (!s_quad || (s_state == WeatherState::IDLE && !ghostVisible)) return;

    if (s_activeType == WeatherType::SNOW)
    {
        float intensity = GetBlizzardIntensity();
        Snow_DrawParticles(intensity);
        DrawFullscreenOverlay(OVL_R, OVL_G, OVL_B, OVL_ALPHA_MAX * intensity);
        if (s_state == WeatherState::FROZEN) { DrawFreezeHUD(); DrawProgressBar(); }
    }
    else if (s_activeType == WeatherType::THUNDER)
    {
        Thunder_DrawRain();
        // Dark stormy overlay fades in over first 2 seconds of storm
        float elapsed = STORM_DURATION - s_stormTimer;
        DrawFullscreenOverlay(STORM_OVL_R, STORM_OVL_G, STORM_OVL_B,
            STORM_OVL_ALPHA_MAX * Clamp01(elapsed / 2.0f));
        Thunder_DrawStrikeAnim();
        Thunder_DrawHUD();
        Thunder_DrawStrikeFlash();
    }
    // RAINBOW: sprite drawn in Weather_DrawBackground() — HUD drawn here
    if (s_activeType == WeatherType::RAINBOW && s_state != WeatherState::IDLE)
    {
        Rainbow_DrawHUD();
    }
    // GHOST: ghosts + HUD drawn here
    if (s_activeType == WeatherType::GHOST && s_ghostEventActive)
    {
        Ghost_Draw();
        Ghost_DrawHUD();
    }
}

void Weather_Exit()
{
    SoundManager::StopSnowAmbient();
    SoundManager::StopRainAmbient();
    if (s_quad) { AEGfxMeshFree(s_quad);             s_quad = nullptr; }
    if (s_rainbowTex) { AEGfxTextureUnload(s_rainbowTex);  s_rainbowTex = nullptr; }
    if (s_rainbowMesh) { AEGfxMeshFree(s_rainbowMesh);      s_rainbowMesh = nullptr; }
    if (s_tStrikeTex) { AEGfxTextureUnload(s_tStrikeTex);  s_tStrikeTex = nullptr; }
    if (s_tStrikeMesh) { AEGfxMeshFree(s_tStrikeMesh);      s_tStrikeMesh = nullptr; }
    if (s_ghostTex) { AEGfxTextureUnload(s_ghostTex);    s_ghostTex = nullptr; }
    if (s_ghostMesh) { AEGfxMeshFree(s_ghostMesh);        s_ghostMesh = nullptr; }
}

// Functions used in game.cpp to prevent certain game mechanics
bool Weather_IsPlayerFrozen() { return s_state == WeatherState::FROZEN; }
bool Weather_IsPlayerStunned() { return (s_activeType == WeatherType::THUNDER && s_stunTimer > 0.0f); }
bool Weather_IsTypingLocked() { return (s_activeType == WeatherType::GHOST && s_ghostLockTimer > 0.0f); }

// Used in debugconsole
void Weather_ForceEvent(int type, std::vector<Data::Enemy>& enemy_pool)
{
    // Restore any active lunge before cancelling (prevents permanent speed boost)
    Thunder_RestoreLunge(enemy_pool);
    SoundManager::StopSnowAmbient();
    SoundManager::StopRainAmbient();
    // Cancel any active event first, reset to a clean IDLE state
    s_state = WeatherState::IDLE;
    s_phaseTimer = 0.0f;
    s_spacePresses = 0;
    s_flashTimer = 0.0f;
    s_spaceWasHeld = false;
    s_stunTimer = 0.0f;
    s_strikeFlash = 0.0f;
    s_tStrike = {};
    // Ghost reset
    s_ghostEventActive = false;
    s_ghostLifeTimer = 0.0f;
    s_ghostLockTimer = 0.0f;
    s_rainbowPopupTimer = 0.0f;
    s_ghostPopupTimer = 0.0f;
    for (int i = 0; i < GHOST_COUNT; ++i) s_ghosts[i].alive = false;

    if (type == 0)
    {
        // Force snow blizzard, skip IDLE, jump straight to FREEZING
        s_activeType = WeatherType::SNOW;
        s_state = WeatherState::FREEZING;
        s_phaseTimer = 0.0f;
        SoundManager::PlaySnowAmbient();
    }
    else if (type == 1)
    {
        // Force thunderstorm
        s_activeType = WeatherType::THUNDER;
        Thunder_StartStorm();
    }
    else if (type == 2)
    {
        // Force rainbow
        s_activeType = WeatherType::RAINBOW;
        Rainbow_Start();
    }
    else
    {
        // Force ghost weather
        s_activeType = WeatherType::GHOST;
        Ghost_Start();
    }

    // Reset the trigger timer so the next random event fires on schedule after this one
    ResetTriggerTimer();
}


// ============================================================
//  Internal helper implementations
// ============================================================
namespace
{
    // Pick a new random interval and reset the countdown
    static void ResetTriggerTimer()
    {
        s_triggerTimer = 0.0f;
        s_triggerTarget = RandF(TRIGGER_MIN_SEC, TRIGGER_MAX_SEC);
    }

    // Initialise / respawn one ambient flake above the screen at a random X
    static void SpawnAmbientFlake(Snowflake& f)
    {
        f.x = RandF(-HALF_W, HALF_W);
        f.y = HALF_H + RandF(0.0f, 60.0f);    // just off-screen top
        f.speedY = RandF(AMB_SPEED_MIN, AMB_SPEED_MAX);
        f.driftX = RandF(-DRIFT_RANGE, DRIFT_RANGE);
        f.size = RandF(FLAKE_SIZE_MIN, FLAKE_SIZE_MAX);
        f.alpha = RandF(0.4f, 0.85f);
        f.isBlizzard = false;
    }

    // Initialise / respawn one blizzard flake (used only during events)
    static void SpawnBlizzardFlake(Snowflake& f)
    {
        f.x = RandF(-HALF_W, HALF_W);
        f.y = HALF_H + RandF(0.0f, 80.0f);
        f.speedY = RandF(BLZ_SPEED_MIN, BLZ_SPEED_MAX);
        f.driftX = RandF(-DRIFT_RANGE * 2.0f, DRIFT_RANGE * 2.0f);
        f.size = RandF(FLAKE_SIZE_MIN, FLAKE_SIZE_MAX - 1.0f);
        f.alpha = RandF(0.5f, 1.0f);
        f.isBlizzard = true;
    }

    // Move one flake, respawn when it exits the bottom
    static void UpdateFlake(Snowflake& f, float dt, float speedMul)
    {
        f.y -= f.speedY * speedMul * dt;
        f.x += f.driftX * speedMul * dt;

        // Wrap horizontally so flakes dont disappear off the side of screen
        if (f.x < -HALF_W) f.x += SCREEN_W;
        if (f.x > HALF_W) f.x -= SCREEN_W;

        // Respawn when below the screen
        if (f.y < -HALF_H - 20.0f)
            f.isBlizzard ? SpawnBlizzardFlake(f) : SpawnAmbientFlake(f);
    }

    // Returns a 0..1 intensity for the blizzard based on state + phaseTimer
    static float GetBlizzardIntensity()
    {
        // Thunder doesnt use snow blizzard particles
        if (s_activeType == WeatherType::THUNDER) return 0.0f;

        switch (s_state)
        {
        case WeatherState::IDLE:     return 0.0f;
        case WeatherState::FREEZING: return Clamp01(s_phaseTimer / FREEZE_IN_DUR);
        case WeatherState::FROZEN:   return 1.0f + (s_flashTimer > 0.0f ? 0.15f * (s_flashTimer / FLASH_DUR) : 0.0f);
        case WeatherState::THAWING:  return Clamp01(1.0f - s_phaseTimer / THAW_DUR);
        }
        return 0.0f;
    }

    // Shared helper draws a full screen coloured overlay quad at the given alpha
    static void DrawFullscreenOverlay(float r, float g, float b, float alpha)
    {
        if (alpha <= 0.001f) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, 0.0f, SCREEN_W, SCREEN_H, 0.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(1.0f);
    }

    // Draw snow particles (ambient + blizzard burst) snow weather only
    static void Snow_DrawParticles(float blizzardIntensity)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_ADD);   // bright, glowy snow

        // Ambient snowflakes (always visible)
        for (int i = 0; i < AMBIENT_COUNT; ++i)
        {
            const Snowflake& f = s_ambient[i];
            AEGfxSetTransparency(f.alpha);
            AEGfxSetColorToMultiply(0.85f, 0.93f, 1.0f, 1.0f);   // slight blue-white tint
            Mesh_Renderer(s_quad, f.x, f.y, f.size, f.size, 0.0f);
        }

        // Blizzard particles fade in with intensity
        if (blizzardIntensity > 0.01f)
        {
            for (int i = 0; i < BLIZZARD_COUNT; ++i)
            {
                const Snowflake& f = s_blizzard[i];
                AEGfxSetTransparency(f.alpha * blizzardIntensity);
                AEGfxSetColorToMultiply(0.8f, 0.92f, 1.0f, 1.0f);
                Mesh_Renderer(s_quad, f.x, f.y, f.size, f.size, 0.0f);
            }
        }

        // Reset render state
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Shared HUD text helper measures, centres, and prints one line
    static void DrawCentredText(const char* text, float ndcY, float scale,
        float r, float g, float b, float alpha)
    {
        if (Graphics::font_id < 0) return; // check for valid font
        float w, h;
        AEGfxGetPrintSize(Graphics::font_id, text, scale, &w, &h);
        AEGfxPrint(Graphics::font_id, text, -w * 0.5f, ndcY, scale, r, g, b, alpha);
    }

    // "SPAM SPACE TO BREAK FREE!" prompt text
    static void DrawFreezeHUD()
    {
        if (Graphics::font_id < 0) return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        // Main "FROZEN" title icy blue
        DrawCentredText("YOU ARE FROZEN!", 0.35f, 0.75f, 0.6f, 0.88f, 1.0f, 1.0f);
        // Instruction – white
        DrawCentredText("SPAM SPACE TO BREAK FREE!", 0.15f, 0.55f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Horizontal progress bar showing breakfree progress
    static void DrawProgressBar()
    {
        float progress = Clamp01((float)s_spacePresses / (float)SPACES_NEEDED);
        constexpr float barBorder = 4.0f;  // border thickness (px each side)

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Background (dark, slightly blue)
        AEGfxSetTransparency(0.85f);
        AEGfxSetColorToMultiply(0.1f, 0.15f, 0.25f, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, BAR_Y, BAR_W + barBorder * 2.0f, BAR_H + barBorder * 2.0f, 0.0f);

        // Bar fill
        if (progress > 0.001f)
        {
            float fillW = (BAR_W - barBorder * 2.0f) * progress;
            float fillStartX = -(BAR_W * 0.5f) + barBorder + fillW * 0.5f;

            // Lerp colour from ice blue (0% warm) to warm orange (100% warm)
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.3f + 0.7f * progress,
                0.5f + 0.1f * progress,
                1.0f - 0.8f * progress, 1.0f);
            Mesh_Renderer(s_quad, fillStartX, BAR_Y, fillW, BAR_H - barBorder * 2.0f, 0.0f);
        }

        // --- Label above bar ---
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        float barYndc = (BAR_Y + (BAR_H * 0.5f) + 20.0f) / HALF_H;
        DrawCentredText("WARMTH", barYndc, 0.4f, 0.9f, 0.95f, 1.0f, 1.0f);

        // Reset
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // ================================================================
    //  Rain helpers
    // ================================================================

    static void SpawnRaindrop(Raindrop& r)
    {
        r.x = RandF(-HALF_W, HALF_W);
        r.y = HALF_H + RandF(0.0f, 80.0f);   // just off-screen top
        r.speedY = RandF(RAIN_SPEED_MIN, RAIN_SPEED_MAX);
        r.w = RandF(RAIN_W_MIN, RAIN_W_MAX);
        r.h = RandF(RAIN_H_MIN, RAIN_H_MAX);
        r.alpha = RandF(0.35f, 0.75f);
    }

    static void UpdateRaindrop(Raindrop& r, float dt)
    {
        r.y -= r.speedY * dt;
        r.x += RAIN_DRIFT * dt;   // wind-driven diagonal

        // Wrap horizontally
        if (r.x > HALF_W) r.x -= SCREEN_W;

        // Respawn when below screen
        if (r.y < -HALF_H - 30.0f) SpawnRaindrop(r);
    }

    static void Thunder_DrawRain()
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        for (int i = 0; i < RAIN_COUNT; ++i)
        {
            const Raindrop& r = s_rain[i];
            AEGfxSetTransparency(r.alpha);
            AEGfxSetColorToMultiply(0.6f, 0.75f, 0.95f, 1.0f);   // cool blue-grey
            Mesh_Renderer(s_quad, r.x, r.y, r.w, r.h, 0.0f);
        }

        // Reset
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // ================================================================
    //  Thunder helpers
    // ================================================================

    // Kick off a new thunder storm session
    static void Thunder_StartStorm()
    {
        s_state = WeatherState::FREEZING;  // reuse FREEZING as "storm active" sentinel
        s_stormTimer = STORM_DURATION;
        s_boltTimer = BOLT_INTERVAL * 0.4f;    // first bolt comes faster to grab attention
        s_boltPhase = BoltPhase::WAITING;
        s_boltPhaseTimer = 0.0f;
        s_stunTimer = 0.0f;
        s_strikeFlash = 0.0f;
        s_lungeEnemyIdx = -1;
        s_lungeTimer = 0.0f;
        SoundManager::PlayRainAmbient();
    }

    // Pick a random active enemy and apply the lunge speed boost
    static void Thunder_FireBolt(std::vector<Data::Enemy>& enemy_pool)
    {
        // Collect indices of valid targets
        static int candidates[64];
        int count = 0;
        for (int i = 0; i < (int)enemy_pool.size() && count < 64; ++i)
        {
            const Data::Enemy& e = enemy_pool[(size_t)i];
            if (e.enemy_is_active && !e.enemy_is_dying) candidates[count++] = i;
        }
        if (count == 0) return;  // no targets skip this bolt

        int chosen = candidates[rand() % count];
        Data::Enemy& target = enemy_pool[(size_t)chosen];

        // Store original speed and apply boost
        s_lungeEnemyIdx = chosen;
        s_lungeOrigSpeed = target.enemy_fall_speed_units_per_second;
        s_lungeTimer = LUNGE_DUR;
        target.enemy_fall_speed_units_per_second *= LUNGE_SPEED_MUL;

        // Spawn on-enemy strike animation
        s_tStrike = { true, target.enemy_position_x, target.enemy_position_y, 0, 0.0f };

        // Trigger stun + white flash
        s_stunTimer = STUN_DUR;
        s_strikeFlash = 1.0f;
        SoundManager::PlayLightningSfx();
    }

    // Restore lunging enemy's original speed when lunge expires
    static void Thunder_RestoreLunge(std::vector<Data::Enemy>& enemy_pool)
    {
        if (s_lungeEnemyIdx < 0 || s_lungeEnemyIdx >= (int)enemy_pool.size()) { s_lungeEnemyIdx = -1; s_lungeTimer = 0.0f; return; }
        Data::Enemy& e = enemy_pool[(size_t)s_lungeEnemyIdx];
        // Only restore if still active (might have been killed)
        if (e.enemy_is_active) e.enemy_fall_speed_units_per_second = s_lungeOrigSpeed;
        s_lungeEnemyIdx = -1;
        s_lungeTimer = 0.0f;
    }

    // Main thunder update — called every frame while storm is active
    static void Thunder_Update(float dt, std::vector<Data::Enemy>& enemy_pool)
    {
        // Count down total storm duration
        s_stormTimer -= dt;
        if (s_stormTimer <= 0.0f)
        {
            // Storm over — restore lunge if still active, go IDLE
            Thunder_RestoreLunge(enemy_pool);
            s_stunTimer = 0.0f;
            s_strikeFlash = 0.0f;
            s_state = WeatherState::IDLE;
            ResetTriggerTimer();
            SoundManager::StopRainAmbient();
            return;
        }

        // Decay stun
        if (s_stunTimer > 0.0f) s_stunTimer -= dt;
        // Decay white flash
        if (s_strikeFlash > 0.0f) s_strikeFlash -= dt / STRIKE_FLASH_DUR;
        if (s_strikeFlash < 0.0f) s_strikeFlash = 0.0f;

        // Decay lunge timer — restore when it expires
        if (s_lungeTimer > 0.0f)
        {
            s_lungeTimer -= dt;
            if (s_lungeTimer <= 0.0f) Thunder_RestoreLunge(enemy_pool);
        }

        // Also restore lunge if struck enemy was killed mid-lunge
        if (s_lungeEnemyIdx >= 0 && s_lungeEnemyIdx < (int)enemy_pool.size())
        {
            const Data::Enemy& e = enemy_pool[(size_t)s_lungeEnemyIdx];
            if (!e.enemy_is_active || e.enemy_is_dying) Thunder_RestoreLunge(enemy_pool);
        }

        // ---- Bolt phase state machine ----
        s_boltPhaseTimer += dt;

        switch (s_boltPhase)
        {
        case BoltPhase::WAITING:
            s_boltTimer -= dt;
            if (s_boltTimer <= 0.0f)
            {
                // Transition to WARNING
                s_boltPhase = BoltPhase::WARNING;
                s_boltPhaseTimer = 0.0f;
            }
            break;

        case BoltPhase::WARNING:
            if (s_boltPhaseTimer >= BRACE_WARNING_DUR)
            {
                // Fire the bolt
                Thunder_FireBolt(enemy_pool);
                s_boltPhase = BoltPhase::STRIKING;
                s_boltPhaseTimer = 0.0f;
            }
            break;

        case BoltPhase::STRIKING:
            // Stay in STRIKING while stun lasts, then set up next bolt
            if (s_stunTimer <= 0.0f)
            {
                s_boltPhase = BoltPhase::WAITING;
                s_boltPhaseTimer = 0.0f;
                s_boltTimer = BOLT_INTERVAL;
            }
            break;
        }
    }

    // "BRACE!" warning text and stun indicator
    static void Thunder_DrawHUD()
    {
        if (Graphics::font_id < 0) return;
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        if (s_boltPhase == BoltPhase::WARNING)
        {
            // Blink speed increases as warning time runs out
            float progress = Clamp01(s_boltPhaseTimer / BRACE_WARNING_DUR);
            float blinkHz = 2.0f + progress * 6.0f;   // 2 Hz → 8 Hz
            float pulse = ((int)(s_boltPhaseTimer * blinkHz * 2.0f) % 2 == 0) ? 1.0f : 0.4f;
            DrawCentredText("BRACE!", 0.25f, 1.0f, 1.0f, 0.85f, 0.1f, pulse);   // yellow
        }
        else if (s_boltPhase == BoltPhase::STRIKING && s_stunTimer > 0.0f)
        {
            float alpha = Clamp01(s_stunTimer / STUN_DUR);
            DrawCentredText("STUNNED!", 0.10f, 1.2f, 1.0f, 0.3f, 0.1f, alpha);   // red-orange, larger
        }
    }

    // Full-screen white flash on strike
    static void Thunder_DrawStrikeFlash()
    {
        if (s_strikeFlash <= 0.001f) return;
        DrawFullscreenOverlay(1.0f, 1.0f, 0.85f, s_strikeFlash * 0.75f);   // warm white/yellow flash
    }

    // Per-enemy thunderstorm sprite played on the struck enemy
    static void Thunder_DrawStrikeAnim()
    {
        if (!s_tStrike.active || !s_tStrikeTex || !s_tStrikeMesh) return;

        float uOffset = s_tStrike.frame * TSTRIKE_UV_W;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_ADD);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_tStrikeTex, uOffset, 0.0f);
        Mesh_Renderer(s_tStrikeMesh, s_tStrike.x, s_tStrike.y,
            TSTRIKE_DISPLAY_W, TSTRIKE_DISPLAY_H, 0.0f);

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // ================================================================
    //  Rainbow helpers
    // ================================================================

    // No persistent mesh needed — we build an exact UV-sliced mesh per frame in Rainbow_DrawSprite.
    static void Rainbow_BuildMesh()
    {
        if (s_rainbowMesh) { AEGfxMeshFree(s_rainbowMesh); s_rainbowMesh = nullptr; }
        // intentionally empty — per-frame meshes created in Rainbow_DrawSprite
    }

    // Kick off a new rainbow event
    static void Rainbow_Start()
    {
        s_state = WeatherState::FREEZING;
        s_rainbowTimer = RAINBOW_DURATION;
        s_rainbowPowerupTimer = RAINBOW_POWERUP_INTERVAL;
        s_rainbowFrame = 0;
        s_rainbowFrameTimer = 0.0f;
        s_rainbowPopupTimer = WEATHER_POPUP_DUR;
    }

    // Per-frame rainbow update: advance animation, trigger power-up spawns
    static void Rainbow_Update(float dt)
    {
        // Count down total event duration
        s_rainbowTimer -= dt;
        if (s_rainbowTimer <= 0.0f)
        {
            // Event over — restore default power-up spawn rate and go idle
            PowerUpSystem::SetDropSpawnInterval(15.0f, 30.0f);
            s_state = WeatherState::IDLE;
            ResetTriggerTimer();
            return;
        }

        // Advance sprite sheet animation
        s_rainbowFrameTimer += dt;
        if (s_rainbowFrameTimer >= RAINBOW_FRAME_TIME)
        {
            s_rainbowFrameTimer -= RAINBOW_FRAME_TIME;
            s_rainbowFrame = (s_rainbowFrame + 1) % RAINBOW_FRAME_COUNT;
        }

        // Every RAINBOW_POWERUP_INTERVAL seconds, force the PowerUp system to
        // spawn a drop almost immediately by shrinking its spawn window to near-zero.
        // The interval is restored when the rainbow event ends (or exits).
        s_rainbowPowerupTimer -= dt;
        if (s_rainbowPowerupTimer <= 0.0f)
        {
            s_rainbowPowerupTimer = RAINBOW_POWERUP_INTERVAL;
            PowerUpSystem::SetDropSpawnInterval(0.01f, 0.05f);
        }
    }

    // Draw the animated rainbow sprite across the sky.
    // Uses a fresh per-frame Spritesheet_Mesh so the UV slice is exact — no stretching.
    static void Rainbow_DrawSprite()
    {
        if (!s_rainbowTex) return;

        // Compute smooth fade-in / fade-out alpha
        float elapsed = RAINBOW_DURATION - s_rainbowTimer;
        float fadeIn = Clamp01(elapsed / RAINBOW_FADE_TIME);
        float fadeOut = Clamp01(s_rainbowTimer / RAINBOW_FADE_TIME);
        float alpha = fadeIn * fadeOut;
        if (alpha <= 0.001f) return;

        // Build a mesh with the UV slice for the current frame baked in
        constexpr float uvW = 1.0f / (float)RAINBOW_FRAME_COUNT;
        float uMin = (float)s_rainbowFrame * uvW;
        float uMax = uMin + uvW;
        AEGfxVertexList* frameMesh = Spritesheet_Mesh(1.0f, 1.0f, uMin, 0.0f, uMax, 1.0f);
        if (!frameMesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_rainbowTex, 0.0f, 0.0f);  // no offset — UVs baked into mesh
        Mesh_Renderer(frameMesh, 0.0f, RAINBOW_Y, RAINBOW_DISPLAY_W, RAINBOW_DISPLAY_H, 0.0f);

        AEGfxMeshFree(frameMesh);

        // Reset render state
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // Rainbow HUD: cycling rainbow-coloured "POWER UPS GALORE!" + timer below
    static void Rainbow_DrawHUD()
    {
        if (s_rainbowPopupTimer <= 0.0f || Graphics::font_id < 0) return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Fade out over last WEATHER_POPUP_FADE seconds
        float alpha = (s_rainbowPopupTimer < WEATHER_POPUP_FADE)
            ? (s_rainbowPopupTimer / WEATHER_POPUP_FADE) : 1.0f;

        // Cycle hue using the pulse timer
        constexpr float HUE_SPEED = 2.5f;
        float h = fmodf(s_weatherPulseTimer * HUE_SPEED, 6.0f);
        float r, g, b;
        int   seg = (int)h;
        float f = h - (float)seg;
        switch (seg % 6)
        {
        case 0: r = 1.0f;     g = f;        b = 0.0f;     break;
        case 1: r = 1.0f - f; g = 1.0f;     b = 0.0f;     break;
        case 2: r = 0.0f;     g = 1.0f;     b = f;        break;
        case 3: r = 0.0f;     g = 1.0f - f; b = 1.0f;     break;
        case 4: r = f;        g = 0.0f;     b = 1.0f;     break;
        default: r = 1.0f;    g = 0.0f;     b = 1.0f - f; break;
        }

        constexpr float BASE_SCALE = 0.75f;
        constexpr float PULSE_AMP = 0.08f;
        float scale = BASE_SCALE + PULSE_AMP * sinf(s_weatherPulseTimer * 4.0f);

        DrawCentredText("POWER UPS GALORE!", 0.25f, scale, r, g, b, alpha);
    }

    // ================================================================
    //  Ghost weather helpers
    // ================================================================

    // Ghost mesh is rebuilt each draw call with exact UV coords for the current frame.
    // No persistent mesh needed — we free/rebuild per-ghost per-frame using Spritesheet_Mesh.
    static void Ghost_BuildMesh()
    {
        // Nothing to pre-build; per-frame meshes are created in Ghost_Draw.
        // Free any stale mesh from a previous session.
        if (s_ghostMesh) { AEGfxMeshFree(s_ghostMesh); s_ghostMesh = nullptr; }
    }

    // Spawn one ghost at a random screen edge with a random inward velocity.
    static void Ghost_SpawnOne(Ghost& g, int digit)
    {
        // Pick a random edge: 0=top, 1=bottom, 2=left, 3=right
        int edge = rand() % 4;
        switch (edge)
        {
        case 0: // top
            g.x = RandF(-HALF_W, HALF_W);
            g.y = HALF_H + GHOST_SPAWN_MARGIN;
            break;
        case 1: // bottom
            g.x = RandF(-HALF_W, HALF_W);
            g.y = -HALF_H - GHOST_SPAWN_MARGIN;
            break;
        case 2: // left
            g.x = -HALF_W - GHOST_SPAWN_MARGIN;
            g.y = RandF(-HALF_H, HALF_H);
            break;
        default: // right
            g.x = HALF_W + GHOST_SPAWN_MARGIN;
            g.y = RandF(-HALF_H, HALF_H);
            break;
        }

        // Random direction — ensure it has a non-trivial inward component
        float angle = RandF(0.0f, 6.2832f); // full 2*pi
        float speed = RandF(GHOST_SPEED_MIN, GHOST_SPEED_MAX);
        g.velX = cosf(angle) * speed;
        g.velY = sinf(angle) * speed;

        g.digit = digit;
        g.alive = true;
        g.frame = 0;
        g.frameTimer = 0.0f;
    }

    // Kick off a new ghost weather event.
    static void Ghost_Start()
    {
        s_ghostEventActive = true;
        s_ghostLifeTimer = 0.0f;
        s_ghostLockTimer = 0.0f;
        s_ghostPopupTimer = WEATHER_POPUP_DUR;

        // Assign digits 5-9 (one per ghost, shuffled slightly via rand)
        int digits[GHOST_COUNT] = { 5, 6, 7, 8, 9 };
        // Fisher-Yates shuffle for variety
        for (int i = GHOST_COUNT - 1; i > 0; --i)
        {
            int j = rand() % (i + 1);
            int tmp = digits[i]; digits[i] = digits[j]; digits[j] = tmp;
        }

        for (int i = 0; i < GHOST_COUNT; ++i)
            Ghost_SpawnOne(s_ghosts[i], digits[i]);
    }

    // Per-frame ghost update: movement, animation, digit input, lock timer.
    static void Ghost_Update(float dt)
    {
        // ---- Lock timer: count down ----
        if (s_ghostLockTimer > 0.0f)
        {
            s_ghostLockTimer -= dt;
            if (s_ghostLockTimer < 0.0f) s_ghostLockTimer = 0.0f;
        }

        // ---- Count still-alive ghosts ----
        int alive = 0;
        for (int i = 0; i < GHOST_COUNT; ++i)
            if (s_ghosts[i].alive) ++alive;

        // Event ends when all ghosts are dead and no lock is pending
        if (alive == 0 && s_ghostLockTimer <= 0.0f)
        {
            s_ghostEventActive = false;
            s_state = WeatherState::IDLE;
            ResetTriggerTimer();
            return;
        }

        // ---- Life timer: triggers lock if a ghost survives too long ----
        if (alive > 0)
        {
            s_ghostLifeTimer += dt;
            if (s_ghostLifeTimer >= GHOST_LIFETIME && s_ghostLockTimer <= 0.0f)
            {
                // Lock typing for GHOST_LOCK_DUR seconds
                s_ghostLockTimer = GHOST_LOCK_DUR;
                s_ghostLifeTimer = 0.0f; // reset so subsequent survivors can trigger again
            }
        }

        // ---- Move and animate live ghosts ----
        for (int i = 0; i < GHOST_COUNT; ++i)
        {
            Ghost& g = s_ghosts[i];
            if (!g.alive) continue;

            g.x += g.velX * dt;
            g.y += g.velY * dt;

            // Bounce off screen edges to keep ghosts on screen
            if (g.x < -HALF_W) { g.x = -HALF_W; g.velX = fabsf(g.velX); }
            if (g.x > HALF_W) { g.x = HALF_W; g.velX = -fabsf(g.velX); }
            if (g.y < -HALF_H) { g.y = -HALF_H; g.velY = fabsf(g.velY); }
            if (g.y > HALF_H) { g.y = HALF_H; g.velY = -fabsf(g.velY); }

            // Advance animation frame
            g.frameTimer += dt;
            if (g.frameTimer >= GHOST_FRAME_TIME)
            {
                g.frameTimer -= GHOST_FRAME_TIME;
                g.frame = (g.frame + 1) % GHOST_FRAME_COUNT;
            }
        }

        // ---- Check digit input (only when typing is not locked) ----
        if (s_ghostLockTimer <= 0.0f)
        {
            // Check keys '5' through '9'
            for (int key = 5; key <= 9; ++key)
            {
                // AEVK_0 = 0x30, so digit key = 0x30 + key
                u8 vk = (u8)(0x30 + key);
                if (AEInputCheckTriggered(vk))
                {
                    // Kill the first live ghost whose digit matches
                    for (int i = 0; i < GHOST_COUNT; ++i)
                    {
                        if (s_ghosts[i].alive && s_ghosts[i].digit == key)
                        {
                            s_ghosts[i].alive = false;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Draw all live ghosts (sprite + digit label above them).
    static void Ghost_Draw()
    {
        if (!s_ghostTex) return;

        constexpr float uvW = 1.0f / (float)GHOST_FRAME_COUNT;

        for (int i = 0; i < GHOST_COUNT; ++i)
        {
            const Ghost& g = s_ghosts[i];
            if (!g.alive) continue;

            // ---- Build a fresh mesh with exact UV slice for this frame ----
            float uMin = (float)g.frame * uvW;
            float uMax = uMin + uvW;
            AEGfxVertexList* frameMesh = Spritesheet_Mesh(1.0f, 1.0f, uMin, 0.0f, uMax, 1.0f);
            if (!frameMesh) continue;

            // ---- Draw sprite ----
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxTextureSet(s_ghostTex, 0.0f, 0.0f); // no extra offset; UVs are baked
            Mesh_Renderer(frameMesh, g.x, g.y, GHOST_SIZE, GHOST_SIZE, 0.0f);

            AEGfxMeshFree(frameMesh);

            // ---- Draw digit label above the ghost ----
            if (Graphics::font_id >= 0)
            {
                char label[2] = { (char)('0' + g.digit), '\0' };
                constexpr float DIGIT_SCALE = 0.6f;

                float hw = (float)AEGfxGetWindowWidth() * 0.5f;
                float hh = (float)AEGfxGetWindowHeight() * 0.5f;
                float ndcX = (hw > 0.0f) ? (g.x / hw) : 0.0f;
                float ndcY = (hh > 0.0f) ? ((g.y + GHOST_LABEL_OFFSET_Y) / hh) : 0.0f;

                float tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(Graphics::font_id, label, DIGIT_SCALE, &tw, &th);
                ndcX -= tw * 0.5f;

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::font_id, label, ndcX, ndcY, DIGIT_SCALE, 1.0f, 0.95f, 0.2f, 1.0f);
            }
        }

        // Reset render state
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    // Draw "DEFEAT THE GHOSTS!" prompt always while event is active,
    // plus "GHOSTIFIED!" lock indicator when typing is locked.
    static void Ghost_DrawHUD()
    {
        if (Graphics::font_id < 0) return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // ---- "DEFEAT THE GHOSTS!" — popup only on event start ----
        if (s_ghostPopupTimer > 0.0f)
        {
            float alpha = (s_ghostPopupTimer < WEATHER_POPUP_FADE)
                ? (s_ghostPopupTimer / WEATHER_POPUP_FADE) : 1.0f;
            float brightness = fabsf(sinf(s_weatherPulseTimer * 2.5f));
            float scale = 0.65f + 0.08f * sinf(s_weatherPulseTimer * 3.5f);
            DrawCentredText("DEFEAT THE GHOSTS!", 0.38f, scale,
                brightness, brightness, brightness, alpha);
        }

        // ---- "GHOSTIFIED!" lock indicator ----
        if (s_ghostLockTimer > 0.0f)
        {
            float pulse = ((int)(s_ghostLockTimer * 6.0f) % 2 == 0) ? 1.0f : 0.55f;
            DrawCentredText("GHOSTIFIED!", 0.10f, 0.85f, 1.0f, 0.15f, 0.15f, pulse);

            char buf[16];
            snprintf(buf, sizeof(buf), "%d", (int)ceilf(s_ghostLockTimer));
            DrawCentredText(buf, -0.05f, 0.65f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}