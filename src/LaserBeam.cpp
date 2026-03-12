/*!
@file       LaserBeam.cpp
@brief      Player laser beam — animated travel, runtime colour & shape.

Each beam has two phases:
  1. TRAVEL (0 -> TRAVEL_TIME): tip advances from player origin to enemy.
  2. FADE   (TRAVEL_TIME -> TRAVEL_TIME + FADE_TIME): full beam fades out.

Five shape modes stamp different geometry along the drawn path:
  SOLID   — one stretched quad
  DASHED  — segments with even gaps
  ZIGZAG  — alternating angled quads (triangle-wave look)
  DOTS    — small squares at regular intervals
  TEXTURE — user-supplied PNG stretched across the beam quad
*/

#include "LaserBeam.h"
#include "AEEngine.h"
#include "MeshTemplates.h"
#include <cmath>

// ----------------------------------------------------------------
// Tuning
// ----------------------------------------------------------------
namespace
{
    static const int   MAX_BEAMS = 12;
    static const float TRAVEL_TIME = 0.12f;
    static const float FADE_TIME = 0.18f;
    static const float BEAM_THICKNESS = 8.0f;
    static const float CORE_THICKNESS = 3.0f;

    // Dashed
    static const float DASH_SEG = 24.0f;
    static const float DASH_GAP = 12.0f;

    // Zigzag
    static const float ZIG_SEG = 20.0f;
    static const float ZIG_ANGLE = 0.65f;   // ~37 degrees off beam axis

    // Dots
    static const float DOT_STEP = 18.0f;
    static const float DOT_SIZE = 9.0f;

    // ----------------------------------------------------------------
    // Runtime state
    // ----------------------------------------------------------------
    static float s_outerR = 0.2f, s_outerG = 1.0f, s_outerB = 1.0f;
    static float s_coreR = 1.0f, s_coreG = 1.0f, s_coreB = 0.8f;
    static int   s_shape = LASER_SHAPE_SOLID;

    struct Beam
    {
        bool  active = false;
        float fromX = 0.0f, fromY = 0.0f;
        float toX = 0.0f, toY = 0.0f;
        float timer = 0.0f;
    };

    static Beam             s_beams[MAX_BEAMS]{};
    static AEGfxVertexList* s_quadMesh = nullptr;  // unit quad — colour mode
    static AEGfxVertexList* s_texMesh = nullptr;  // unit quad — texture mode
    static AEGfxTexture* s_customTex = nullptr;  // Assets/laser_custom.png

    static float s_originX = -625.0f;
    static float s_originY = -175.0f;

    // ----------------------------------------------------------------
    // Internal helpers
    // ----------------------------------------------------------------
    static void DrawQuad(float cx, float cy, float w, float h, float angle,
        AEGfxVertexList* mesh)
    {
        if (!mesh) return;
        AEMtx33 mS, mR, mT, mOut;
        AEMtx33Scale(&mS, w, h);
        AEMtx33Rot(&mR, angle);
        AEMtx33Trans(&mT, cx, cy);
        AEMtx33Concat(&mOut, &mR, &mS);
        AEMtx33Concat(&mOut, &mT, &mOut);
        AEGfxSetTransform(mOut.m);
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
    }

    // SOLID
    static void DrawSolid(float fromX, float fromY, float tipX, float tipY,
        float angle, float alpha)
    {
        float dx = tipX - fromX, dy = tipY - fromY;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1.0f) return;
        float cx = (fromX + tipX) * 0.5f;
        float cy = (fromY + tipY) * 0.5f;

