/*!
@file       LaserBeam.cpp
@brief      Player laser beam rendered using a sprite sheet, with a
            sprite-sheet explosion that plays at the enemy position
            after the beam arrives.

Laser spritesheet  (Assets/laser.png)
  - LASER_FRAME_COUNT frames in a single horizontal row.
  - Rendered as a stretched quad along the travel path.
  - One animation cycle plays during the travel phase.

Explosion spritesheet  (Assets/explosion.png)
  - EXPLOSION_FRAME_COUNT frames in a single horizontal row.
  - Plays once at the enemy position after the beam arrives.
  - Scales up slightly as it plays for a punchy feel.
*/

#include "pch.h"
#include "LaserBeam.h"


// ----------------------------------------------------------------
// Tuning constants
// ----------------------------------------------------------------
namespace
{
    // ---- Pool sizes ----
    static const int MAX_BEAMS = 12;
    static const int MAX_EXPLOSIONS = 12;

    // ---- Timing ----
    static const float TRAVEL_TIME = 0.18f;   // seconds for beam to reach enemy
    static const float FADE_TIME = 0.06f;   // seconds beam lingers before vanishing

    // ---- Laser sprite sheet ----
    // laser.png: single row of LASER_FRAME_COUNT frames
    static const int   LASER_FRAME_COUNT = 4;
    static const float LASER_FRAME_TIME = TRAVEL_TIME / (float)LASER_FRAME_COUNT;
    static const float LASER_UV_W = 1.0f / (float)LASER_FRAME_COUNT;
    static const float LASER_UV_H = 1.0f;    // one row
    static const float LASER_HEIGHT = 100.0f;  // world-space height of beam quad

    // ---- Explosion sprite sheet ----
    // explosion.png: single row of EXPLOSION_FRAME_COUNT frames
    static const int   EXPLOSION_FRAME_COUNT = 4;
    static const float EXPLOSION_TOTAL_TIME = 0.20f;
    static const float EXPLOSION_FRAME_TIME = EXPLOSION_TOTAL_TIME / (float)EXPLOSION_FRAME_COUNT;
    static const float EXPLOSION_UV_W = 1.0f / (float)EXPLOSION_FRAME_COUNT;
    static const float EXPLOSION_UV_H = 1.0f;    // one row
    static const float EXPLOSION_SIZE_START = 80.0f;   // world-space size at frame 0
    static const float EXPLOSION_SIZE_END = 140.0f;  // world-space size at last frame

    // ----------------------------------------------------------------
    // Structs
    // ----------------------------------------------------------------
    struct Beam
    {
        bool  active = false;
        float from_x = 0.0f;
        float from_y = 0.0f;
        float to_x = 0.0f;
        float to_y = 0.0f;
        float timer = 0.0f;   // total elapsed time for this beam
        int   frame = 0;      // current laser animation frame
        float frame_tmr = 0.0f;   // time accumulator for laser frame
    };

    struct Explosion
    {
        bool  active = false;
        float x = 0.0f;
        float y = 0.0f;
        float timer = 0.0f;
        int   frame = 0;
        float frame_tmr = 0.0f;
    };

    // ----------------------------------------------------------------
    // Runtime state
    // ----------------------------------------------------------------
    static Beam       s_beams[MAX_BEAMS]{};
    static Explosion  s_explosions[MAX_EXPLOSIONS]{};

    static AEGfxVertexList* s_laser_mesh = nullptr;
    static AEGfxVertexList* s_explosion_mesh = nullptr;
    static AEGfxTexture* s_laser_tex = nullptr;
    static AEGfxTexture* s_explosion_tex = nullptr;

    static float s_origin_x = -625.0f;
    static float s_origin_y = -175.0f;

    // ----------------------------------------------------------------
    // Spawn an explosion at a world position
    // ----------------------------------------------------------------
    static void SpawnExplosion(float x, float y)
    {
        for (int i = 0; i < MAX_EXPLOSIONS; ++i)
        {
            if (!s_explosions[i].active)
            {
                s_explosions[i] = { true, x, y, 0.0f, 0, 0.0f };
                return;
            }
        }
    }

