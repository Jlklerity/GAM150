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

#include "Weather.h"
#include "AEEngine.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Enemy.h"        // Graphics::font_id, Data::Enemy

#include <vector>
#include <cstdlib>        // rand()
#include <cmath>          // fabsf


// ============================================================
//  Internal constants
// ============================================================
namespace
{
    // ---- Screen dimensions (world space) ----
    constexpr float SCREEN_W = 1600.0f;
    constexpr float SCREEN_H = 900.0f;
    constexpr float HALF_W = 800.0f;
    constexpr float HALF_H = 450.0f;

    // ---- Trigger timing ----
    constexpr float TRIGGER_MIN_SEC = 25.0f;   // earliest a blizzard can strike
    constexpr float TRIGGER_MAX_SEC = 50.0f;   // latest a blizzard can strike

    // ---- Phase durations ----
    constexpr float FREEZE_IN_DUR = 1.8f;   // ramp-up duration (FREEZING phase)
    constexpr float THAW_DUR = 1.2f;   // fade-out duration (THAWING phase)

    // ---- Spacebar break-free mechanic ----
    constexpr int   SPACES_NEEDED = 15;       // presses to escape

    // ---- Particle counts ----
    constexpr int   AMBIENT_COUNT = 80;       // gentle ambient snowflakes
    constexpr int   BLIZZARD_COUNT = 140;      // extra heavy particles during blizzard

    // ---- Ambient particle speed range (px/s, falling downward) ----
    constexpr float AMB_SPEED_MIN = 60.0f;
    constexpr float AMB_SPEED_MAX = 160.0f;

    // ---- Blizzard particle speed range ----
    constexpr float BLZ_SPEED_MIN = 180.0f;
    constexpr float BLZ_SPEED_MAX = 380.0f;

    // ---- Particle size range (world px) ----
    constexpr float FLAKE_SIZE_MIN = 3.0f;
    constexpr float FLAKE_SIZE_MAX = 10.0f;

    // ---- Horizontal drift per second ----
    constexpr float DRIFT_RANGE = 30.0f;   // max horizontal speed

    // ---- Rain particle constants ----
    constexpr int   RAIN_COUNT = 200;      // number of raindrops
    constexpr float RAIN_SPEED_MIN = 500.0f;   // fast downward fall (px/s)
    constexpr float RAIN_SPEED_MAX = 800.0f;
    constexpr float RAIN_DRIFT = 80.0f;   // fixed rightward drift (wind-driven)
    constexpr float RAIN_W_MIN = 2.0f;   // thin streak width
    constexpr float RAIN_W_MAX = 3.0f;
    constexpr float RAIN_H_MIN = 14.0f;   // tall streak height
    constexpr float RAIN_H_MAX = 24.0f;

    // ---- Overlay tint (RGBA 0-1) ----
    constexpr float OVL_R = 0.55f;
    constexpr float OVL_G = 0.75f;
    constexpr float OVL_B = 1.00f;
    constexpr float OVL_ALPHA_MAX = 0.55f;   // max overlay opacity when fully frozen

    // ---- Progress bar dimensions (world px) ----
    constexpr float BAR_W = 400.0f;
    constexpr float BAR_H = 28.0f;
    constexpr float BAR_Y = -340.0f;  // vertical position

    // ---- Vignette flash when a space is pressed ----
    constexpr float FLASH_DUR = 0.08f;

    // ================================================================
    //  Thunderstorm constants
    // ================================================================

    // How long the storm session lasts once triggered (seconds)
    constexpr float STORM_DURATION = 20.0f;

    // Time between consecutive bolt warnings inside a storm
    constexpr float BOLT_INTERVAL = 5.0f;

    // "BRACE!" warning shown this many seconds before the strike
    constexpr float BRACE_WARNING_DUR = 1.0f;

    // White-flash duration on strike
    constexpr float STRIKE_FLASH_DUR = 0.15f;

    // Input stun duration on strike
    constexpr float STUN_DUR = 0.5f;

    // How long the struck enemy lunges at boosted speed (seconds)
    constexpr float LUNGE_DUR = 2.5f;

    // Speed multiplier applied to the struck enemy during lunge
    constexpr float LUNGE_SPEED_MUL = 3.5f;

