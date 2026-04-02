// File: DeathTransition.cpp
// Author: Pengfei
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
static const float DEATH_ANIM_FRAME_DURATION = 0.5f;

namespace
{
    bool  s_active = false;
    bool  s_complete = false;
    float s_timer = 0.0f;

    // Death sprite animation state
    AEGfxTexture* s_death_texture = nullptr;
    AEGfxVertexList* s_death_mesh = nullptr;
    int              s_death_frame = 0;
    float            s_death_anim_timer = 0.0f;
    bool             s_anim_finished = false;  // stays on last frame once complete

    AEGfxVertexList* s_black_mesh = nullptr;

    // smooths a value in [0,1] so it accelerates at the start and decelerates at the end,
    // used for the circle shrink and the fade to black so don't feel instant 
    float ease_in_out_quad(float x)
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        return x < 0.5f
            ? 2.0f * x * x
            : 1.0f - (-2.0f * x + 2.0f) * (-2.0f * x + 2.0f) * 0.5f;
    }

    // stronger ease in than quadratic, used specifically for the circle shrink so it
    // starts slow and end fast 
    float ease_in_cubic(float x)
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        return x * x * x;
    }

    // remaps x from the range [lo, hi] to [0, 1], clamped at both ends,
    // used to isolate each phase of the transition (shrink, hold, fade) from the global timer
    float remap01(float x, float lo, float hi)
    {
        if (x <= lo) return 0.0f;
        if (x >= hi) return 1.0f;
        return (x - lo) / (hi - lo);
    }

    // draws a black ring that covers everything outside a circle of the given radius centred at (cx, cy),
    // built from triangles arranged in a ring between the inner circle and a huge outer radius,
    // each slice is its own mesh built and freed on the spot 
    // params are the centre position, the inner circle radius, and the window dimensions for the outer size
    void draw_circular_mask(float cx, float cy, float radius, float win_w, float win_h)
    {
        if (!s_black_mesh) return;

        float outer_radius = sqrtf(win_w * win_w + win_h * win_h) * 1.5f;
        float angle_step = (2.0f * DT_PI) / (float)SLICES;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

        for (int i = 0; i < SLICES; ++i)
        {
            float a1 = i * angle_step;
            float a2 = (i + 1) * angle_step;

            float ix1 = cosf(a1) * radius, iy1 = sinf(a1) * radius;
            float ix2 = cosf(a2) * radius, iy2 = sinf(a2) * radius;
            float ox1 = cosf(a1) * outer_radius, oy1 = sinf(a1) * outer_radius;
            float ox2 = cosf(a2) * outer_radius, oy2 = sinf(a2) * outer_radius;

            AEGfxMeshStart();
            AEGfxTriAdd(ix1, iy1, 0xFF000000, 0.0f, 0.0f,
                ix2, iy2, 0xFF000000, 0.0f, 0.0f,
                ox1, oy1, 0xFF000000, 0.0f, 0.0f);
            AEGfxTriAdd(ix2, iy2, 0xFF000000, 0.0f, 0.0f,
                ox2, oy2, 0xFF000000, 0.0f, 0.0f,
                ox1, oy1, 0xFF000000, 0.0f, 0.0f);
            AEGfxVertexList* slice_mesh = AEGfxMeshEnd();
            if (slice_mesh)
            {
                AEMtx33 trans, transform;
                AEMtx33Trans(&trans, cx, cy);
                AEMtx33Identity(&transform);
                AEMtx33Concat(&transform, &trans, &transform);
                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(slice_mesh, AE_GFX_MDM_TRIANGLES);
                AEGfxMeshFree(slice_mesh);
            }
        }
    }
}

namespace DeathTransition
{
    // satrts the death cinematic, loads the death sprite sheet and builds meshes if not already loaded,
    // resets all timers and animation state so it plays cleanly from the beginning every time
    void start()
    {
        s_active = true;
        s_complete = false;
        s_timer = 0.0f;
        s_death_frame = 0;
        s_death_anim_timer = 0.0f;
        s_anim_finished = false;

        if (!s_black_mesh)
            s_black_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

        // Load the death animation sprite sheet that matches the active skin
        if (!s_death_texture)
        {
            const char* death_path = "Assets/death1.png";
            if (g_shop_skin_id == 1) death_path = "Assets/player2death.png";
            else if (g_shop_skin_id == 2) death_path = "Assets/catdeath.png";
            s_death_texture = AEGfxTextureLoad(death_path);
        }

        // Build a UV-mapped mesh for one death frame
        if (!s_death_mesh)
            s_death_mesh = Sprite_Mesh(DEATH_FRAME_UV_W, DEATH_FRAME_UV_H);
    }