    // ----------------------------------------------------------------
    // Draw a textured quad centered at (cx,cy) with given size & angle.
    // uv_x/uv_y are the UV offsets into the sprite sheet.
    // ----------------------------------------------------------------
    static void DrawSprite(AEGfxVertexList* mesh, AEGfxTexture* tex,
        float cx, float cy,
        float w, float h, float angle,
        float uv_x, float uv_y,
        float alpha)
    {
        if (!mesh || !tex) return;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(tex, uv_x, uv_y);

        // Build transform: Scale -> Rotate -> Translate
        AEMtx33 mS, mR, mT, mOut;
        AEMtx33Scale(&mS, w, h);
        AEMtx33Rot(&mR, angle);
        AEMtx33Trans(&mT, cx, cy);
        AEMtx33Concat(&mOut, &mR, &mS);
        AEMtx33Concat(&mOut, &mT, &mOut);
        AEGfxSetTransform(mOut.m);
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }

} // anonymous namespace

// ================================================================
// Lifecycle
// ================================================================
void LaserBeam_Init()
{
    // Reset pools
    for (int i = 0; i < MAX_BEAMS; ++i)       s_beams[i].active = false;
    for (int i = 0; i < MAX_EXPLOSIONS; ++i)  s_explosions[i].active = false;

    // --- Laser mesh ---
    // A single-frame UV quad: maps [0,LASER_UV_W] x [0,1] so we shift
    // the UV offset each frame via AEGfxTextureSet offset.
    if (s_laser_mesh) { AEGfxMeshFree(s_laser_mesh); s_laser_mesh = nullptr; }
    s_laser_mesh = Sprite_Mesh(LASER_UV_W, LASER_UV_H);

    // --- Explosion mesh ---
    if (s_explosion_mesh) { AEGfxMeshFree(s_explosion_mesh); s_explosion_mesh = nullptr; }
    s_explosion_mesh = Sprite_Mesh(EXPLOSION_UV_W, EXPLOSION_UV_H);

    // --- Textures ---
    if (s_laser_tex) { AEGfxTextureUnload(s_laser_tex);     s_laser_tex = nullptr; }
    if (s_explosion_tex) { AEGfxTextureUnload(s_explosion_tex); s_explosion_tex = nullptr; }

    s_laser_tex = AEGfxTextureLoad("Assets/laser.png");
    s_explosion_tex = AEGfxTextureLoad("Assets/explosion.png");
}

void LaserBeam_Free()
{
    if (s_laser_mesh) { AEGfxMeshFree(s_laser_mesh);          s_laser_mesh = nullptr; }
    if (s_explosion_mesh) { AEGfxMeshFree(s_explosion_mesh);      s_explosion_mesh = nullptr; }
    if (s_laser_tex) { AEGfxTextureUnload(s_laser_tex);      s_laser_tex = nullptr; }
    if (s_explosion_tex) { AEGfxTextureUnload(s_explosion_tex);  s_explosion_tex = nullptr; }

    for (int i = 0; i < MAX_BEAMS; ++i)       s_beams[i].active = false;
    for (int i = 0; i < MAX_EXPLOSIONS; ++i)  s_explosions[i].active = false;
}

// ================================================================
// Configuration
// ================================================================
void LaserBeam_SetOrigin(float x, float y)
{
    s_origin_x = x;
    s_origin_y = y;
}

// ================================================================
// Event: fire a beam toward (to_x, to_y)
// ================================================================
void LaserBeam_Fire(float to_x, float to_y)
{
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!s_beams[i].active)
        {
            s_beams[i].active = true;
            s_beams[i].from_x = s_origin_x;
            s_beams[i].from_y = s_origin_y;
            s_beams[i].to_x = to_x;
            s_beams[i].to_y = to_y;
            s_beams[i].timer = 0.0f;
            s_beams[i].frame = 0;
            s_beams[i].frame_tmr = 0.0f;
            return;
        }
    }
}