    // Dark storm overlay max alpha
    constexpr float STORM_OVL_ALPHA_MAX = 0.25f;

    // Storm overlay colour (dark grey-purple)
    constexpr float STORM_OVL_R = 0.10f;
    constexpr float STORM_OVL_G = 0.08f;
    constexpr float STORM_OVL_B = 0.18f;

    // ---- Inline helpers ----
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
    enum class WeatherType
    {
        SNOW,
        THUNDER
    };

    // Weather state machine (shared by both types)
    enum class WeatherState
    {
        IDLE,       // no event active, gentle ambient snow
        FREEZING,   // blizzard building up (FREEZE_IN_DUR seconds)
        FROZEN,     // player is fully frozen – must spam SPACE
        THAWING     // event over, overlay fading out
    };

    // Thunder-specific bolt phases (runs inside a storm session)
    enum class BoltPhase
    {
        WAITING,    // counting down to next bolt
        WARNING,    // "BRACE!" shown, bolt imminent
        STRIKING,   // white flash + stun active
    };

    // A single snow particle
    struct Snowflake
    {
        float x, y;
        float speedY;
        float driftX;
        float size;
        float alpha;
        bool  isBlizzard;
    };

    // ---- Shared state ----
    static WeatherState   s_state = WeatherState::IDLE;
    static WeatherType    s_activeType = WeatherType::SNOW;   // set when event triggers
    static float          s_triggerTimer = 0.0f;
    static float          s_triggerTarget = 0.0f;
    static float          s_phaseTimer = 0.0f;

    // ---- Snow-specific state ----
    static int            s_spacePresses = 0;
    static float          s_flashTimer = 0.0f;
    static bool           s_spaceWasHeld = false;

    // ---- Thunder-specific state ----
    static float          s_stormTimer = 0.0f;    // total time remaining in storm
    static float          s_boltTimer = 0.0f;    // time until next bolt (counts down)
    static BoltPhase      s_boltPhase = BoltPhase::WAITING;
    static float          s_boltPhaseTimer = 0.0f;   // timer within current bolt phase
    static float          s_stunTimer = 0.0f;    // input stun remaining
    static float          s_strikeFlash = 0.0f;    // white-flash alpha (fades out)
    static int            s_lungeEnemyIdx = -1;       // index of enemy currently lunging
    static float          s_lungeTimer = 0.0f;    // lunge duration remaining
    static float          s_lungeOrigSpeed = 0.0f;   // original speed before lunge boost

    // ---- Particles ----
    static Snowflake      s_ambient[AMBIENT_COUNT]{};
    static Snowflake      s_blizzard[BLIZZARD_COUNT]{};

    // Rain particle (thin tall streak)
    struct Raindrop
    {
        float x, y;
        float speedY;
        float w, h;
        float alpha;
    };
    static Raindrop s_rain[RAIN_COUNT]{};

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
    static void  DrawOverlay(float intensity);
    static void  Snow_DrawParticles(float blizzardIntensity);  // snow-only particles
    static void  DrawFreezeHUD();
    static void  DrawProgressBar();

    // Rain
    static void  SpawnRaindrop(Raindrop& r);
    static void  UpdateRaindrop(Raindrop& r, float dt);
    static void  Thunder_DrawRain();                           // rain-only particles

    // Thunder helpers
    static void  Thunder_StartStorm();
    static void  Thunder_Update(float dt, std::vector<Data::Enemy>& enemy_pool);
    static void  Thunder_FireBolt(std::vector<Data::Enemy>& enemy_pool);
    static void  Thunder_RestoreLunge(std::vector<Data::Enemy>& enemy_pool);
    static void  Thunder_DrawOverlay();
    static void  Thunder_DrawHUD();
    static void  Thunder_DrawStrikeFlash();
}


// ============================================================
//  Public interface  (flat functions matching project convention)
// ============================================================

