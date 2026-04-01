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

@author     Azfar
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
    constexpr float TRIGGER_MIN_SEC = 15.0f;   // earliest
    constexpr float TRIGGER_MAX_SEC = 30.0f;   // latest

    // Phase durations
    constexpr float FREEZE_IN_DUR = 3.0f;
    constexpr float THAW_DUR = 3.0f;

    // Spacebar break free mechanic
    constexpr int SPACES_NEEDED = 20;

    // Particle counts
    constexpr int AMBIENT_COUNT = 80;
    constexpr int BLIZZARD_COUNT = 140;

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
    constexpr int   RAIN_COUNT = 200;       // number of raindrops
    constexpr float RAIN_SPEED_MIN = 500.0f;    // downward fall
    constexpr float RAIN_SPEED_MAX = 1000.0f;
    constexpr float RAIN_DRIFT = 80.0f;     // -ve for left, +ve for right
    constexpr float RAIN_W_MIN = 2.0f;      // width
    constexpr float RAIN_W_MAX = 3.0f;
    constexpr float RAIN_H_MIN = 14.0f;     // height
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
    constexpr float BOLT_INTERVAL = 4.0f;    // lightning strike
    constexpr float BRACE_WARNING_DUR = 1.0f;    // BRACE! warning shown x seconds before the strike
    constexpr float STRIKE_FLASH_DUR = 0.15f;   // White flash duration on strike
    constexpr float STUN_DUR = 0.5f;    // Input stun duration on strike
    constexpr float LUNGE_DUR = 2.5f;    // How long the struck enemy falls at boosted speed
    constexpr float LUNGE_SPEED_MUL = 3.5f;
    constexpr float STORM_OVL_ALPHA_MAX = 0.25f;

    // Storm overlay colour (dark grey)
    constexpr float STORM_OVL_R = 0.10f;
    constexpr float STORM_OVL_G = 0.08f;
    constexpr float STORM_OVL_B = 0.18f;

    // Thunderstorm sprite animation (thunderstorm.png — 3 frames horizontal)
    constexpr int   TSTRIKE_FRAME_COUNT = 3;
    constexpr float TSTRIKE_FRAME_TIME = 0.12f;                              // ~12 fps, snappy
    constexpr float TSTRIKE_UV_W = 1.0f / (float)TSTRIKE_FRAME_COUNT;
    constexpr float TSTRIKE_DISPLAY_W = 120.0f;
    constexpr float TSTRIKE_DISPLAY_H = 200.0f;

    struct TStrikeAnim
    {
        bool  active = false;
        float x = 0.0f;
        float y = 0.0f;
        int   frame = 0;
        float frame_timer = 0.0f;
    };

    static TStrikeAnim       s_t_strike{};
    static AEGfxTexture* s_t_strike_tex = nullptr;
    static AEGfxVertexList* s_t_strike_mesh = nullptr;   // single-frame-width UV quad

    // ================================================================
    //  Rainbow constants
    // ================================================================
    constexpr float RAINBOW_DURATION = 10.0f;   // total event length
    constexpr float RAINBOW_POWERUP_INTERVAL = 5.0f;    // spawn a power-up every N seconds
    constexpr int   RAINBOW_FRAME_COUNT = 9;       // frames in the sprite sheet
    constexpr float RAINBOW_FRAME_TIME = 0.12f;   // seconds per frame (~8 fps)
    constexpr float RAINBOW_DISPLAY_W = 1600.0f; // stretch to full screen width
    constexpr float RAINBOW_DISPLAY_H = 80.0f;
    constexpr float RAINBOW_Y = 250.0f;  // upper sky area, well above enemies
    constexpr float RAINBOW_FADE_TIME = 1.5f;    // fade-in / fade-out duration

    // ================================================================
    //  Ghost weather constants
    // ================================================================
    constexpr int   GHOST_COUNT = 5;
    constexpr float GHOST_LIFETIME = 10.0f;   // seconds before lock triggers
    constexpr float GHOST_LOCK_DUR = 3.0f;    // typing locked duration
    constexpr float GHOST_SPEED_MIN = 80.0f;
    constexpr float GHOST_SPEED_MAX = 160.0f;
    constexpr float GHOST_SIZE = 120.0f;  // display size (px)
    // Sprite sheet: 16 frames in one row (walk cycle)
    constexpr int   GHOST_FRAME_COUNT = 16;
    constexpr float GHOST_FRAME_TIME = 0.07f;   // seconds per frame
    constexpr float GHOST_LABEL_OFFSET_Y = 38.0f;  // px above ghost centre for digit label
    constexpr float GHOST_SPAWN_MARGIN = 80.0f;   // off-screen spawn margin

    // Inline helpers
    static inline float RandF(float lo, float hi)
    {
        return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
    }
    static inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
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

    // Thunderstorm specific bolt phases
    enum class BoltPhase
    {
        WAITING,    // counting down to next bolt
        WARNING,    // BRACE! shown
        STRIKING,   // white flash + stun active
    };

    // Snow particle
    struct Snowflake { float x, y, speed_y, drift_x, size, alpha; bool is_blizzard; };

    // Rain particle
    struct Raindrop { float x, y, speed_y, w, h, alpha; };

    // Ghost particle (ghost weather)
    struct Ghost
    {
        float x, y;          // world-space position
        float vel_x, vel_y;  // velocity (px/s)
        int   digit;         // 5-9 assigned digit
        bool  alive;         // false once killed by player
        int   frame;         // current animation frame
        float frame_timer;   // accumulator for frame advance
    };

    // Shared state
    static WeatherState s_state = WeatherState::IDLE;
    static WeatherType  s_active_type = WeatherType::SNOW;
    static float        s_trigger_timer = 0.0f;
    static float        s_trigger_target = 0.0f;
    static float        s_phase_timer = 0.0f;

    // Snow state
    static int   s_space_presses = 0;
    static float s_flash_timer = 0.0f;
    static bool  s_space_was_held = false;

    // Thunderstorm state
    static float     s_storm_timer = 0.0f;
    static float     s_bolt_timer = 0.0f;
    static BoltPhase s_bolt_phase = BoltPhase::WAITING;
    static float     s_bolt_phase_timer = 0.0f;
    static float     s_stun_timer = 0.0f;
    static float     s_strike_flash = 0.0f;
    static int       s_lunge_enemy_idx = -1;
    static float     s_lunge_timer = 0.0f;
    static float     s_lunge_orig_speed = 0.0f;

    // Rainbow state
    static float            s_rainbow_timer = 0.0f;
    static float            s_rainbow_powerup_timer = 0.0f;
    static int              s_rainbow_frame = 0;
    static float            s_rainbow_frame_timer = 0.0f;
    static AEGfxTexture* s_rainbow_tex = nullptr;
    static AEGfxVertexList* s_rainbow_mesh = nullptr;

    // Running pulse timer (always increments, used for sine-wave text pulsing)
    static float s_weather_pulse_timer = 0.0f;

    // Short popup timers: text shows briefly on event start then disappears
    constexpr float WEATHER_POPUP_DUR = 1.8f;
    constexpr float WEATHER_POPUP_FADE = 0.5f;
    static float s_rainbow_popup_timer = 0.0f;
    static float s_ghost_popup_timer = 0.0f;

    // Ghost weather state
    static Ghost            s_ghosts[GHOST_COUNT]{};
    static float            s_ghost_life_timer = 0.0f;
    static float            s_ghost_lock_timer = 0.0f;
    static bool             s_ghost_event_active = false;
    static AEGfxTexture* s_ghost_tex = nullptr;
    static AEGfxVertexList* s_ghost_mesh = nullptr;

    // Ghost pre-event announcement (shown for 3s before ghosts actually spawn)
    constexpr float GHOST_ANNOUNCE_DUR = 3.0f;
    static float    s_ghost_announce_timer = 0.0f;
    static bool     s_ghost_announce_pending = false;

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
    static void  reset_trigger_timer();
    static void  spawn_ambient_flake(Snowflake& f);
    static void  spawn_blizzard_flake(Snowflake& f);
    static void  update_flake(Snowflake& f, float dt, float speed_mul);
    static float get_blizzard_intensity();
    static void  draw_fullscreen_overlay(float r, float g, float b, float alpha);
    static void  snow_draw_particles(float blizzard_intensity);
    static void  draw_freeze_hud();
    static void  draw_progress_bar();

    // Rain
    static void  spawn_raindrop(Raindrop& r);
    static void  update_raindrop(Raindrop& r, float dt);
    static void  thunder_draw_rain();

    // Thunder helpers
    static void  thunder_start_storm();
    static void  thunder_update(float dt, std::vector<Data::Enemy>& enemy_pool);
    static void  thunder_fire_bolt(std::vector<Data::Enemy>& enemy_pool);
    static void  thunder_restore_lunge(std::vector<Data::Enemy>& enemy_pool);
    static void  thunder_draw_hud();
    static void  thunder_draw_strike_flash();
    static void  thunder_draw_strike_anim();

    // Rainbow helpers
    static void  rainbow_build_mesh();
    static void  rainbow_start();
    static void  rainbow_update(float dt);
    static void  rainbow_draw_sprite();
    static void  rainbow_draw_hud();

    // Ghost weather helpers
    static void  ghost_build_mesh();
    static void  ghost_start();
    static void  ghost_spawn_all();
    static void  ghost_update(float dt);
    static void  ghost_draw();
    static void  ghost_draw_hud();
    static void  ghost_spawn_one(Ghost& g, int digit);

    static void  draw_centred_text(const char* text, float ndc_y, float scale,
        float r, float g, float b, float alpha);
}