// ================================================================
// Per-frame update
// ================================================================
void LaserBeam_Update(float dt)
{
    // -- Beams --
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!s_beams[i].active) continue;
        Beam& b = s_beams[i];

        b.timer += dt;
        b.frame_tmr += dt;

        // Advance laser animation frame
        if (b.frame_tmr >= LASER_FRAME_TIME)
        {
            b.frame_tmr -= LASER_FRAME_TIME;
            b.frame = (b.frame + 1) % LASER_FRAME_COUNT;
        }

        // When beam finishes travelling, spawn explosion then enter fade
        if (b.timer >= TRAVEL_TIME && b.timer - dt < TRAVEL_TIME)
        {
            SpawnExplosion(b.to_x, b.to_y);
        }

        // Deactivate after travel + fade
        if (b.timer >= TRAVEL_TIME + FADE_TIME)
            b.active = false;
    }

    // -- Explosions --
    for (int i = 0; i < MAX_EXPLOSIONS; ++i)
    {
        if (!s_explosions[i].active) continue;
        Explosion& e = s_explosions[i];

        e.timer += dt;
        e.frame_tmr += dt;

        // Advance explosion frame
        if (e.frame_tmr >= EXPLOSION_FRAME_TIME)
        {
            e.frame_tmr -= EXPLOSION_FRAME_TIME;
            e.frame++;
        }

        // Deactivate when animation finishes
        if (e.frame >= EXPLOSION_FRAME_COUNT)
            e.active = false;
    }
}

// ================================================================
// Per-frame draw
// ================================================================
void LaserBeam_Draw()
{
    // -- Draw beams --
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!s_beams[i].active) continue;
        const Beam& b = s_beams[i];

        float dx = b.to_x - b.from_x;
        float dy = b.to_y - b.from_y;
        float full_len = sqrtf(dx * dx + dy * dy);
        if (full_len < 1.0f) continue;

        float angle = atan2f(dy, dx);

        // Tip position during travel phase
        float tip_x, tip_y, alpha;
        if (b.timer < TRAVEL_TIME)
        {
            float t = b.timer / TRAVEL_TIME;
            tip_x = b.from_x + dx * t;
            tip_y = b.from_y + dy * t;
            alpha = 1.0f;
        }
        else
        {
            tip_x = b.to_x;
            tip_y = b.to_y;
            alpha = 1.0f - (b.timer - TRAVEL_TIME) / FADE_TIME;
        }

        // Current drawn length
        float seg_dx = tip_x - b.from_x;
        float seg_dy = tip_y - b.from_y;
        float draw_len = sqrtf(seg_dx * seg_dx + seg_dy * seg_dy);
        if (draw_len < 1.0f) continue;

        float cx = (b.from_x + tip_x) * 0.5f;
        float cy = (b.from_y + tip_y) * 0.5f;

        // UV offset for current animation frame
        float uv_x = b.frame * LASER_UV_W;

        DrawSprite(s_laser_mesh, s_laser_tex,
            cx, cy,
            draw_len, LASER_HEIGHT,
            angle,
            uv_x, 0.0f,
            alpha);
    }

    // -- Draw explosions --
    for (int i = 0; i < MAX_EXPLOSIONS; ++i)
    {
        if (!s_explosions[i].active) continue;
        const Explosion& e = s_explosions[i];

        // Interpolate size from start to end as animation progresses
        float progress = (float)e.frame / (float)(EXPLOSION_FRAME_COUNT - 1);
        float size = EXPLOSION_SIZE_START + (EXPLOSION_SIZE_END - EXPLOSION_SIZE_START) * progress;

        float uv_x = e.frame * EXPLOSION_UV_W;

        DrawSprite(s_explosion_mesh, s_explosion_tex,
            e.x, e.y,
            size, size,
            0.0f,
            uv_x, 0.0f,
            1.0f);
    }

    // Reset render state
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}