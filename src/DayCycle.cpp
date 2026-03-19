#include "pch.h"
#include "DayCycle.h"


// ---------------------------------------------------------------------------
// Tuning constants
// ---------------------------------------------------------------------------
static const float TRANSITION_DURATION = 5.0f;

// Sky colours (day -> night lerp)
static const float DAY_R   = 0.53f, DAY_G   = 0.81f, DAY_B   = 0.92f;
static const float NIGHT_R = 0.04f, NIGHT_G = 0.04f, NIGHT_B = 0.15f;

// Sun / moon world position (top-right corner)
static const float BODY_SIZE   = 90.0f;   // sprite render size (square)
static const float BODY_X      = 650.0f;  // world X
static const float BODY_Y      = 380.0f;  // world Y (top-right area)
static const float SUN_DRIFT   = 30.0f;   // how far the sun drifts upward as it fades

// Stars layer
static const float STARS_SPEED = 0.004f;  // UV scroll speed (slower than clouds)

namespace
{
    bool  s_active      = false;   // transition is running
    bool  s_isNight     = false;   // fully night
    float s_timer       = 0.0f;
    float s_nightAlpha  = 0.0f;   // 0 = day, 1 = night
    float s_starsOffset = 0.0f;

    AEGfxTexture*    s_sunTex    = nullptr;
    AEGfxTexture*    s_moonTex   = nullptr;
    AEGfxTexture*    s_starsTex  = nullptr;
    AEGfxVertexList* s_bodyMesh  = nullptr;  // shared square mesh for sun/moon
    AEGfxVertexList* s_bgMesh    = nullptr;  // fullscreen quad for background
    AEGfxVertexList* s_starsMesh = nullptr;  // fullscreen quad for stars layer

    // Smooth ease in-out
    float EaseInOut(float t)
    {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        return t < 0.5f ? 2.0f * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * 0.5f;
    }
}

namespace DayCycle
{
    void Init()
    {
        s_active      = false;
        s_isNight     = false;
        s_timer       = 0.0f;
        s_nightAlpha  = 0.0f;
        s_starsOffset = 0.0f;

        // Free previous if re-entering state
        if (s_sunTex)   { AEGfxTextureUnload(s_sunTex);   s_sunTex   = nullptr; }
        if (s_moonTex)  { AEGfxTextureUnload(s_moonTex);  s_moonTex  = nullptr; }
        if (s_starsTex) { AEGfxTextureUnload(s_starsTex); s_starsTex = nullptr; }
        if (s_bodyMesh) { AEGfxMeshFree(s_bodyMesh);      s_bodyMesh = nullptr; }
        if (s_bgMesh)   { AEGfxMeshFree(s_bgMesh);        s_bgMesh   = nullptr; }
        if (s_starsMesh){ AEGfxMeshFree(s_starsMesh);     s_starsMesh = nullptr; }

        s_sunTex    = AEGfxTextureLoad("Assets/sun.png");
        AE_ASSERT_MESG(s_sunTex,   "DayCycle: failed to load sun.png");

        s_moonTex   = AEGfxTextureLoad("Assets/moon.png");
        AE_ASSERT_MESG(s_moonTex,  "DayCycle: failed to load moon.png");

        s_starsTex  = AEGfxTextureLoad("Assets/stars.png");
        AE_ASSERT_MESG(s_starsTex, "DayCycle: failed to load stars.png");

        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();

        s_bodyMesh  = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
        s_bgMesh    = Rect_Mesh(winW, winH, 0xFFFFFFFF);
        s_starsMesh = Rect_Mesh(winW, winH, 0xFFFFFFFF);
    }

    void TriggerTransition()
    {
        if (s_active || s_isNight) return; // already running or done
        s_active = true;
        s_timer  = 0.0f;
    }

    void Update(float dt)
    {
        // Scroll stars regardless (invisible until alpha > 0)
        s_starsOffset += STARS_SPEED * dt;
        if (s_starsOffset > 1.0f) s_starsOffset -= 1.0f;

        if (!s_active) return;

        s_timer += dt;
        float t = s_timer / TRANSITION_DURATION;
        if (t >= 1.0f)
        {
            t            = 1.0f;
            s_active     = false;
            s_isNight    = true;
        }
        s_nightAlpha = EaseInOut(t);
    }

    void Draw()
{
    if (!s_bgMesh) return;

    float camX, camY;
    AEGfxGetCamPosition(&camX, &camY);
    //float winW = (float)AEGfxGetWindowWidth();
    //float winH = (float)AEGfxGetWindowHeight();

    // ------------------------------------------------------------------
    // 1. Background colour overlay — SKY ONLY
    // ------------------------------------------------------------------
   // const float SKY_HEIGHT = 670.0f; // tweak to match where ground 
    //float skyOffsetY = (winH * 0.5f) - (winH - SKY_HEIGHT) * 0.5f;

    // 1. Background colour overlay 
    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(s_nightAlpha);
    AEGfxSetColorToMultiply(NIGHT_R, NIGHT_G, NIGHT_B, 1.0f);
    Mesh_Renderer(s_bgMesh, camX, camY, 1.0f, 1.0f, 0.0f);


    // ------------------------------------------------------------------
    // 2. Stars layer 
    // ------------------------------------------------------------------
    if (s_starsTex && s_starsMesh && s_nightAlpha > 0.0f)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(s_nightAlpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_starsTex, s_starsOffset, 0.0f);
        Mesh_Renderer(s_starsMesh, camX, camY + 180.0f, 1.0f, 1.0f, 0.0f);



    }

    // ------------------------------------------------------------------
    // 3. Sun —
    // ------------------------------------------------------------------
    float sunAlpha = 1.0f - s_nightAlpha;
    if (s_sunTex && s_bodyMesh && sunAlpha > 0.01f)
    {
        float sunY = BODY_Y + SUN_DRIFT * s_nightAlpha;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(sunAlpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_sunTex, 0.0f, 0.0f);
        Mesh_Renderer(s_bodyMesh, BODY_X, sunY, BODY_SIZE, BODY_SIZE, 0.0f);
    }

    // ------------------------------------------------------------------
    // 4. Moon 
    // ------------------------------------------------------------------
    if (s_moonTex && s_bodyMesh && s_nightAlpha > 0.01f)
    {
        float moonY = (BODY_Y + SUN_DRIFT) - SUN_DRIFT * s_nightAlpha;
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(s_nightAlpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_moonTex, 0.0f, 0.0f);
        Mesh_Renderer(s_bodyMesh, BODY_X, moonY, BODY_SIZE, BODY_SIZE, 0.0f);
    }
}

    bool IsNight()
    {
        return s_isNight;
    }

    float GetNightAlpha()
    {
        return s_nightAlpha;
    }

    void Exit()
    {
        if (s_sunTex)    { AEGfxTextureUnload(s_sunTex);    s_sunTex    = nullptr; }
        if (s_moonTex)   { AEGfxTextureUnload(s_moonTex);   s_moonTex   = nullptr; }
        if (s_starsTex)  { AEGfxTextureUnload(s_starsTex);  s_starsTex  = nullptr; }
        if (s_bodyMesh)  { AEGfxMeshFree(s_bodyMesh);       s_bodyMesh  = nullptr; }
        if (s_bgMesh)    { AEGfxMeshFree(s_bgMesh);         s_bgMesh    = nullptr; }
        if (s_starsMesh) { AEGfxMeshFree(s_starsMesh);      s_starsMesh = nullptr; }

        s_active     = false;
        s_isNight    = false;
        s_timer      = 0.0f;
        s_nightAlpha = 0.0f;
    }
}
