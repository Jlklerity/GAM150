/*
@file           DeathTransition.cpp
@brief          Death cinematic - circular black mask closes in on the player,
                then plays an 8-frame death sprite animation before fading to black.

Phases:
  0.0 - 0.6s  (SHRINK)  Circle closes from full screen down to player size.
  0.6 - 1.0s  (HOLD)    Circle locked tight. Death animation plays here.
  1.0 - 2.5s  (FADE)    Full black fade. Game Over appears after.
*/
#include "pch.h"
#include "DeathTransition.h"


static const float PLAYER_X = -700.0f;
static const float PLAYER_Y = -200.0f;
static const float PLAYER_SIZE = 150.0f;

static const float PHASE_SHRINK_END = 0.6f / 2.5f;
static const float PHASE_HOLD_END = 1.0f / 2.5f;

static const float DT_PI = 3.14159265f;
static const int   SLICES = 32;

// Death sprite sheet: 8 frames, 1 row
static const int   DEATH_ANIM_COLS = 8;
static const int   DEATH_ANIM_ROWS = 1;
static const float DEATH_FRAME_UV_W = 1.0f / DEATH_ANIM_COLS;
static const float DEATH_FRAME_UV_H = 1.0f / DEATH_ANIM_ROWS;
static const float DEATH_ANIM_FRAME_DURATION = 0.5f; // seconds per frame — tune as needed

namespace
{
    bool  s_active = false;
    bool  s_complete = false;
    float s_timer = 0.0f;

    // Death sprite animation state
    AEGfxTexture* s_deathTexture = nullptr;
    AEGfxVertexList* s_deathMesh = nullptr;
    int              s_deathFrame = 0;
    float            s_deathAnimTimer = 0.0f;
    bool             s_animFinished = false;  // stays on last frame once complete

    AEGfxVertexList* s_blackMesh = nullptr;

    float EaseInOutQuad(float x)
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        return x < 0.5f
            ? 2.0f * x * x
            : 1.0f - (-2.0f * x + 2.0f) * (-2.0f * x + 2.0f) * 0.5f;
    }

    float EaseInCubic(float x)
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        return x * x * x;
    }

    float Remap01(float x, float lo, float hi)
    {
        if (x <= lo) return 0.0f;
        if (x >= hi) return 1.0f;
        return (x - lo) / (hi - lo);
    }

    void DrawCircularMask(float cx, float cy, float radius, float winW, float winH)
    {
        if (!s_blackMesh) return;

        float outerRadius = sqrtf(winW * winW + winH * winH) * 1.5f;
        float angleStep = (2.0f * DT_PI) / (float)SLICES;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

        for (int i = 0; i < SLICES; ++i)
        {
            float a1 = i * angleStep;
            float a2 = (i + 1) * angleStep;

            float ix1 = cosf(a1) * radius, iy1 = sinf(a1) * radius;
            float ix2 = cosf(a2) * radius, iy2 = sinf(a2) * radius;
            float ox1 = cosf(a1) * outerRadius, oy1 = sinf(a1) * outerRadius;
            float ox2 = cosf(a2) * outerRadius, oy2 = sinf(a2) * outerRadius;

            AEGfxMeshStart();
            AEGfxTriAdd(ix1, iy1, 0xFF000000, 0.0f, 0.0f,
                ix2, iy2, 0xFF000000, 0.0f, 0.0f,
                ox1, oy1, 0xFF000000, 0.0f, 0.0f);
            AEGfxTriAdd(ix2, iy2, 0xFF000000, 0.0f, 0.0f,
                ox2, oy2, 0xFF000000, 0.0f, 0.0f,
                ox1, oy1, 0xFF000000, 0.0f, 0.0f);
            AEGfxVertexList* sliceMesh = AEGfxMeshEnd();
            if (sliceMesh)
            {
                AEMtx33 trans, transform;
                AEMtx33Trans(&trans, cx, cy);
                AEMtx33Identity(&transform);
                AEMtx33Concat(&transform, &trans, &transform);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(sliceMesh, AE_GFX_MDM_TRIANGLES);
                AEGfxMeshFree(sliceMesh);
            }
        }
    }
}

