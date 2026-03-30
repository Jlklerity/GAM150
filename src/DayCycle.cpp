#include "pch.h"
#include "DayCycle.h"

// ---------------------------------------------------------------------------
// Tuning constants
// ---------------------------------------------------------------------------
static const float TRANSITION_DURATION = 5.0f;

static const float DAY_R = 0.53f, DAY_G = 0.81f, DAY_B = 0.92f;
static const float NIGHT_R = 0.04f, NIGHT_G = 0.04f, NIGHT_B = 0.15f;

static const float BODY_SIZE = 450.0f;
static const float BODY_X = 340.0f;
static const float BODY_Y = 190.0f;
static const float SUN_DRIFT = 30.0f;

static const float STARS_SPEED = 0.004f;

namespace
{
    bool  s_active = false;
    bool  s_is_night = false;
    bool  s_reverse_active = false;
    float s_timer = 0.0f;
    float s_night_alpha = 0.0f;
    float s_stars_offset = 0.0f;

    AEGfxTexture* s_sun_tex = nullptr;
    AEGfxTexture* s_moon_tex = nullptr;
    AEGfxTexture* s_stars_tex = nullptr;
    AEGfxVertexList* s_body_mesh = nullptr;
    AEGfxVertexList* s_bg_mesh = nullptr;
    AEGfxVertexList* s_stars_mesh = nullptr;

    // smooths out the transition between day and night so it doesn't look jarring,
    // takes t in range [0,1] and returns a smoothed value in the same range,
    // uses quadratic ease-in-out so the transition accelerates then decelerates
    float EaseInOut(float t)
    {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        return t < 0.5f ? 2.0f * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * 0.5f;
    }
}

namespace DayCycle
{
    // sets up all state, loads the sun/moon/stars textures and builds the meshes needed for rendering,
    // safe to call on restart since it unloads any previously loaded assets before reloading
    void Init()
    {
        s_active = false;
        s_is_night = false;
        s_reverse_active = false;
        s_timer = 0.0f;
        s_night_alpha = 0.0f;
        s_stars_offset = 0.0f;

        if (s_sun_tex) { AEGfxTextureUnload(s_sun_tex);   s_sun_tex = nullptr; }
        if (s_moon_tex) { AEGfxTextureUnload(s_moon_tex);  s_moon_tex = nullptr; }
        if (s_stars_tex) { AEGfxTextureUnload(s_stars_tex); s_stars_tex = nullptr; }
        if (s_body_mesh) { AEGfxMeshFree(s_body_mesh);      s_body_mesh = nullptr; }
        if (s_bg_mesh) { AEGfxMeshFree(s_bg_mesh);        s_bg_mesh = nullptr; }
        if (s_stars_mesh) { AEGfxMeshFree(s_stars_mesh);     s_stars_mesh = nullptr; }

        s_sun_tex = AEGfxTextureLoad("Assets/sun.png");
        AE_ASSERT_MESG(s_sun_tex, "DayCycle: failed to load sun.png");
        s_moon_tex = AEGfxTextureLoad("Assets/moon.png");
        AE_ASSERT_MESG(s_moon_tex, "DayCycle: failed to load moon.png");
        s_stars_tex = AEGfxTextureLoad("Assets/stars.png");
        AE_ASSERT_MESG(s_stars_tex, "DayCycle: failed to load stars.png");

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();

        s_body_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
        s_bg_mesh = Rect_Mesh(win_w, win_h, 0xFFFFFFFF);
        s_stars_mesh = Rect_Mesh(win_w, win_h, 0xFFFFFFFF);
    }

    // starts the day fo night transition, call this when the boss spawns
    // does nothing if a transition is already running or if it's already night
    void TriggerTransition()
    {
        if (s_active || s_is_night) return;
        s_active = true;
        s_timer = 0.0f;
    }

    // starts the night to day transition, call this when the boss is defeated,
    // does nothing if a reverse transition is already running
    void TriggerDayTransition()
    {
        if (s_reverse_active) return;
        s_reverse_active = true;
        s_is_night = false; // allow reverse to run even if not fully night
        s_timer = 0.0f;
    }

    // advances the transition timers and scrolls the star field every frame
    // param is dt, handles both day to night and night to day independently,
    // s_night_alpha is what controls how dark/bright everything looks 
    void Update(float dt)
    {
        // Scroll stars regardless
        s_stars_offset += STARS_SPEED * dt;
        if (s_stars_offset > 1.0f) s_stars_offset -= 1.0f;

        // Night transition (day -> night)
        if (s_active)
        {
            s_timer += dt;
            float t = s_timer / TRANSITION_DURATION;
            if (t >= 1.0f)
            {
                t = 1.0f;
                s_active = false;
                s_is_night = true;
            }
            s_night_alpha = EaseInOut(t);
        }

        // Day transition (night -> day)
        if (s_reverse_active)
        {
            s_timer += dt;
            float t = s_timer / TRANSITION_DURATION;
            if (t >= 1.0f)
            {
                t = 1.0f;
                s_reverse_active = false;
                s_is_night = false;
                s_night_alpha = 0.0f;
            }
            else
                s_night_alpha = EaseInOut(1.0f - t); // reverse: 1 -> 0
        }
    }