void weather_init()
{
    if (s_quad) { AEGfxMeshFree(s_quad); s_quad = nullptr; }
    s_quad = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

    // Shared reset
    s_state = WeatherState::IDLE;
    s_phase_timer = 0.0f;

    // Snow reset
    s_space_presses = 0;
    s_flash_timer = 0.0f;
    s_space_was_held = false;

    // Thunder reset
    s_storm_timer = 0.0f;
    s_bolt_timer = 0.0f;
    s_bolt_phase = BoltPhase::WAITING;
    s_bolt_phase_timer = 0.0f;
    s_stun_timer = 0.0f;
    s_strike_flash = 0.0f;
    s_lunge_enemy_idx = -1;
    s_lunge_timer = 0.0f;
    s_lunge_orig_speed = 0.0f;

    // Rainbow reset - free then reload to avoid double-load on re-init
    if (s_rainbow_tex) { AEGfxTextureUnload(s_rainbow_tex);  s_rainbow_tex = nullptr; }
    if (s_rainbow_mesh) { AEGfxMeshFree(s_rainbow_mesh);      s_rainbow_mesh = nullptr; }
    s_rainbow_timer = 0.0f;
    s_rainbow_powerup_timer = 0.0f;
    s_rainbow_frame = 0;
    s_rainbow_frame_timer = 0.0f;
    s_rainbow_tex = AEGfxTextureLoad("Assets/rainbowvfx.png");
    rainbow_build_mesh();

    // Thunderstorm strike animation
    if (s_t_strike_tex) { AEGfxTextureUnload(s_t_strike_tex);  s_t_strike_tex = nullptr; }
    if (s_t_strike_mesh) { AEGfxMeshFree(s_t_strike_mesh);      s_t_strike_mesh = nullptr; }
    s_t_strike_tex = AEGfxTextureLoad("Assets/thunderstorm.png");
    s_t_strike_mesh = Sprite_Mesh(TSTRIKE_UV_W, 1.0f);
    s_t_strike = {};

    s_weather_pulse_timer = 0.0f;
    s_rainbow_popup_timer = 0.0f;
    s_ghost_popup_timer = 0.0f;

    // Ghost weather init
    s_ghost_tex = AEGfxTextureLoad("Assets/ghost.png");
    ghost_build_mesh();
    s_ghost_event_active = false;
    s_ghost_life_timer = 0.0f;
    s_ghost_lock_timer = 0.0f;
    for (int i = 0; i < GHOST_COUNT; ++i) s_ghosts[i].alive = false;

    reset_trigger_timer();

    for (int i = 0; i < AMBIENT_COUNT; ++i)
    {
        spawn_ambient_flake(s_ambient[i]);
        s_ambient[i].y = RandF(-HALF_H, HALF_H);  // scatter on startup
    }

    for (int i = 0; i < BLIZZARD_COUNT; ++i)
        spawn_blizzard_flake(s_blizzard[i]);

    // Scatter rain
    for (int i = 0; i < RAIN_COUNT; ++i)
    {
        spawn_raindrop(s_rain[i]);
        s_rain[i].y = RandF(-HALF_H, HALF_H);  // scatter vertically
    }
}