    // advances the global timer and the death sprite animation each frame, param is dt in seconds,
    // animation only plays during the shrink and hold phases, then holds on the last frame,
    // sets s_complete when the full duration is reached so the caller knows to show game over
    void update(float dt)
    {
        if (!s_active || s_complete) return;
        s_timer += dt;

        // Advance death animation only during SHRINK + HOLD phases
        float t = s_timer / DEATH_DURATION;
        if (t <= PHASE_HOLD_END && !s_anim_finished)
        {
            s_death_anim_timer += dt;
            if (s_death_anim_timer >= DEATH_ANIM_FRAME_DURATION)
            {
                s_death_anim_timer = 0.0f;
                s_death_frame++;
                if (s_death_frame >= DEATH_ANIM_COLS)
                {
                    s_death_frame = DEATH_ANIM_COLS - 1; // hold last frame
                    s_anim_finished = true;
                }
            }
        }

        if (s_timer / DEATH_DURATION >= 1.0f)
        {
            s_complete = true;
            s_active = false;
        }
    }

    // renders the full death anim in three layered phases:
    // first the black circular mask closes in on the player position,
    // then the death sprite animation plays inside the circle cutout,
    // finally a full black fade covers the screen before game over appears,
    // player mesh params are unused but kept for interface compatibility

    void draw(AEGfxVertexList* /*playerMesh*/, AEGfxTexture* /*playerTex*/,
        float /*uvOffsetX*/, float /*uvOffsetY*/)
    {
        if (!s_black_mesh) return;
        if (!s_active && !s_complete) return;

        float t = s_timer / DEATH_DURATION;
        if (t > 1.0f) t = 1.0f;

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();

        AEGfxSetCamPosition(0.0f, 0.0f);

        // --- Circular mask ---
        float shrink_prog = ease_in_out_quad(remap01(t, 0.0f, PHASE_SHRINK_END));
        float max_radius = sqrtf(win_w * win_w + win_h * win_h) * 0.6f;
        float min_radius = PLAYER_SIZE * 1.3f;
        float radius = min_radius;

        if (t <= PHASE_SHRINK_END)
            radius = max_radius + (min_radius - max_radius) * ease_in_cubic(shrink_prog);

        draw_circular_mask(PLAYER_X, PLAYER_Y, radius, win_w, win_h);

        // --- Death sprite animation ---
        if (s_death_texture && s_death_mesh && t <= PHASE_HOLD_END)
        {
            float uv_x = s_death_frame * DEATH_FRAME_UV_W;
            float uv_y = 0.0f; // single row

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            AEGfxTextureSet(s_death_texture, uv_x, uv_y);
            mesh_renderer(s_death_mesh, PLAYER_X, PLAYER_Y, PLAYER_SIZE, PLAYER_SIZE, 0.0f);
        }

        // --- Fade to black during fade phase ---
        float fade_t = ease_in_out_quad(remap01(t, PHASE_HOLD_END, 1.0f));
        if (fade_t > 0.0f)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(fade_t);
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
            mesh_renderer(s_black_mesh, 0.0f, 0.0f, win_w, win_h, 0.0f);
        }
    }

    // returns true while the cinematic is currently playing, false before start() or after it finishes
    bool is_active() { return s_active; }

    // returns true once the full duration has elapsed, use this to trigger the game over screen
    bool is_complete() { return s_complete; }

    // clears all state and frees the meshes and texture, call this on game state exit or restart,
    // also resets the camera position since draw() moves it to origin during the cinematic
    void reset()
    {
        s_active = false;
        s_complete = false;
        s_timer = 0.0f;
        s_death_frame = 0;
        s_death_anim_timer = 0.0f;
        s_anim_finished = false;

        AEGfxSetCamPosition(0.0f, 0.0f);

        if (s_black_mesh) { AEGfxMeshFree(s_black_mesh);          s_black_mesh = nullptr; }
        if (s_death_mesh) { AEGfxMeshFree(s_death_mesh);          s_death_mesh = nullptr; }
        if (s_death_texture) { AEGfxTextureUnload(s_death_texture);   s_death_texture = nullptr; }
    }
}