        AEGfxSetColorToMultiply(s_outerR, s_outerG, s_outerB, alpha);
        DrawQuad(cx, cy, len, BEAM_THICKNESS, angle, s_quadMesh);
        AEGfxSetColorToMultiply(s_coreR, s_coreG, s_coreB, alpha);
        DrawQuad(cx, cy, len, CORE_THICKNESS, angle, s_quadMesh);
    }

    // DASHED
    static void DrawDashed(float fromX, float fromY, float tipX, float tipY,
        float angle, float alpha)
    {
        float dx = tipX - fromX, dy = tipY - fromY;
        float totalLen = sqrtf(dx * dx + dy * dy);
        if (totalLen < 1.0f) return;
        float nx = dx / totalLen, ny = dy / totalLen;
        const float step = DASH_SEG + DASH_GAP;

        for (float t = 0.0f; t + DASH_SEG <= totalLen; t += step)
        {
            float midT = t + DASH_SEG * 0.5f;
            float cx = fromX + nx * midT;
            float cy = fromY + ny * midT;
            AEGfxSetColorToMultiply(s_outerR, s_outerG, s_outerB, alpha);
            DrawQuad(cx, cy, DASH_SEG, BEAM_THICKNESS, angle, s_quadMesh);
            AEGfxSetColorToMultiply(s_coreR, s_coreG, s_coreB, alpha);
            DrawQuad(cx, cy, DASH_SEG, CORE_THICKNESS, angle, s_quadMesh);
        }
    }

    // ZIGZAG — alternating angled segments give a triangle-wave look
    static void DrawZigzag(float fromX, float fromY, float tipX, float tipY,
        float angle, float alpha)
    {
        float dx = tipX - fromX, dy = tipY - fromY;
        float totalLen = sqrtf(dx * dx + dy * dy);
        if (totalLen < 1.0f) return;
        float nx = dx / totalLen, ny = dy / totalLen;

        int seg = 0;
        for (float t = 0.0f; t + ZIG_SEG <= totalLen; t += ZIG_SEG, ++seg)
        {
            float midT = t + ZIG_SEG * 0.5f;
            float cx = fromX + nx * midT;
            float cy = fromY + ny * midT;
            float a = angle + ((seg % 2 == 0) ? ZIG_ANGLE : -ZIG_ANGLE);
            AEGfxSetColorToMultiply(s_outerR, s_outerG, s_outerB, alpha);
            DrawQuad(cx, cy, ZIG_SEG * 1.3f, BEAM_THICKNESS, a, s_quadMesh);
            AEGfxSetColorToMultiply(s_coreR, s_coreG, s_coreB, alpha);
            DrawQuad(cx, cy, ZIG_SEG * 1.3f, CORE_THICKNESS, a, s_quadMesh);
        }
    }

    // DOTS — small squares stamped at regular intervals
    static void DrawDots(float fromX, float fromY, float tipX, float tipY,
        float /*angle*/, float alpha)
    {
        float dx = tipX - fromX, dy = tipY - fromY;
        float totalLen = sqrtf(dx * dx + dy * dy);
        if (totalLen < 1.0f) return;
        float nx = dx / totalLen, ny = dy / totalLen;

        for (float t = 0.0f; t <= totalLen; t += DOT_STEP)
        {
            float cx = fromX + nx * t;
            float cy = fromY + ny * t;
            AEGfxSetColorToMultiply(s_outerR, s_outerG, s_outerB, alpha);
            DrawQuad(cx, cy, DOT_SIZE, DOT_SIZE, 0.0f, s_quadMesh);
            AEGfxSetColorToMultiply(s_coreR, s_coreG, s_coreB, alpha);
            DrawQuad(cx, cy, DOT_SIZE * 0.45f, DOT_SIZE * 0.45f, 0.0f, s_quadMesh);
        }
    }

    // TEXTURE — stretch the user's PNG across the full beam
    static void DrawTexture(float fromX, float fromY, float tipX, float tipY,
        float angle, float alpha)
    {
        if (!s_customTex || !s_texMesh)
        {
            DrawSolid(fromX, fromY, tipX, tipY, angle, alpha);
            return;
        }
        float dx = tipX - fromX, dy = tipY - fromY;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1.0f) return;
        float cx = (fromX + tipX) * 0.5f;
        float cy = (fromY + tipY) * 0.5f;

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        // Multiply tint lets the player's chosen colour still tint the image
        AEGfxSetColorToMultiply(s_outerR, s_outerG, s_outerB, alpha);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        AEGfxTextureSet(s_customTex, 0.0f, 0.0f);
        DrawQuad(cx, cy, len, BEAM_THICKNESS * 2.5f, angle, s_texMesh);

        // Back to colour mode for next beam in the pool
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    }

} // anonymous namespace