void Weather_Init()
{
    if (s_quad) { AEGfxMeshFree(s_quad); s_quad = nullptr; }
    s_quad = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

    // ---- Shared reset ----
    s_state = WeatherState::IDLE;
    s_phaseTimer = 0.0f;

    // ---- Snow reset ----
    s_spacePresses = 0;
    s_flashTimer = 0.0f;
    s_spaceWasHeld = false;

    // ---- Thunder reset ----
    s_stormTimer = 0.0f;
    s_boltTimer = 0.0f;
    s_boltPhase = BoltPhase::WAITING;
    s_boltPhaseTimer = 0.0f;
    s_stunTimer = 0.0f;
    s_strikeFlash = 0.0f;
    s_lungeEnemyIdx = -1;
    s_lungeTimer = 0.0f;
    s_lungeOrigSpeed = 0.0f;

    ResetTriggerTimer();

    for (int i = 0; i < AMBIENT_COUNT; ++i)
    {
        SpawnAmbientFlake(s_ambient[i]);
        s_ambient[i].y = RandF(-HALF_H, HALF_H);  // scatter on startup
    }
    for (int i = 0; i < BLIZZARD_COUNT; ++i)
        SpawnBlizzardFlake(s_blizzard[i]);

    // Scatter rain across full screen on init (invisible until thunder triggers)
    for (int i = 0; i < RAIN_COUNT; ++i)
    {
        SpawnRaindrop(s_rain[i]);
        s_rain[i].y = RandF(-HALF_H, HALF_H);  // scatter vertically
    }
}