    // returns true only once the night transition is fully complete, not during the transition itself,
    // use this to know when the boss fight is in full night mode
    bool IsNight()
    {
        return s_is_night;
    }

    // returns true only when fully back to day - no transitions running and not night,
    // use this to know when the post-boss day restoration is fully complete
    bool IsDay()
    {
        return !s_is_night && !s_active && !s_reverse_active;
    }

    // returns how far into the night we are, 0.0 is full day and 1.0 is full night,
    // this is smooth so other systems can use it to blend their own visuals 
    float GetNightAlpha()
    {
        return s_night_alpha;
    }

    // frees all textures and meshes and resets all state back to defaults,
    // call this in the game state exit function to avoid memory leaks
    void Exit()
    {
        if (s_sun_tex) { AEGfxTextureUnload(s_sun_tex);    s_sun_tex = nullptr; }
        if (s_moon_tex) { AEGfxTextureUnload(s_moon_tex);   s_moon_tex = nullptr; }
        if (s_stars_tex) { AEGfxTextureUnload(s_stars_tex);  s_stars_tex = nullptr; }
        if (s_body_mesh) { AEGfxMeshFree(s_body_mesh);       s_body_mesh = nullptr; }
        if (s_bg_mesh) { AEGfxMeshFree(s_bg_mesh);         s_bg_mesh = nullptr; }
        if (s_stars_mesh) { AEGfxMeshFree(s_stars_mesh);      s_stars_mesh = nullptr; }

        s_active = false;
        s_is_night = false;
        s_reverse_active = false;
        s_timer = 0.0f;
        s_night_alpha = 0.0f;
    }

    // draws the night overlay and scrolling star field on top of the background,
    // the overlay gets darker as s_night_alpha increases, stars only draw when s_night_alpha > 0,
    // call this after drawing the background but before drawing enemies or the player
    void DrawSky()
    {
        if (!s_bg_mesh) return;

        float cam_x, cam_y;
        AEGfxGetCamPosition(&cam_x, &cam_y);
        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();

        // Night overlay
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(s_night_alpha * 0.85f);
        AEGfxSetColorToMultiply(NIGHT_R, NIGHT_G, NIGHT_B, 1.0f);
        Mesh_Renderer(s_bg_mesh, cam_x, cam_y, win_w, win_h, 0.0f);

        // Stars
        if (s_stars_tex && s_stars_mesh && s_night_alpha > 0.0f)
        {
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(s_night_alpha);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(s_stars_tex, s_stars_offset, 0.0f);
            Mesh_Renderer(s_stars_mesh, cam_x, cam_y + 180.0f, 1.0f, 1.0f, 0.0f);
        }
    }

    // draws the sun and moon sprites, sun fades out as night comes in and moon fades in,
    // both drift slightly in Y as the transition plays to give a rising/setting feel,
    // aspect ratio is applied so they don't get stretched on non-square screens
    void DrawBodies()
    {
        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();
        float aspect = win_w / win_h;

        // Sun
        float sun_alpha = 1.0f - s_night_alpha;
        if (s_sun_tex && s_body_mesh && sun_alpha > 0.01f)
        {
            float sun_y = BODY_Y + SUN_DRIFT * s_night_alpha;
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(sun_alpha);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(s_sun_tex, 0.0f, 0.0f);
            Mesh_Renderer(s_body_mesh, BODY_X, sun_y, BODY_SIZE * aspect, BODY_SIZE, 0.0f);
        }

        // Moon
        if (s_moon_tex && s_body_mesh && s_night_alpha > 0.01f)
        {
            float moon_y = (BODY_Y + SUN_DRIFT) - SUN_DRIFT * s_night_alpha;
            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(s_night_alpha);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(s_moon_tex, 0.0f, 0.0f);
            Mesh_Renderer(s_body_mesh, BODY_X, moon_y, BODY_SIZE * aspect, BODY_SIZE, 0.0f);
        }
    }

    // convenience function that calls DrawSky then DrawBodies in the correct order,
    // call this once per frame after the background clear and before drawing game objects
    void Draw()
    {
        DrawSky();
        DrawBodies();
    }
}