// ================================================================
// Lifecycle
// ================================================================
void LaserBeam_Init()
{
    for (int i = 0; i < MAX_BEAMS; ++i) s_beams[i].active = false;

    // Colour-mode unit quad
    if (s_quadMesh) { AEGfxMeshFree(s_quadMesh); s_quadMesh = nullptr; }
    AEGfxMeshStart();
    AEGfxTriAdd(-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, 1.0f,
        0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    AEGfxTriAdd(0.5f, -0.5f, 0xFFFFFFFF, 1.0f, 1.0f,
        0.5f, 0.5f, 0xFFFFFFFF, 1.0f, 0.0f,
        -0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);
    s_quadMesh = AEGfxMeshEnd();

    // Texture-mode unit quad
    if (s_texMesh) { AEGfxMeshFree(s_texMesh); s_texMesh = nullptr; }
    s_texMesh = Sprite_Mesh(1.0f, 1.0f);

    // Try to load custom texture — silently skips if file absent
    if (s_customTex) { AEGfxTextureUnload(s_customTex); s_customTex = nullptr; }
    s_customTex = AEGfxTextureLoad("Assets/laser_custom.png");
}

void LaserBeam_Free()
{
    if (s_quadMesh) { AEGfxMeshFree(s_quadMesh);       s_quadMesh = nullptr; }
    if (s_texMesh) { AEGfxMeshFree(s_texMesh);        s_texMesh = nullptr; }
    if (s_customTex) { AEGfxTextureUnload(s_customTex); s_customTex = nullptr; }
    for (int i = 0; i < MAX_BEAMS; ++i) s_beams[i].active = false;
}

// ================================================================
// Configuration
// ================================================================
void LaserBeam_SetOrigin(float x, float y) { s_originX = x; s_originY = y; }
void LaserBeam_SetOuterColor(float r, float g, float b) { s_outerR = r; s_outerG = g; s_outerB = b; }
void LaserBeam_SetCoreColor(float r, float g, float b) { s_coreR = r;  s_coreG = g;  s_coreB = b; }
void LaserBeam_GetOuterColor(float& r, float& g, float& b) { r = s_outerR; g = s_outerG; b = s_outerB; }
void LaserBeam_GetCoreColor(float& r, float& g, float& b) { r = s_coreR;  g = s_coreG;  b = s_coreB; }
void LaserBeam_SetShape(int shape) { if (shape >= 0 && shape < LASER_SHAPE_COUNT) s_shape = shape; }
int  LaserBeam_GetShape() { return s_shape; }

// ================================================================
// Event
// ================================================================
void LaserBeam_Fire(float toX, float toY)
{
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!s_beams[i].active)
        {
            s_beams[i] = { true, s_originX, s_originY, toX, toY, 0.0f };
            return;
        }
    }
}

// ================================================================
// Per-frame
// ================================================================
void LaserBeam_Update(float dt)
{
    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!s_beams[i].active) continue;
        s_beams[i].timer += dt;
        if (s_beams[i].timer >= TRAVEL_TIME + FADE_TIME)
            s_beams[i].active = false;
    }
}

void LaserBeam_Draw()
{
    if (!s_quadMesh) return;

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < MAX_BEAMS; ++i)
    {
        if (!s_beams[i].active) continue;
        const Beam& b = s_beams[i];

        float dx = b.toX - b.fromX, dy = b.toY - b.fromY;
        float fullLen = sqrtf(dx * dx + dy * dy);
        if (fullLen < 1.0f) continue;

        float angle = atan2f(dy, dx);
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
            tipX = b.toX; tipY = b.toY;
            alpha = 1.0f - (b.timer - TRAVEL_TIME) / FADE_TIME;
        }

        switch (s_shape)
        {
        case LASER_SHAPE_DASHED:  DrawDashed(b.fromX, b.fromY, tipX, tipY, angle, alpha); break;
        case LASER_SHAPE_ZIGZAG:  DrawZigzag(b.fromX, b.fromY, tipX, tipY, angle, alpha); break;
        case LASER_SHAPE_DOTS:    DrawDots(b.fromX, b.fromY, tipX, tipY, angle, alpha); break;
        case LASER_SHAPE_TEXTURE: DrawTexture(b.fromX, b.fromY, tipX, tipY, angle, alpha); break;
        default:                  DrawSolid(b.fromX, b.fromY, tipX, tipY, angle, alpha); break;
        }
    }

    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
}