void weather_update(float dt, std::vector<Data::Enemy>& enemy_pool)
{
    // Always tick the pulse timer (used for sine-wave text effects)
    s_weather_pulse_timer += dt;
    if (s_rainbow_popup_timer > 0.0f) { s_rainbow_popup_timer -= dt; if (s_rainbow_popup_timer < 0.0f) s_rainbow_popup_timer = 0.0f; }
    if (s_ghost_popup_timer > 0.0f) { s_ghost_popup_timer -= dt; if (s_ghost_popup_timer < 0.0f) s_ghost_popup_timer = 0.0f; }

    // Advance thunderstorm on-enemy strike animation
    if (s_t_strike.active)
    {
        s_t_strike.frame_timer += dt;
        if (s_t_strike.frame_timer >= TSTRIKE_FRAME_TIME)
        {
            s_t_strike.frame_timer -= TSTRIKE_FRAME_TIME;
            ++s_t_strike.frame;
            if (s_t_strike.frame >= TSTRIKE_FRAME_COUNT)
                s_t_strike.active = false;
        }
    }

    // Advance trigger timer when no weather active
    if (s_state == WeatherState::IDLE)
    {
        s_trigger_timer += dt;
        if (s_trigger_timer >= s_trigger_target)
        {
            // Weighted pick: Snow 30%, Thunder 30%, Ghost 30%, Rainbow 10%
            int pick = rand() % 10;
            if (pick < 3) s_active_type = WeatherType::SNOW;
            else if (pick < 6) s_active_type = WeatherType::THUNDER;
            else if (pick < 9) s_active_type = WeatherType::GHOST;
            else               s_active_type = WeatherType::RAINBOW;

            if (s_active_type == WeatherType::SNOW)
            {
                s_state = WeatherState::FREEZING;
                s_phase_timer = 0.0f;
                s_space_presses = 0;
                SoundManager::play_snow_ambient();
            }
            else if (s_active_type == WeatherType::THUNDER)
            {
                thunder_start_storm();
            }
            else if (s_active_type == WeatherType::RAINBOW)
            {
                rainbow_start();
            }
            else
            {
                ghost_start();
            }
        }
    }

    // ================================================================
    //  SNOW path
    // ================================================================
    if (s_active_type == WeatherType::SNOW)
    {
        // FREEZING phase
        if (s_state == WeatherState::FREEZING)
        {
            s_phase_timer += dt;
            if (s_phase_timer >= FREEZE_IN_DUR) { s_state = WeatherState::FROZEN; s_phase_timer = 0.0f; }
        }

        // FROZEN phase: count space presses
        if (s_state == WeatherState::FROZEN)
        {
            bool space_held = (AEInputCheckCurr(AEVK_SPACE) != 0);
            bool space_edge = space_held && !s_space_was_held;
            s_space_was_held = space_held;

            if (space_edge)
            {
                ++s_space_presses;
                s_flash_timer = FLASH_DUR;
                if (s_space_presses >= SPACES_NEEDED) { s_state = WeatherState::THAWING; s_phase_timer = 0.0f; SoundManager::stop_snow_ambient(); }
            }
            if (s_flash_timer > 0.0f) s_flash_timer -= dt;
        }

        // THAWING phase
        if (s_state == WeatherState::THAWING)
        {
            s_phase_timer += dt;
            if (s_phase_timer >= THAW_DUR) { s_state = WeatherState::IDLE; reset_trigger_timer(); }
        }

        // Update snow particles
        for (int i = 0; i < AMBIENT_COUNT; ++i)  update_flake(s_ambient[i], dt, 1.0f);
        float blz_mul = 0.3f + get_blizzard_intensity() * 1.4f;
        for (int i = 0; i < BLIZZARD_COUNT; ++i) update_flake(s_blizzard[i], dt, blz_mul);
    }

    // ================================================================
    //  THUNDER path
    // ================================================================
    if (s_active_type == WeatherType::THUNDER && s_state != WeatherState::IDLE)
    {
        thunder_update(dt, enemy_pool);
        for (int i = 0; i < RAIN_COUNT; ++i) update_raindrop(s_rain[i], dt);
    }

    // ================================================================
    //  RAINBOW path
    // ================================================================
    if (s_active_type == WeatherType::RAINBOW && s_state != WeatherState::IDLE)
    {
        rainbow_update(dt);
    }

    // ================================================================
    //  GHOST path
    // ================================================================
    if (s_active_type == WeatherType::GHOST && s_ghost_event_active)
    {
        ghost_update(dt);
    }
}

// ----------------------------------------------------------------
void weather_draw_background()
{
    // Rainbow is drawn BEFORE enemies so it sits behind them in the scene
    if (s_active_type == WeatherType::RAINBOW && s_state != WeatherState::IDLE)
        rainbow_draw_sprite();
}