// ----------------------------------------------------------------
void Weather_Update(float dt, std::vector<Data::Enemy>& enemy_pool)
{
    // ---- Advance trigger timer (IDLE only) ----
    if (s_state == WeatherState::IDLE)
    {
        s_triggerTimer += dt;
        if (s_triggerTimer >= s_triggerTarget)
        {
            // Randomly pick SNOW or THUNDER (50/50)
            s_activeType = ((rand() % 2) == 0) ? WeatherType::SNOW : WeatherType::THUNDER;

            if (s_activeType == WeatherType::SNOW)
            {
                s_state = WeatherState::FREEZING;
                s_phaseTimer = 0.0f;
                s_spacePresses = 0;
            }
            else
            {
                Thunder_StartStorm();
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
            if (s_phaseTimer >= FREEZE_IN_DUR)
            {
                s_state = WeatherState::FROZEN;
                s_phaseTimer = 0.0f;
            }
        }

        // FROZEN phase – count SPACE presses
        if (s_state == WeatherState::FROZEN)
        {
            bool spaceHeld = (AEInputCheckCurr(AEVK_SPACE) != 0);
            bool spaceEdge = spaceHeld && !s_spaceWasHeld;
            s_spaceWasHeld = spaceHeld;

            if (spaceEdge)
            {
                ++s_spacePresses;
                s_flashTimer = FLASH_DUR;
                if (s_spacePresses >= SPACES_NEEDED)
                {
                    s_state = WeatherState::THAWING;
                    s_phaseTimer = 0.0f;
                }
            }
            if (s_flashTimer > 0.0f) s_flashTimer -= dt;
        }

        // THAWING phase
        if (s_state == WeatherState::THAWING)
        {
            s_phaseTimer += dt;
            if (s_phaseTimer >= THAW_DUR)
            {
                s_state = WeatherState::IDLE;
                ResetTriggerTimer();
            }
        }
    }

    // ================================================================
    //  THUNDER path
    // ================================================================
    if (s_activeType == WeatherType::THUNDER && s_state != WeatherState::IDLE)
    {
        Thunder_Update(dt, enemy_pool);
    }

    // ---- Update particles (each type only runs during its weather) ----
    if (s_activeType == WeatherType::SNOW && s_state != WeatherState::IDLE)
    {
        for (int i = 0; i < AMBIENT_COUNT; ++i)
            UpdateFlake(s_ambient[i], dt, 1.0f);

        float blzMul = 0.3f + GetBlizzardIntensity() * 1.4f;
        for (int i = 0; i < BLIZZARD_COUNT; ++i)
            UpdateFlake(s_blizzard[i], dt, blzMul);
    }

    if (s_activeType == WeatherType::THUNDER && s_state != WeatherState::IDLE)
    {
        for (int i = 0; i < RAIN_COUNT; ++i)
            UpdateRaindrop(s_rain[i], dt);
    }
}

// ----------------------------------------------------------------
void Weather_Draw()
{
    if (!s_quad) return;

    // No particles at all when idle
    if (s_state == WeatherState::IDLE) return;

    float intensity = GetBlizzardIntensity();

    if (s_activeType == WeatherType::SNOW)
    {
        Snow_DrawParticles(intensity);
        DrawOverlay(intensity);
        if (s_state == WeatherState::FROZEN)
        {
            DrawFreezeHUD();
            DrawProgressBar();
        }
    }
    else // THUNDER
    {
        Thunder_DrawRain();
        Thunder_DrawOverlay();
        Thunder_DrawHUD();
        Thunder_DrawStrikeFlash();
    }
}

// ----------------------------------------------------------------
void Weather_Exit()
{
    if (s_quad)
    {
        AEGfxMeshFree(s_quad);
        s_quad = nullptr;
    }
}

// ----------------------------------------------------------------
bool Weather_IsPlayerFrozen()
{
    return s_state == WeatherState::FROZEN;
}

// ----------------------------------------------------------------
bool Weather_IsPlayerStunned()
{
    return (s_activeType == WeatherType::THUNDER && s_stunTimer > 0.0f);
}

// ----------------------------------------------------------------
void Weather_ForceEvent(int type)
{
    // Cancel any active event first — reset to a clean IDLE state
    s_state = WeatherState::IDLE;
    s_phaseTimer = 0.0f;
    s_spacePresses = 0;
    s_flashTimer = 0.0f;
    s_spaceWasHeld = false;
    s_stunTimer = 0.0f;
    s_strikeFlash = 0.0f;
    // Note: lunging enemy speed is NOT restored here since we have no pool reference.
    // The lunge will self-correct on the next Update() call via Thunder_RestoreLunge.
    s_lungeEnemyIdx = -1;
    s_lungeTimer = 0.0f;

    if (type == 0)
    {
        // Force snow blizzard: skip IDLE, jump straight to FREEZING
        s_activeType = WeatherType::SNOW;
        s_state = WeatherState::FREEZING;
        s_phaseTimer = 0.0f;
    }
    else
    {
        // Force thunderstorm
        s_activeType = WeatherType::THUNDER;
        Thunder_StartStorm();
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

    // Move one flake; respawn when it exits the bottom
    static void UpdateFlake(Snowflake& f, float dt, float speedMul)
    {
        f.y -= f.speedY * speedMul * dt;
        f.x += f.driftX * speedMul * dt;

        // Wrap horizontally so flakes don't disappear off the sides
        if (f.x < -HALF_W) f.x += SCREEN_W;
        if (f.x > HALF_W) f.x -= SCREEN_W;

        // Respawn when below the screen
        if (f.y < -HALF_H - 20.0f)
        {
            if (f.isBlizzard)
                SpawnBlizzardFlake(f);
            else
                SpawnAmbientFlake(f);
        }
    }

    // Returns a 0..1 intensity for the blizzard based on state + phaseTimer
    static float GetBlizzardIntensity()
    {
        // Thunder doesn't use snow blizzard particles
        if (s_activeType == WeatherType::THUNDER) return 0.0f;

        switch (s_state)
        {
        case WeatherState::IDLE:
            return 0.0f;
        case WeatherState::FREEZING:
            return Clamp01(s_phaseTimer / FREEZE_IN_DUR);
        case WeatherState::FROZEN:
            if (s_flashTimer > 0.0f)
                return 1.0f + 0.15f * (s_flashTimer / FLASH_DUR);
            return 1.0f;
        case WeatherState::THAWING:
            return Clamp01(1.0f - s_phaseTimer / THAW_DUR);
        }
        return 0.0f;
    }

    // Full-screen blue tinted semi-transparent rectangle
    static void DrawOverlay(float intensity)
    {
        float alpha = OVL_ALPHA_MAX * intensity;
        if (alpha <= 0.001f) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(OVL_R, OVL_G, OVL_B, 1.0f);

        Mesh_Renderer(s_quad, 0.0f, 0.0f, SCREEN_W, SCREEN_H, 0.0f);

        // Reset multiply colour
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(1.0f);
    }

    // Draw snow particles (ambient + blizzard burst) — snow weather only
    static void Snow_DrawParticles(float blizzardIntensity)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_ADD);   // additive = bright, glowy snow

        // Ambient snowflakes (always visible)
        for (int i = 0; i < AMBIENT_COUNT; ++i)
        {
            const Snowflake& f = s_ambient[i];
            float a = f.alpha;
            AEGfxSetTransparency(a);
            AEGfxSetColorToMultiply(0.85f, 0.93f, 1.0f, 1.0f);   // slight blue-white tint
            Mesh_Renderer(s_quad, f.x, f.y, f.size, f.size, 0.0f);
        }

        // Blizzard particles fade in with intensity
        if (blizzardIntensity > 0.01f)
        {
            for (int i = 0; i < BLIZZARD_COUNT; ++i)
            {
                const Snowflake& f = s_blizzard[i];
                float a = f.alpha * blizzardIntensity;
                AEGfxSetTransparency(a);
                AEGfxSetColorToMultiply(0.8f, 0.92f, 1.0f, 1.0f);
                Mesh_Renderer(s_quad, f.x, f.y, f.size, f.size, 0.0f);
            }
        }

        // Reset render state
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // "SPAM SPACE TO BREAK FREE!" prompt text
    static void DrawFreezeHUD()
    {
        if (Graphics::font_id < 0) return;

        float t = s_phaseTimer;
        float sinApprox = 0.75f + 0.25f * (((int)(t * 3.0f) % 2 == 0) ? 1.0f : -1.0f);

        const char* line1 = "YOU ARE FROZEN!";
        const char* line2 = "SPAM SPACE TO BREAK FREE!";

        float w1, h1, w2, h2;
        AEGfxGetPrintSize(Graphics::font_id, line1, 0.75f, &w1, &h1);
        AEGfxGetPrintSize(Graphics::font_id, line2, 0.55f, &w2, &h2);

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);

        // Main "FROZEN" title – icy blue
        AEGfxPrint(Graphics::font_id, line1,
            -w1 * 0.5f,          // centered NDC X
            0.35f,              // NDC Y (upper area)
            0.75f,              // scale
            0.6f, 0.88f, 1.0f, // RGB – icy blue
            sinApprox);         // alpha pulse

        // Instruction – white
        AEGfxPrint(Graphics::font_id, line2,
            -w2 * 0.5f,
            0.15f,
            0.55f,
            1.0f, 1.0f, 1.0f,
            sinApprox);
    }

    // Horizontal progress bar showing break-free progress
    static void DrawProgressBar()
    {
        float progress = Clamp01((float)s_spacePresses / (float)SPACES_NEEDED);

        float barY = BAR_Y;
        float barBorder = 4.0f;  // border thickness (px each side)

        // --- Background (dark, slightly blue) ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.85f);
        AEGfxSetColorToMultiply(0.1f, 0.15f, 0.25f, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, barY, BAR_W + barBorder * 2.0f, BAR_H + barBorder * 2.0f, 0.0f);

        // --- Fill (warm orange/yellow so it contrasts the cold overlay) ---
        if (progress > 0.001f)
        {
            float fillW = (BAR_W - barBorder * 2.0f) * progress;
            float fillStartX = -(BAR_W * 0.5f) + barBorder + fillW * 0.5f;

            // Lerp colour from ice blue (0% warm) to warm orange (100% warm)
            float r = 0.3f + 0.7f * progress;
            float g = 0.5f + 0.1f * progress;
            float b = 1.0f - 0.8f * progress;

            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(r, g, b, 1.0f);
            Mesh_Renderer(s_quad, fillStartX, barY, fillW, BAR_H - barBorder * 2.0f, 0.0f);
        }

        // --- Label above bar ---
        if (Graphics::font_id >= 0)
        {
            const char* label = "WARMTH";
            float lw, lh;
            AEGfxGetPrintSize(Graphics::font_id, label, 0.4f, &lw, &lh);

            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            float barYndc = (barY + (BAR_H * 0.5f) + 20.0f) / HALF_H;
            AEGfxPrint(Graphics::font_id, label, -lw * 0.5f, barYndc, 0.4f, 0.9f, 0.95f, 1.0f, 1.0f);
        }

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
        if (r.y < -HALF_H - 30.0f)
            SpawnRaindrop(r);
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
        s_state = WeatherState::FREEZING;  // re-use FREEZING as "storm active" sentinel
        s_stormTimer = STORM_DURATION;
        s_boltTimer = BOLT_INTERVAL * 0.4f;    // first bolt comes faster to grab attention
        s_boltPhase = BoltPhase::WAITING;
        s_boltPhaseTimer = 0.0f;
        s_stunTimer = 0.0f;
        s_strikeFlash = 0.0f;
        s_lungeEnemyIdx = -1;
        s_lungeTimer = 0.0f;
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
            if (e.enemy_is_active && !e.enemy_is_dying)
                candidates[count++] = i;
        }
        if (count == 0) return;  // no targets – skip this bolt

        int chosen = candidates[rand() % count];
        Data::Enemy& target = enemy_pool[(size_t)chosen];

        // Store original speed and apply boost
        s_lungeEnemyIdx = chosen;
        s_lungeOrigSpeed = target.enemy_fall_speed_units_per_second;
        s_lungeTimer = LUNGE_DUR;
        target.enemy_fall_speed_units_per_second *= LUNGE_SPEED_MUL;

        // Trigger stun + white flash
        s_stunTimer = STUN_DUR;
        s_strikeFlash = 1.0f;
    }

    // Restore lunging enemy's original speed when lunge expires
    static void Thunder_RestoreLunge(std::vector<Data::Enemy>& enemy_pool)
    {
        if (s_lungeEnemyIdx < 0) return;
        Data::Enemy& e = enemy_pool[(size_t)s_lungeEnemyIdx];
        // Only restore if still active (might have been killed)
        if (e.enemy_is_active)
            e.enemy_fall_speed_units_per_second = s_lungeOrigSpeed;
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
            if (s_lungeTimer <= 0.0f)
                Thunder_RestoreLunge(enemy_pool);
        }

        // Also restore lunge if struck enemy was killed mid-lunge
        if (s_lungeEnemyIdx >= 0)
        {
            const Data::Enemy& e = enemy_pool[(size_t)s_lungeEnemyIdx];
            if (!e.enemy_is_active || e.enemy_is_dying)
                Thunder_RestoreLunge(enemy_pool);
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

    // Dark stormy overlay — intensity based on remaining storm time
    static void Thunder_DrawOverlay()
    {
        // Fade in over first 2 seconds, constant while running, not used (storm ends abruptly)
        float elapsed = STORM_DURATION - s_stormTimer;
        float fadeIn = Clamp01(elapsed / 2.0f);
        float alpha = STORM_OVL_ALPHA_MAX * fadeIn;

        if (alpha <= 0.001f) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(STORM_OVL_R, STORM_OVL_G, STORM_OVL_B, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, 0.0f, SCREEN_W, SCREEN_H, 0.0f);

        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(1.0f);
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

            const char* brace = "BRACE!";
            float bw, bh;
            AEGfxGetPrintSize(Graphics::font_id, brace, 1.0f, &bw, &bh);
            AEGfxPrint(Graphics::font_id, brace,
                -bw * 0.5f, 0.25f, 1.0f,
                1.0f, 0.85f, 0.1f,   // yellow
                pulse);
        }
        else if (s_boltPhase == BoltPhase::STRIKING && s_stunTimer > 0.0f)
        {
            const char* stunText = "STUNNED!";
            float sw, sh;
            AEGfxGetPrintSize(Graphics::font_id, stunText, 0.65f, &sw, &sh);
            float alpha = Clamp01(s_stunTimer / STUN_DUR);
            AEGfxPrint(Graphics::font_id, stunText,
                -sw * 0.5f, 0.10f, 0.65f,
                1.0f, 0.3f, 0.1f,    // red-orange
                alpha);
        }
    }

    // Full-screen white flash on strike
    static void Thunder_DrawStrikeFlash()
    {
        if (s_strikeFlash <= 0.001f) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(s_strikeFlash * 0.75f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 0.85f, 1.0f);   // warm white/yellow flash
        Mesh_Renderer(s_quad, 0.0f, 0.0f, SCREEN_W, SCREEN_H, 0.0f);

        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetTransparency(1.0f);
    }
}