namespace DeathTransition
{
    void Start()
    {
        s_active = true;
        s_complete = false;
        s_timer = 0.0f;
        s_deathFrame = 0;
        s_deathAnimTimer = 0.0f;
        s_animFinished = false;

        if (!s_blackMesh)
            s_blackMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

        // Load the death animation sprite sheet
        if (!s_deathTexture)
            s_deathTexture = AEGfxTextureLoad("Assets/death1.png");

        // Build a UV-mapped mesh for one death frame
        if (!s_deathMesh)
            s_deathMesh = Sprite_Mesh(DEATH_FRAME_UV_W, DEATH_FRAME_UV_H);
    }

    void Update(float dt)
    {
        if (!s_active || s_complete) return;
        s_timer += dt;

        // Advance death animation only during SHRINK + HOLD phases
        float t = s_timer / DEATH_DURATION;
        if (t <= PHASE_HOLD_END && !s_animFinished)
        {
            s_deathAnimTimer += dt;
            if (s_deathAnimTimer >= DEATH_ANIM_FRAME_DURATION)
            {
                s_deathAnimTimer = 0.0f;
                s_deathFrame++;
                if (s_deathFrame >= DEATH_ANIM_COLS)
                {
                    s_deathFrame = DEATH_ANIM_COLS - 1; // hold last frame
                    s_animFinished = true;
                }
            }
        }

        if (s_timer / DEATH_DURATION >= 1.0f)
        {
            s_complete = true;
            s_active = false;
        }
    }

    void Draw(AEGfxVertexList* /*playerMesh*/, AEGfxTexture* /*playerTex*/,
        float /*uvOffsetX*/, float /*uvOffsetY*/)
    {
        if (!s_blackMesh) return;
        if (!s_active && !s_complete) return;

        float t = s_timer / DEATH_DURATION;
        if (t > 1.0f) t = 1.0f;

        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();

        AEGfxSetCamPosition(0.0f, 0.0f);

        // --- Circular mask ---
        float shrinkProg = EaseInOutQuad(Remap01(t, 0.0f, PHASE_SHRINK_END));
        float maxRadius = sqrtf(winW * winW + winH * winH) * 0.6f;
        float minRadius = PLAYER_SIZE * 1.3f;
        float radius = minRadius;

        if (t <= PHASE_SHRINK_END)
            radius = maxRadius + (minRadius - maxRadius) * EaseInCubic(shrinkProg);

        DrawCircularMask(PLAYER_X, PLAYER_Y, radius, winW, winH);

        // --- Death sprite animation (drawn inside the circle cutout) ---
        if (s_deathTexture && s_deathMesh && t <= PHASE_HOLD_END)
        {
            float uvX = s_deathFrame * DEATH_FRAME_UV_W;
            float uvY = 0.0f; // single row

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(s_deathTexture, uvX, uvY);
            Mesh_Renderer(s_deathMesh, PLAYER_X, PLAYER_Y, PLAYER_SIZE, PLAYER_SIZE, 0.0f);
        }

        // --- Fade to black during fade phase ---
        float fadeT = EaseInOutQuad(Remap01(t, PHASE_HOLD_END, 1.0f));
        if (fadeT > 0.0f)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(fadeT);
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
            Mesh_Renderer(s_blackMesh, 0.0f, 0.0f, winW, winH, 0.0f);
        }
    }

    bool IsActive() { return s_active; }
    bool IsComplete() { return s_complete; }

    void Reset()
    {
        s_active = false;
        s_complete = false;
        s_timer = 0.0f;
        s_deathFrame = 0;
        s_deathAnimTimer = 0.0f;
        s_animFinished = false;

        AEGfxSetCamPosition(0.0f, 0.0f);

        if (s_blackMesh) { AEGfxMeshFree(s_blackMesh);          s_blackMesh = nullptr; }
        if (s_deathMesh) { AEGfxMeshFree(s_deathMesh);          s_deathMesh = nullptr; }
        if (s_deathTexture) { AEGfxTextureUnload(s_deathTexture);   s_deathTexture = nullptr; }
    }
}