// ----------------------------------------------------------------
void weather_draw()
{
    // Ghost weather has its own active flag, not bound to the shared state machine
    bool ghost_visible = (s_active_type == WeatherType::GHOST && s_ghost_event_active);
    if (!s_quad || (s_state == WeatherState::IDLE && !ghost_visible)) return;

    if (s_active_type == WeatherType::SNOW)
    {
        float intensity = get_blizzard_intensity();
        snow_draw_particles(intensity);
        draw_fullscreen_overlay(OVL_R, OVL_G, OVL_B, OVL_ALPHA_MAX * intensity);
        if (s_state == WeatherState::FROZEN) { draw_freeze_hud(); draw_progress_bar(); }
    }
    else if (s_active_type == WeatherType::THUNDER)
    {
        thunder_draw_rain();
        // Dark stormy overlay fades in over first 2 seconds of storm
        float elapsed = STORM_DURATION - s_storm_timer;
        draw_fullscreen_overlay(STORM_OVL_R, STORM_OVL_G, STORM_OVL_B,
            STORM_OVL_ALPHA_MAX * Clamp01(elapsed / 2.0f));
        thunder_draw_strike_anim();
        thunder_draw_hud();
        thunder_draw_strike_flash();
    }
    // RAINBOW: sprite drawn in weather_draw_background() - HUD drawn here
    if (s_active_type == WeatherType::RAINBOW && s_state != WeatherState::IDLE)
    {
        rainbow_draw_hud();
    }
    // GHOST: ghosts + HUD drawn here
    if (s_active_type == WeatherType::GHOST && s_ghost_event_active)
    {
        ghost_draw();
        ghost_draw_hud();
    }
}

void weather_exit()
{
    SoundManager::stop_snow_ambient();
    SoundManager::stop_rain_ambient();
    if (s_quad) { AEGfxMeshFree(s_quad);              s_quad = nullptr; }
    if (s_rainbow_tex) { AEGfxTextureUnload(s_rainbow_tex);  s_rainbow_tex = nullptr; }
    if (s_rainbow_mesh) { AEGfxMeshFree(s_rainbow_mesh);      s_rainbow_mesh = nullptr; }
    if (s_t_strike_tex) { AEGfxTextureUnload(s_t_strike_tex);  s_t_strike_tex = nullptr; }
    if (s_t_strike_mesh) { AEGfxMeshFree(s_t_strike_mesh);      s_t_strike_mesh = nullptr; }
    if (s_ghost_tex) { AEGfxTextureUnload(s_ghost_tex);    s_ghost_tex = nullptr; }
    if (s_ghost_mesh) { AEGfxMeshFree(s_ghost_mesh);        s_ghost_mesh = nullptr; }
}

bool weather_is_player_frozen() { return s_state == WeatherState::FROZEN; }
bool weather_is_player_stunned() { return (s_active_type == WeatherType::THUNDER && s_stun_timer > 0.0f); }
bool weather_is_typing_locked() { return (s_active_type == WeatherType::GHOST && s_ghost_lock_timer > 0.0f); }

void weather_force_event(int type, std::vector<Data::Enemy>& enemy_pool)
{
    // Restore any active lunge before cancelling (prevents permanent speed boost)
    thunder_restore_lunge(enemy_pool);
    SoundManager::stop_snow_ambient();
    SoundManager::stop_rain_ambient();

    // Cancel any active event first, reset to a clean IDLE state
    s_state = WeatherState::IDLE;
    s_phase_timer = 0.0f;
    s_space_presses = 0;
    s_flash_timer = 0.0f;
    s_space_was_held = false;
    s_stun_timer = 0.0f;
    s_strike_flash = 0.0f;
    s_t_strike = {};

    // Ghost reset
    s_ghost_event_active = false;
    s_ghost_life_timer = 0.0f;
    s_ghost_lock_timer = 0.0f;
    s_rainbow_popup_timer = 0.0f;
    s_ghost_popup_timer = 0.0f;
    for (int i = 0; i < GHOST_COUNT; ++i) s_ghosts[i].alive = false;

    if (type == 0)
    {
        s_active_type = WeatherType::SNOW;
        s_state = WeatherState::FREEZING;
        s_phase_timer = 0.0f;
        SoundManager::play_snow_ambient();
    }
    else if (type == 1)
    {
        s_active_type = WeatherType::THUNDER;
        thunder_start_storm();
    }
    else if (type == 2)
    {
        s_active_type = WeatherType::RAINBOW;
        rainbow_start();
    }
    else
    {
        s_active_type = WeatherType::GHOST;
        ghost_start();
    }

    reset_trigger_timer();
}


// ============================================================
//  Internal helper implementations
// ============================================================
namespace
{
    static void reset_trigger_timer()
    {
        s_trigger_timer = 0.0f;
        s_trigger_target = RandF(TRIGGER_MIN_SEC, TRIGGER_MAX_SEC);
    }

    static void spawn_ambient_flake(Snowflake& f)
    {
        f.x = RandF(-HALF_W, HALF_W);
        f.y = HALF_H + RandF(0.0f, 60.0f);
        f.speed_y = RandF(AMB_SPEED_MIN, AMB_SPEED_MAX);
        f.drift_x = RandF(-DRIFT_RANGE, DRIFT_RANGE);
        f.size = RandF(FLAKE_SIZE_MIN, FLAKE_SIZE_MAX);
        f.alpha = RandF(0.4f, 0.85f);
        f.is_blizzard = false;
    }

    static void spawn_blizzard_flake(Snowflake& f)
    {
        f.x = RandF(-HALF_W, HALF_W);
        f.y = HALF_H + RandF(0.0f, 80.0f);
        f.speed_y = RandF(BLZ_SPEED_MIN, BLZ_SPEED_MAX);
        f.drift_x = RandF(-DRIFT_RANGE * 2.0f, DRIFT_RANGE * 2.0f);
        f.size = RandF(FLAKE_SIZE_MIN, FLAKE_SIZE_MAX - 1.0f);
        f.alpha = RandF(0.5f, 1.0f);
        f.is_blizzard = true;
    }

