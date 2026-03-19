#include "pch.h"
#include "GameClouds.h"


namespace
{
    AEGfxTexture* s_cloudTex = nullptr;
    AEGfxVertexList* s_cloudMesh = nullptr;

    float s_uOffset = 0.0f;

    // How fast the clouds drift - tweak to taste
    const float CLOUD_SPEED = 0.008f;
}

namespace GameClouds
{
    void Init()
    {
        if (s_cloudTex)
        {
            AEGfxTextureUnload(s_cloudTex);
            s_cloudTex = nullptr;
        }

        if (s_cloudMesh)
        {
            AEGfxMeshFree(s_cloudMesh);
            s_cloudMesh = nullptr;
        }
        s_uOffset = 0.0f;

        s_cloudTex = AEGfxTextureLoad("Assets/cloudextra.png");
        AE_ASSERT_MESG(s_cloudTex, "Failed to load cloudextra.png");

        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        s_cloudMesh = Rect_Mesh(winW, winH, 0xFFFFFFFF);
    }

    void Update(float dt)
    {
        s_uOffset += CLOUD_SPEED * dt;
        if (s_uOffset > 1.0f)
            s_uOffset -= 1.0f;
    }

    void Draw()
    {
        if (!s_cloudTex || !s_cloudMesh) return;

        // Fade clouds out as night falls
        float cloudAlpha = 1.0f - DayCycle::GetNightAlpha();
        if (cloudAlpha <= 0.01f) return;

        float camX, camY;
        AEGfxGetCamPosition(&camX, &camY);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(cloudAlpha);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        AEGfxTextureSet(s_cloudTex, s_uOffset, 0.0f);
        Mesh_Renderer(s_cloudMesh, camX, camY + 180.0f, 1.0f, 1.0f, 0.0f);
    }

    void Exit()
    {
        if (s_cloudTex)
        {
            AEGfxTextureUnload(s_cloudTex);
            s_cloudTex = nullptr;
        }
        if (s_cloudMesh)
        {
            AEGfxMeshFree(s_cloudMesh);
            s_cloudMesh = nullptr;
        }
    }
}