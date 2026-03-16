

#include "ParallaxZoom.h"
#include "BackgroundParallex.h"
#include "GameStateManager.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "AEEngine.h"
#include <cmath>

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
    int   s_targetState = 0;
    bool  s_switched = false;

    AEGfxVertexList* s_fadeMesh = nullptr;
    AEGfxVertexList* s_layerMesh = nullptr;

    // Ease-in cubic: slow start then accelerates
    float EaseInCubic(float x)
    {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        return x * x * x;
    }

    // Remap x in [lo, hi] to [0, 1], clamped
    float Remap01(float x, float lo, float hi)
    {
        if (x <= lo) return 0.0f;
        if (x >= hi) return 1.0f;
        return (x - lo) / (hi - lo);
    }
}

namespace ParallaxZoom
{
    void Start(int targetState)
    {
        s_active = true;
        s_timer = 0.0f;
        s_targetState = targetState;
        s_switched = false;

        //float winW = (float)AEGfxGetWindowWidth();
        //float winH = (float)AEGfxGetWindowHeight();

        if (!s_fadeMesh)  s_fadeMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
        if (!s_layerMesh) s_layerMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF); // 1x1, scaled in Draw
    }

    void Update(float dt)
    {
        if (!s_active) return;

        s_timer += dt;
        float t = s_timer / ZOOM_DURATION;
        if (t > 1.0f) t = 1.0f;

        if (!s_switched && t >= STATE_SWITCH_T)
        {
            gGameStateNext = s_targetState;
            s_switched = true;
        }

        if (t >= 1.0f)
            s_active = false;
    }

    void Draw(float vCamY)
    {
        if (!s_active || !s_layerMesh) return;

        float t = s_timer / ZOOM_DURATION;
        if (t > 1.0f) t = 1.0f;
        float easedT = EaseInCubic(t);

        float camX, camY;
        AEGfxGetCamPosition(&camX, &camY);
        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();

        // Sky background
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_NONE);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.53f, 0.81f, 0.92f, 1.0f);
        Mesh_Renderer(s_fadeMesh, camX, camY, winW, winH, 0.0f);

        // Draw each layer
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        int layerCount = BackgroundParallax::MenuParallax_GetLayerCount();
        float panOffset = BackgroundParallax::MenuParallax_GetManualPanOffset();

        for (int i = 0; i < layerCount; ++i)
        {
            AEGfxTexture* tex = BackgroundParallax::MenuParallax_GetLayerTexture(i);
            float depth = BackgroundParallax::MenuParallax_GetLayerDepth(i);
            float uOffset = BackgroundParallax::MenuParallax_GetLayerUOffset(i);

            if (!tex) continue;

            float zoom = 1.0f + (depth * ZOOM_STRENGTH * easedT);
            float renderY = camY - (vCamY * depth);
            float renderOffset = uOffset - (panOffset * depth);

            AEGfxTextureSet(tex, renderOffset, 0.0f);
            Mesh_Renderer(s_layerMesh, camX, renderY, winW * zoom, winH * zoom, 0.0f);
        }

        // Fade to black
        float fadeAlpha = Remap01(t, FADE_START_T, 1.0f);
        if (fadeAlpha > 0.0f && s_fadeMesh)
        {
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetTransparency(fadeAlpha);
            AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
            Mesh_Renderer(s_fadeMesh, camX, camY, winW, winH, 0.0f);
        }
    }

    bool IsActive()
    {
        return s_active;
    }

    void Reset()
    {
        s_active = false;
        s_timer = 0.0f;
        s_switched = false;

        if (s_fadeMesh)
        {
            AEGfxMeshFree(s_fadeMesh);
            s_fadeMesh = nullptr;
        }
        if (s_layerMesh)
        {
            AEGfxMeshFree(s_layerMesh);
            s_layerMesh = nullptr;
        }
    }
}