    static void update_flake(Snowflake& f, float dt, float speed_mul)
    {
        f.y -= f.speed_y * speed_mul * dt;
        f.x += f.drift_x * speed_mul * dt;

        // Wrap horizontally so flakes don't disappear off the side of screen
        if (f.x < -HALF_W) f.x += SCREEN_W;
        if (f.x > HALF_W) f.x -= SCREEN_W;

        // Respawn when below the screen
        if (f.y < -HALF_H - 20.0f)
            f.is_blizzard ? spawn_blizzard_flake(f) : spawn_ambient_flake(f);
    }

    static float get_blizzard_intensity()
    {
        if (s_active_type == WeatherType::THUNDER) return 0.0f;

        switch (s_state)
        {
        case WeatherState::IDLE:     return 0.0f;
        case WeatherState::FREEZING: return Clamp01(s_phase_timer / FREEZE_IN_DUR);
        case WeatherState::FROZEN:   return 1.0f + (s_flash_timer > 0.0f ? 0.15f * (s_flash_timer / FLASH_DUR) : 0.0f);
        case WeatherState::THAWING:  return Clamp01(1.0f - s_phase_timer / THAW_DUR);
        }
        return 0.0f;
    }

    static void draw_fullscreen_overlay(float r, float g, float b, float alpha)
    {
        if (alpha <= 0.001f) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(r, g, b, 1.0f);
        mesh_renderer(s_quad, 0.0f, 0.0f, SCREEN_W, SCREEN_H, 0.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(1.0f);
    }

    static void snow_draw_particles(float blizzard_intensity)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_ADD);   // bright, glowy snow

        // Ambient snowflakes (always visible)
        for (int i = 0; i < AMBIENT_COUNT; ++i)
        {
            const Snowflake& f = s_ambient[i];
            AEGfxSetTransparency(f.alpha);
            AEGfxSetColorToMultiply(0.85f, 0.93f, 1.0f, 1.0f);   // slight blue-white tint
            mesh_renderer(s_quad, f.x, f.y, f.size, f.size, 0.0f);
        }

        // Blizzard particles fade in with intensity
        if (blizzard_intensity > 0.01f)
        {
            for (int i = 0; i < BLIZZARD_COUNT; ++i)
            {
                const Snowflake& f = s_blizzard[i];
                AEGfxSetTransparency(f.alpha * blizzard_intensity);
                AEGfxSetColorToMultiply(0.8f, 0.92f, 1.0f, 1.0f);
                mesh_renderer(s_quad, f.x, f.y, f.size, f.size, 0.0f);
            }
        }

        // Reset render state
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    static void draw_centred_text(const char* text, float ndc_y, float scale,
        float r, float g, float b, float alpha)
    {
        if (Graphics::g_font_id < 0) return;
        float w, h;
        AEGfxGetPrintSize(Graphics::g_font_id, text, scale, &w, &h);
        AEGfxPrint(Graphics::g_font_id, text, -w * 0.5f, ndc_y, scale, r, g, b, alpha);
    }

