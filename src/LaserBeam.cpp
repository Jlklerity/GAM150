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
    static const float LASER_UV_H = 1.0f;   // one row
    static const float LASER_HEIGHT = 100.0f;  // world-space height of beam quad

    // ---- Explosion sprite sheet ----
    // explosion.png: single row of EXPLOSION_FRAME_COUNT frames
    static const int   EXPLOSION_FRAME_COUNT = 4;
    static const float EXPLOSION_TOTAL_TIME = 0.20f;
    static const float EXPLOSION_FRAME_TIME = EXPLOSION_TOTAL_TIME / (float)EXPLOSION_FRAME_COUNT;
    static const float EXPLOSION_UV_W = 1.0f / (float)EXPLOSION_FRAME_COUNT;
    static const float EXPLOSION_UV_H = 1.0f;   // one row
    static const float EXPLOSION_SIZE_START = 80.0f;  // world-space size at frame 0
    static const float EXPLOSION_SIZE_END = 140.0f; // world-space size at last frame

    // ----------------------------------------------------------------
    // Structs
    // ----------------------------------------------------------------
    struct Beam
    {
        bool  active = false;
        float fromX = 0.0f, fromY = 0.0f;
        float toX = 0.0f, toY = 0.0f;
        float timer = 0.0f;   // total elapsed time for this beam
        int   frame = 0;      // current laser animation frame
        float frameTmr = 0.0f;   // time accumulator for laser frame
    };

    struct Explosion
    {
        bool  active = false;
        float x = 0.0f, y = 0.0f;
        float timer = 0.0f;
        int   frame = 0;
        float frameTmr = 0.0f;
    };

    // ----------------------------------------------------------------
    // Runtime state
    // ----------------------------------------------------------------
    static Beam       s_beams[MAX_BEAMS]{};
    static Explosion  s_explosions[MAX_EXPLOSIONS]{};

    static AEGfxVertexList* s_laserMesh = nullptr;
    static AEGfxVertexList* s_explosionMesh = nullptr;
    static AEGfxTexture* s_laserTex = nullptr;
    static AEGfxTexture* s_explosionTex = nullptr;

    static float s_originX = -625.0f;
    static float s_originY = -175.0f;

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
    // uvX/uvY are the UV offsets into the sprite sheet.
    // ----------------------------------------------------------------
    static void DrawSprite(AEGfxVertexList* mesh, AEGfxTexture* tex,
        float cx, float cy,
        float w, float h, float angle,
        float uvX, float uvY,
        float alpha)
    {
        if (!mesh || !tex) return;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(tex, uvX, uvY);

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
    for (int i = 0; i < MAX_BEAMS; ++i) s_beams[i].active = false;
    for (int i = 0; i < MAX_EXPLOSIONS; ++i) s_explosions[i].active = false;

    // --- Laser mesh ---
    // A single-frame UV quad: maps [0,LASER_UV_W] x [0,1] so we shift
    // the UV offset each frame via AEGfxTextureSet offset.
    if (s_laserMesh) { AEGfxMeshFree(s_laserMesh); s_laserMesh = nullptr; }
    s_laserMesh = Sprite_Mesh(LASER_UV_W, LASER_UV_H);

    // --- Explosion mesh ---
    if (s_explosionMesh) { AEGfxMeshFree(s_explosionMesh); s_explosionMesh = nullptr; }
    s_explosionMesh = Sprite_Mesh(EXPLOSION_UV_W, EXPLOSION_UV_H);

    // --- Textures ---
    if (s_laserTex) { AEGfxTextureUnload(s_laserTex);     s_laserTex = nullptr; }
    if (s_explosionTex) { AEGfxTextureUnload(s_explosionTex); s_explosionTex = nullptr; }

    s_laserTex = AEGfxTextureLoad("Assets/laser.png");
    s_explosionTex = AEGfxTextureLoad("Assets/explosion.png");
}

void LaserBeam_Free()
{
    if (s_laserMesh) { AEGfxMeshFree(s_laserMesh);          s_laserMesh = nullptr; }
    if (s_explosionMesh) { AEGfxMeshFree(s_explosionMesh);      s_explosionMesh = nullptr; }
    if (s_laserTex) { AEGfxTextureUnload(s_laserTex);      s_laserTex = nullptr; }
    if (s_explosionTex) { AEGfxTextureUnload(s_explosionTex);  s_explosionTex = nullptr; }

    for (int i = 0; i < MAX_BEAMS; ++i) s_beams[i].active = false;
    for (int i = 0; i < MAX_EXPLOSIONS; ++i) s_explosions[i].active = false;
}

// ================================================================
// Configuration
// ================================================================
void LaserBeam_SetOrigin(float x, float y)
{
    s_originX = x;
    s_originY = y;
}

// ================================================================
// Event: fire a beam toward (toX, toY)
// ================================================================
void LaserBeam_Fire(float toX, float toY)
{
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!s_beams[i].active)
        {
            s_beams[i].active = true;
            s_beams[i].fromX = s_originX;
            s_beams[i].fromY = s_originY;
            s_beams[i].toX = toX;
            s_beams[i].toY = toY;
            s_beams[i].timer = 0.0f;
            s_beams[i].frame = 0;
            s_beams[i].frameTmr = 0.0f;
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
        b.frameTmr += dt;

        // Advance laser animation frame
        if (b.frameTmr >= LASER_FRAME_TIME)
        {
            b.frameTmr -= LASER_FRAME_TIME;
            b.frame = (b.frame + 1) % LASER_FRAME_COUNT;
        }

        // When beam finishes travelling, spawn explosion then enter fade
        if (b.timer >= TRAVEL_TIME && b.timer - dt < TRAVEL_TIME)
        {
            SpawnExplosion(b.toX, b.toY);
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
        e.frameTmr += dt;

        // Advance explosion frame
        if (e.frameTmr >= EXPLOSION_FRAME_TIME)
        {
            e.frameTmr -= EXPLOSION_FRAME_TIME;
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

        float dx = b.toX - b.fromX;
        float dy = b.toY - b.fromY;
        float fullLen = sqrtf(dx * dx + dy * dy);
        if (fullLen < 1.0f) continue;

        float angle = atan2f(dy, dx);

        // Tip position during travel phase
        float tipX, tipY, alpha;
        if (b.timer < TRAVEL_TIME)
        {
            float t = b.timer / TRAVEL_TIME;
            tipX = b.fromX + dx * t;
            tipY = b.fromY + dy * t;
            alpha = 1.0f;
        }
        else
        {
            tipX = b.toX;
            tipY = b.toY;
            alpha = 1.0f - (b.timer - TRAVEL_TIME) / FADE_TIME;
        }

        // Current drawn length
        float segDx = tipX - b.fromX;
        float segDy = tipY - b.fromY;
        float drawLen = sqrtf(segDx * segDx + segDy * segDy);
        if (drawLen < 1.0f) continue;

        float cx = (b.fromX + tipX) * 0.5f;
        float cy = (b.fromY + tipY) * 0.5f;

        // UV offset for current animation frame
        float uvX = b.frame * LASER_UV_W;

        DrawSprite(s_laserMesh, s_laserTex,
            cx, cy,
            drawLen, LASER_HEIGHT,
            angle,
            uvX, 0.0f,
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

        float uvX = e.frame * EXPLOSION_UV_W;

        DrawSprite(s_explosionMesh, s_explosionTex,
            e.x, e.y,
            size, size,
            0.0f,
            uvX, 0.0f,
            1.0f);
    }

    // Reset render state
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}