// File: GameCLouds.cpp
// Author: Pengfei
#include "pch.h"
#include "GameClouds.h"


namespace
{
    AEGfxTexture* s_cloud_tex = nullptr;
    AEGfxVertexList* s_cloud_mesh = nullptr;

    float s_u_offset = 0.0f;

    // How fast the clouds drift - tweak to taste
    const float CLOUD_SPEED = 0.008f;
}

namespace GameClouds
{
    // loads the cloud texture and builds the fullscreen mesh, safe to call on restart since it
    // unloads any previously loaded assets first, resets the scroll offset back to zero
    void init()
    {
        if (s_cloud_tex)
        {
            AEGfxTextureUnload(s_cloud_tex);
            s_cloud_tex = nullptr;
        }

        if (s_cloud_mesh)
        {
            AEGfxMeshFree(s_cloud_mesh);
            s_cloud_mesh = nullptr;
        }
        s_u_offset = 0.0f;

        s_cloud_tex = AEGfxTextureLoad("Assets/cloudextra.png");
        AE_ASSERT_MESG(s_cloud_tex, "Failed to load cloudextra.png");

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();
        s_cloud_mesh = Rect_Mesh(win_w, win_h, 0xFFFFFFFF);
    }

    // advances the UV scroll offset each frame to make the clouds drift horizontally,
    // param is dt in seconds, wraps back to 0 when offset exceeds 1 to keep the UV in range
    void update(float dt)
    {
        s_u_offset += CLOUD_SPEED * dt;
        if (s_u_offset > 1.0f)
            s_u_offset -= 1.0f;
    }

    // draws the scrolling cloud layer at the top of the screen, fades out as night falls using DayCycle::get_night_alpha(),
    // skips drawing entirely if the alpha is near zero so we don't waste draw calls at full night,
    // follows the camera position so clouds stay locked to the screen
    void draw()
    {
        if (!s_cloud_tex || !s_cloud_mesh) return;

        // Fade clouds out as night falls
        float cloud_alpha = 1.0f - DayCycle::get_night_alpha();
        if (cloud_alpha <= 0.01f) return;

        float cam_x, cam_y;
        AEGfxGetCamPosition(&cam_x, &cam_y);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(cloud_alpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_cloud_tex, s_u_offset, 0.0f);
        mesh_renderer(s_cloud_mesh, cam_x, cam_y + 180.0f, 1.0f, 1.0f, 0.0f);
    }

    // frees the cloud texture and mesh, call this in the game state exit function to avoid memory leaks
    void cleanup()
    {
        if (s_cloud_tex)
        {
            AEGfxTextureUnload(s_cloud_tex);
            s_cloud_tex = nullptr;
        }
        if (s_cloud_mesh)
        {
            AEGfxMeshFree(s_cloud_mesh);
            s_cloud_mesh = nullptr;
        }
    }
}