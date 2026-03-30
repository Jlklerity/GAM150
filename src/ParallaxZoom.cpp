#include "pch.h"
#include "ParallaxZoom.h"


namespace
{
    // How far each layer zooms - keep low so foreground doesn't swamp the screen
    // foreground (depth 0.9) -> 1 + 0.9*0.8 = 1.72x max
    // sky        (depth 0.1) -> 1 + 0.1*0.8 = 1.08x max
    const float ZOOM_STRENGTH = 0.8f;

    const float FADE_START_T = 0.75f;  // black fade begins at 55%
    const float STATE_SWITCH_T = 0.98f;  // state switches at 98%

    bool  s_active = false;
    float s_timer = 0.0f;
    int   s_target_state = 0;
    bool  s_switched = false;

    AEGfxVertexList* s_fade_mesh = nullptr;
    AEGfxVertexList* s_layer_mesh = nullptr;

    // cubic ease-in curve - slow start that accelerates, used to make the zoom feel like a rush forward
    // rather than a constant speed push, param t is in [0,1], returns smoothed value in [0,1]
    float EaseInCubic(float x)
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        return x * x * x;
    }

    // remaps x from [lo, hi] to [0, 1], clamped at both ends,
    // used to isolate the fade phase from the full zoom timer
    float Remap01(float x, float lo, float hi)
    {
        if (x <= lo) return 0.0f;
        if (x >= hi) return 1.0f;
        return (x - lo) / (hi - lo);
    }
}

namespace ParallaxZoom
{
    // kicks off the parallax zoom transition towards the given game state,
    // param is the target game state to switch to (e.g. GS_GAME, GS_MAIN_MENU),
    // frees and rebuilds meshes so it's safe to call multiple times, resets all timers
    void Start(int targetState)
    {
        if (s_fade_mesh)
        {
            AEGfxMeshFree(s_fade_mesh);
            s_fade_mesh = nullptr;
        }
        if (s_layer_mesh)
        {
            AEGfxMeshFree(s_layer_mesh);
            s_layer_mesh = nullptr;
        }
        s_active = true;
        s_timer = 0.0f;
        s_target_state = targetState;
        s_switched = false;

        //float win_w = (float)AEGfxGetWindowWidth();
        //float win_h = (float)AEGfxGetWindowHeight();

        if (!s_fade_mesh)  s_fade_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
        if (!s_layer_mesh) s_layer_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF); // 1x1, scaled in Draw
    }

    // advances the zoom timer each frame, param is dt in seconds,
    // triggers the game state switch at STATE_SWITCH_T (98% through) so the screen is nearly black before switching,
    // deactivates itself once the full duration is reached
    void Update(float dt)
    {
        if (!s_active) return;

        s_timer += dt;
        float t = s_timer / ZOOM_DURATION;
        if (t > 1.0f) t = 1.0f;

        if (!s_switched && t >= STATE_SWITCH_T)
        {
            gGameStateNext = s_target_state;
            s_switched = true;
        }

        if (t >= 1.0f)
            s_active = false;
    }

    // renders the parallax zoom effect each frame, param is the vertical camera offset for parallax depth,
    // draws a sky colour fill first, then each parallax layer scaled by depth and zoom progress,
    // deeper layers (higher depth value) zoom more aggressively to sell the rush-forward feeling,
    // fades to black starting at FADE_START_T and switches to BM_NONE at 50% fade to avoid blending artefacts
    void Draw(float v_cam_y)
    {
        if (!s_active || !s_layer_mesh) return;

        float t = s_timer / ZOOM_DURATION;
        if (t > 1.0f) t = 1.0f;
        float eased_t = EaseInCubic(t);

        float cam_x, cam_y;
        AEGfxGetCamPosition(&cam_x, &cam_y);
        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();

        // Sky background
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.53f, 0.81f, 0.92f, 1.0f);
        Mesh_Renderer(s_fade_mesh, cam_x, cam_y, win_w, win_h, 0.0f);

        // Draw each layer
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        int layer_count = BackgroundParallax::MenuParallax_GetLayerCount();
        float pan_offset = BackgroundParallax::MenuParallax_GetManualPanOffset();

        for (int i = 0; i < layer_count; ++i)
        {
            AEGfxTexture* tex = BackgroundParallax::MenuParallax_GetLayerTexture(i);
            float depth = BackgroundParallax::MenuParallax_GetLayerDepth(i);
            float u_offset = BackgroundParallax::MenuParallax_GetLayerUOffset(i);

            if (!tex) continue;

            float zoom = 1.0f + (depth * ZOOM_STRENGTH * eased_t);
            float render_y = cam_y - (v_cam_y * depth);
            float render_offset = u_offset - (pan_offset * depth);

            AEGfxTextureSet(tex, render_offset, 0.0f);
            Mesh_Renderer(s_layer_mesh, cam_x, render_y, win_w * zoom, win_h * zoom, 0.0f);
        }

        float fade_alpha = Remap01(t, FADE_START_T, 1.0f);
        if (s_fade_mesh && fade_alpha > 0.0f)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

            if (fade_alpha >= 0.5f)  // switch to BM_NONE at 50% fade, not 100%
            {
                AEGfxSetBlendMode(AE_GFX_BM_NONE);
                AEGfxSetTransparency(1.0f);
            }
            else
            {
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(fade_alpha * 2.0f); // remap 0-0.5 to 0-1
            }

            Mesh_Renderer(s_fade_mesh, cam_x, cam_y, win_w * 3.0f, win_h * 3.0f, 0.0f);
        }
    }

    // returns true while the zoom transition is running, use this to skip normal gameplay rendering
    bool IsActive()
    {
        return s_active;
    }

    // immediately cancels the transition and frees the meshes, call this if the game state
    // needs to abort mid-transition or on cleanup to avoid memory leaks
    void Reset()
    {
        s_active = false;
        s_timer = 0.0f;
        s_switched = false;

        if (s_fade_mesh)
        {
            AEGfxMeshFree(s_fade_mesh);
            s_fade_mesh = nullptr;
        }
        if (s_layer_mesh)
        {
            AEGfxMeshFree(s_layer_mesh);
            s_layer_mesh = nullptr;
        }
    }
}