    static void draw_freeze_hud()
    {
        if (Graphics::g_font_id < 0) return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        draw_centred_text("YOU ARE FROZEN!", 0.10f, 0.75f, 0.6f, 0.88f, 1.0f, 1.0f);
        draw_centred_text("SPAM SPACE TO BREAK FREE!", -0.10f, 0.55f, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    static void draw_progress_bar()
    {
        float progress = Clamp01((float)s_space_presses / (float)SPACES_NEEDED);
        constexpr float BAR_BORDER = 4.0f;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Background (dark, slightly blue)
        AEGfxSetTransparency(0.85f);
        AEGfxSetColorToMultiply(0.1f, 0.15f, 0.25f, 1.0f);
        mesh_renderer(s_quad, 0.0f, BAR_Y, BAR_W + BAR_BORDER * 2.0f, BAR_H + BAR_BORDER * 2.0f, 0.0f);

        // Bar fill
        if (progress > 0.001f)
        {
            float fill_w = (BAR_W - BAR_BORDER * 2.0f) * progress;
            float fill_start_x = -(BAR_W * 0.5f) + BAR_BORDER + fill_w * 0.5f;

            // Lerp colour from ice blue (0% warm) to warm orange (100% warm)
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.3f + 0.7f * progress,
                0.5f + 0.1f * progress,
                1.0f - 0.8f * progress, 1.0f);
            mesh_renderer(s_quad, fill_start_x, BAR_Y, fill_w, BAR_H - BAR_BORDER * 2.0f, 0.0f);
        }

        // Label above bar
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        float bar_y_ndc = (BAR_Y + (BAR_H * 0.5f) + 20.0f) / HALF_H;
        draw_centred_text("WARMTH", bar_y_ndc, 0.4f, 0.9f, 0.95f, 1.0f, 1.0f);

        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // ================================================================
    //  Rain helpers
    // ================================================================

    static void spawn_raindrop(Raindrop& r)
    {
        r.x = RandF(-HALF_W, HALF_W);
        r.y = HALF_H + RandF(0.0f, 80.0f);
        r.speed_y = RandF(RAIN_SPEED_MIN, RAIN_SPEED_MAX);
        r.w = RandF(RAIN_W_MIN, RAIN_W_MAX);
        r.h = RandF(RAIN_H_MIN, RAIN_H_MAX);
        r.alpha = RandF(0.35f, 0.75f);
    }

    static void update_raindrop(Raindrop& r, float dt)
    {
        r.y -= r.speed_y * dt;
        r.x += RAIN_DRIFT * dt;   // wind-driven diagonal

        // Wrap horizontally
        if (r.x > HALF_W) r.x -= SCREEN_W;

        // Respawn when below screen
        if (r.y < -HALF_H - 30.0f) spawn_raindrop(r);
    }

    static void thunder_draw_rain()
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        for (int i = 0; i < RAIN_COUNT; ++i)
        {
            const Raindrop& r = s_rain[i];
            AEGfxSetTransparency(r.alpha);
            AEGfxSetColorToMultiply(0.6f, 0.75f, 0.95f, 1.0f);   // cool blue-grey
            mesh_renderer(s_quad, r.x, r.y, r.w, r.h, 0.0f);
        }

        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // ================================================================
    //  Thunder helpers
    // ================================================================

    static void thunder_start_storm()
    {
        s_state = WeatherState::FREEZING;  // reuse FREEZING as "storm active" sentinel
        s_storm_timer = STORM_DURATION;
        s_bolt_timer = BOLT_INTERVAL * 0.4f;    // first bolt comes faster to grab attention
        s_bolt_phase = BoltPhase::WAITING;
        s_bolt_phase_timer = 0.0f;
        s_stun_timer = 0.0f;
        s_strike_flash = 0.0f;
        s_lunge_enemy_idx = -1;
        s_lunge_timer = 0.0f;
        SoundManager::play_rain_ambient();
    }

    static void thunder_fire_bolt(std::vector<Data::Enemy>& enemy_pool)
    {
        static int candidates[64];
        int count = 0;
        for (int i = 0; i < (int)enemy_pool.size() && count < 64; ++i)
        {
            const Data::Enemy& e = enemy_pool[(size_t)i];
            if (e.enemy_is_active && !e.enemy_is_dying) candidates[count++] = i;
        }
        if (count == 0) return;

        int chosen = candidates[rand() % count];
        Data::Enemy& target = enemy_pool[(size_t)chosen];

        s_lunge_enemy_idx = chosen;
        s_lunge_orig_speed = target.enemy_fall_speed_units_per_second;
        s_lunge_timer = LUNGE_DUR;
        target.enemy_fall_speed_units_per_second *= LUNGE_SPEED_MUL;

        s_t_strike = { true, target.enemy_position_x, target.enemy_position_y, 0, 0.0f };

        s_stun_timer = STUN_DUR;
        s_strike_flash = 1.0f;
        SoundManager::play_lightning_sfx();
    }

    static void thunder_restore_lunge(std::vector<Data::Enemy>& enemy_pool)
    {
        if (s_lunge_enemy_idx < 0 || s_lunge_enemy_idx >= (int)enemy_pool.size()) { s_lunge_enemy_idx = -1; s_lunge_timer = 0.0f; return; }
        Data::Enemy& e = enemy_pool[(size_t)s_lunge_enemy_idx];
        if (e.enemy_is_active) e.enemy_fall_speed_units_per_second = s_lunge_orig_speed;
        s_lunge_enemy_idx = -1;
        s_lunge_timer = 0.0f;
    }

    static void thunder_update(float dt, std::vector<Data::Enemy>& enemy_pool)
    {
        s_storm_timer -= dt;
        if (s_storm_timer <= 0.0f)
        {
            thunder_restore_lunge(enemy_pool);
            s_stun_timer = 0.0f;
            s_strike_flash = 0.0f;
            s_state = WeatherState::IDLE;
            reset_trigger_timer();
            SoundManager::stop_rain_ambient();
            return;
        }

        if (s_stun_timer > 0.0f) s_stun_timer -= dt;

        if (s_strike_flash > 0.0f) s_strike_flash -= dt / STRIKE_FLASH_DUR;
        if (s_strike_flash < 0.0f) s_strike_flash = 0.0f;

        if (s_lunge_timer > 0.0f)
        {
            s_lunge_timer -= dt;
            if (s_lunge_timer <= 0.0f) thunder_restore_lunge(enemy_pool);
        }

        // Also restore lunge if struck enemy was killed mid-lunge
        if (s_lunge_enemy_idx >= 0 && s_lunge_enemy_idx < (int)enemy_pool.size())
        {
            const Data::Enemy& e = enemy_pool[(size_t)s_lunge_enemy_idx];
            if (!e.enemy_is_active || e.enemy_is_dying) thunder_restore_lunge(enemy_pool);
        }

        // ---- Bolt phase state machine ----
        s_bolt_phase_timer += dt;

        switch (s_bolt_phase)
        {
        case BoltPhase::WAITING:
            s_bolt_timer -= dt;
            if (s_bolt_timer <= 0.0f)
            {
                s_bolt_phase = BoltPhase::WARNING;
                s_bolt_phase_timer = 0.0f;
            }
            break;

        case BoltPhase::WARNING:
            if (s_bolt_phase_timer >= BRACE_WARNING_DUR)
            {
                thunder_fire_bolt(enemy_pool);
                s_bolt_phase = BoltPhase::STRIKING;
                s_bolt_phase_timer = 0.0f;
            }
            break;

        case BoltPhase::STRIKING:
            if (s_stun_timer <= 0.0f)
            {
                s_bolt_phase = BoltPhase::WAITING;
                s_bolt_phase_timer = 0.0f;
                s_bolt_timer = BOLT_INTERVAL;
            }
            break;
        }
    }

    static void thunder_draw_hud()
    {
        if (Graphics::g_font_id < 0) return;
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        if (s_bolt_phase == BoltPhase::WARNING)
        {
            float progress = Clamp01(s_bolt_phase_timer / BRACE_WARNING_DUR);
            float blink_hz = 2.0f + progress * 6.0f;
            float pulse = ((int)(s_bolt_phase_timer * blink_hz * 2.0f) % 2 == 0) ? 1.0f : 0.4f;
            draw_centred_text("BRACE!", 0.0f, 1.0f, 1.0f, 0.85f, 0.1f, pulse);
        }
        else if (s_bolt_phase == BoltPhase::STRIKING && s_stun_timer > 0.0f)
        {
            float alpha = Clamp01(s_stun_timer / STUN_DUR);
            draw_centred_text("STUNNED!", 0.0f, 1.2f, 1.0f, 0.3f, 0.1f, alpha);
        }
    }

    static void thunder_draw_strike_flash()
    {
        if (s_strike_flash <= 0.001f) return;
        draw_fullscreen_overlay(1.0f, 1.0f, 0.85f, s_strike_flash * 0.75f);
    }

    static void thunder_draw_strike_anim()
    {
        if (!s_t_strike.active || !s_t_strike_tex || !s_t_strike_mesh) return;

        float u_offset = s_t_strike.frame * TSTRIKE_UV_W;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_ADD);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_t_strike_tex, u_offset, 0.0f);
        mesh_renderer(s_t_strike_mesh, s_t_strike.x, s_t_strike.y,
            TSTRIKE_DISPLAY_W, TSTRIKE_DISPLAY_H, 0.0f);

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // ================================================================
    //  Rainbow helpers
    // ================================================================

    static void rainbow_build_mesh()
    {
        if (s_rainbow_mesh) { AEGfxMeshFree(s_rainbow_mesh); s_rainbow_mesh = nullptr; }
        // intentionally empty - per-frame meshes created in Rainbow_DrawSprite
    }

    static void rainbow_start()
    {
        s_state = WeatherState::FREEZING;
        s_rainbow_timer = RAINBOW_DURATION;
        s_rainbow_powerup_timer = RAINBOW_POWERUP_INTERVAL;
        s_rainbow_frame = 0;
        s_rainbow_frame_timer = 0.0f;
        s_rainbow_popup_timer = WEATHER_POPUP_DUR;
    }

    static void rainbow_update(float dt)
    {
        s_rainbow_timer -= dt;
        if (s_rainbow_timer <= 0.0f)
        {
            PowerUpSystem::set_drop_spawn_interval(15.0f, 30.0f);
            s_state = WeatherState::IDLE;
            reset_trigger_timer();
            return;
        }

        s_rainbow_frame_timer += dt;
        if (s_rainbow_frame_timer >= RAINBOW_FRAME_TIME)
        {
            s_rainbow_frame_timer -= RAINBOW_FRAME_TIME;
            s_rainbow_frame = (s_rainbow_frame + 1) % RAINBOW_FRAME_COUNT;
        }

        s_rainbow_powerup_timer -= dt;
        if (s_rainbow_powerup_timer <= 0.0f)
        {
            s_rainbow_powerup_timer = RAINBOW_POWERUP_INTERVAL;
            PowerUpSystem::set_drop_spawn_interval(0.01f, 0.05f);
        }
    }

    static void rainbow_draw_sprite()
    {
        if (!s_rainbow_tex) return;

        float elapsed = RAINBOW_DURATION - s_rainbow_timer;
        float fade_in = Clamp01(elapsed / RAINBOW_FADE_TIME);
        float fade_out = Clamp01(s_rainbow_timer / RAINBOW_FADE_TIME);
        float alpha = fade_in * fade_out;
        if (alpha <= 0.001f) return;

        constexpr float uv_w = 1.0f / (float)RAINBOW_FRAME_COUNT;
        float u_min = (float)s_rainbow_frame * uv_w;
        float u_max = u_min + uv_w;
        AEGfxVertexList* frame_mesh = Spritesheet_Mesh(1.0f, 1.0f, u_min, 0.0f, u_max, 1.0f);
        if (!frame_mesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_rainbow_tex, 0.0f, 0.0f);
        mesh_renderer(frame_mesh, 0.0f, RAINBOW_Y, RAINBOW_DISPLAY_W, RAINBOW_DISPLAY_H, 0.0f);

        AEGfxMeshFree(frame_mesh);

        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    static void rainbow_draw_hud()
    {
        if (s_rainbow_popup_timer <= 0.0f || Graphics::g_font_id < 0) return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        float alpha = (s_rainbow_popup_timer < WEATHER_POPUP_FADE)
            ? (s_rainbow_popup_timer / WEATHER_POPUP_FADE) : 1.0f;

        constexpr float HUE_SPEED = 2.5f;
        float h = fmodf(s_weather_pulse_timer * HUE_SPEED, 6.0f);
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
        default: r = 1.0f;   g = 0.0f;     b = 1.0f - f; break;
        }

        constexpr float BASE_SCALE = 0.75f;
        constexpr float PULSE_AMP = 0.08f;
        float scale = BASE_SCALE + PULSE_AMP * sinf(s_weather_pulse_timer * 4.0f);

        draw_centred_text("POWER UPS GALORE!", 0.0f, scale, r, g, b, alpha);
    }

    // ================================================================
    //  Ghost weather helpers
    // ================================================================

    static void ghost_build_mesh()
    {
        if (s_ghost_mesh) { AEGfxMeshFree(s_ghost_mesh); s_ghost_mesh = nullptr; }
    }

    static void ghost_spawn_one(Ghost& g, int digit)
    {
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

        float angle = RandF(0.0f, 6.2832f);
        float speed = RandF(GHOST_SPEED_MIN, GHOST_SPEED_MAX);
        g.vel_x = cosf(angle) * speed;
        g.vel_y = sinf(angle) * speed;
        g.digit = digit;
        g.alive = true;
        g.frame = 0;
        g.frame_timer = 0.0f;
    }

    static void ghost_start()
    {
        s_state = WeatherState::FREEZING;
        reset_trigger_timer();

        s_ghost_event_active = true;
        s_ghost_life_timer = 0.0f;
        s_ghost_lock_timer = 0.0f;
        s_ghost_popup_timer = 0.0f;

        s_ghost_announce_pending = true;
        s_ghost_announce_timer = GHOST_ANNOUNCE_DUR;

        for (int i = 0; i < GHOST_COUNT; ++i)
            s_ghosts[i].alive = false;
    }

    static void ghost_spawn_all()
    {
        int digits[GHOST_COUNT] = { 5, 6, 7, 8, 9 };
        // Fisher-Yates shuffle
        for (int i = GHOST_COUNT - 1; i > 0; --i)
        {
            int j = rand() % (i + 1);
            int tmp = digits[i]; digits[i] = digits[j]; digits[j] = tmp;
        }
        for (int i = 0; i < GHOST_COUNT; ++i)
            ghost_spawn_one(s_ghosts[i], digits[i]);
    }

    static void ghost_update(float dt)
    {
        // ---- Announcement phase: wait 3s before spawning ghosts ----
        if (s_ghost_announce_pending)
        {
            s_ghost_announce_timer -= dt;
            if (s_ghost_announce_timer <= 0.0f)
            {
                s_ghost_announce_pending = false;
                s_ghost_announce_timer = 0.0f;
                ghost_spawn_all();
            }
            return;
        }

        // ---- Lock timer ----
        if (s_ghost_lock_timer > 0.0f)
        {
            s_ghost_lock_timer -= dt;
            if (s_ghost_lock_timer < 0.0f) s_ghost_lock_timer = 0.0f;
        }

        // ---- Count still-alive ghosts ----
        int alive = 0;
        for (int i = 0; i < GHOST_COUNT; ++i)
            if (s_ghosts[i].alive) ++alive;

        // Event ends when all ghosts are dead and no lock is pending
        if (alive == 0 && s_ghost_lock_timer <= 0.0f)
        {
            s_ghost_event_active = false;
            s_state = WeatherState::IDLE;
            reset_trigger_timer();
            return;
        }

        // ---- Life timer: triggers lock if a ghost survives too long ----
        if (alive > 0)
        {
            s_ghost_life_timer += dt;
            if (s_ghost_life_timer >= GHOST_LIFETIME && s_ghost_lock_timer <= 0.0f)
            {
                s_ghost_lock_timer = GHOST_LOCK_DUR;
                s_ghost_life_timer = 0.0f;
            }
        }

        // ---- Move and animate live ghosts ----
        for (int i = 0; i < GHOST_COUNT; ++i)
        {
            Ghost& g = s_ghosts[i];
            if (!g.alive) continue;

            g.x += g.vel_x * dt;
            g.y += g.vel_y * dt;

            // Bounce off screen edges
            if (g.x < -HALF_W) { g.x = -HALF_W; g.vel_x = fabsf(g.vel_x); }
            if (g.x > HALF_W) { g.x = HALF_W; g.vel_x = -fabsf(g.vel_x); }
            if (g.y < -HALF_H) { g.y = -HALF_H; g.vel_y = fabsf(g.vel_y); }
            if (g.y > HALF_H) { g.y = HALF_H; g.vel_y = -fabsf(g.vel_y); }

            g.frame_timer += dt;
            if (g.frame_timer >= GHOST_FRAME_TIME)
            {
                g.frame_timer -= GHOST_FRAME_TIME;
                g.frame = (g.frame + 1) % GHOST_FRAME_COUNT;
            }
        }

        // ---- Check digit input (only when typing is not locked) ----
        if (s_ghost_lock_timer <= 0.0f)
        {
            for (int key = 5; key <= 9; ++key)
            {
                u8 vk = (u8)(0x30 + key);
                if (AEInputCheckTriggered(vk))
                {
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

    static void ghost_draw()
    {
        if (!s_ghost_tex) return;

        constexpr float uv_w = 1.0f / (float)GHOST_FRAME_COUNT;

        for (int i = 0; i < GHOST_COUNT; ++i)
        {
            const Ghost& g = s_ghosts[i];
            if (!g.alive) continue;

            float u_min = (float)g.frame * uv_w;
            float u_max = u_min + uv_w;
            AEGfxVertexList* frame_mesh = Spritesheet_Mesh(1.0f, 1.0f, u_min, 0.0f, u_max, 1.0f);
            if (!frame_mesh) continue;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
            AEGfxTextureSet(s_ghost_tex, 0.0f, 0.0f);
            mesh_renderer(frame_mesh, g.x, g.y, GHOST_SIZE, GHOST_SIZE, 0.0f);

            AEGfxMeshFree(frame_mesh);

            // Draw digit label above the ghost
            if (Graphics::g_font_id >= 0)
            {
                char label[2] = { (char)('0' + g.digit), '\0' };
                constexpr float DIGIT_SCALE = 0.6f;

                float hw = (float)AEGfxGetWindowWidth() * 0.5f;
                float hh = (float)AEGfxGetWindowHeight() * 0.5f;
                float ndc_x = (hw > 0.0f) ? (g.x / hw) : 0.0f;
                float ndc_y = (hh > 0.0f) ? ((g.y + GHOST_LABEL_OFFSET_Y) / hh) : 0.0f;

                float tw = 0.0f, th = 0.0f;
                AEGfxGetPrintSize(Graphics::g_font_id, label, DIGIT_SCALE, &tw, &th);
                ndc_x -= tw * 0.5f;

                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxPrint(Graphics::g_font_id, label, ndc_x, ndc_y, DIGIT_SCALE, 1.0f, 0.95f, 0.2f, 1.0f);
            }
        }

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    }

    static void ghost_draw_hud()
    {
        if (Graphics::g_font_id < 0) return;

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // "DEFEAT GHOSTS (PRESS NUMBERS)" - shown for 3s before ghosts spawn
        if (s_ghost_announce_pending && s_ghost_announce_timer > 0.0f)
        {
            float alpha = (s_ghost_announce_timer < WEATHER_POPUP_FADE)
                ? (s_ghost_announce_timer / WEATHER_POPUP_FADE) : 1.0f;
            float brightness = fabsf(sinf(s_weather_pulse_timer * 2.5f));
            float scale = 0.65f + 0.08f * sinf(s_weather_pulse_timer * 3.5f);
            draw_centred_text("DEFEAT GHOSTS (PRESS NUMBERS)", 0.0f, scale,
                brightness, brightness, brightness, alpha);
        }

        // "GHOSTIFIED!" lock indicator
        if (s_ghost_lock_timer > 0.0f)
        {
            float pulse = ((int)(s_ghost_lock_timer * 6.0f) % 2 == 0) ? 1.0f : 0.55f;
            draw_centred_text("GHOSTIFIED!", 0.10f, 0.85f, 1.0f, 0.15f, 0.15f, pulse);

            char buf[16];
            snprintf(buf, sizeof(buf), "%d", (int)ceilf(s_ghost_lock_timer));
            draw_centred_text(buf, -0.05f, 0.65f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
}