#include "BackgroundParallex.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include <AEEngine.h>
namespace {

    struct ParallaxLayer {
        AEGfxTexture* texture;
        float depthFactor; // 1.0 = foreground, 0.0 = infinitely far away
        float uOffset; // Tracks the current horizontal position of the image
        float uSpeed;  // How fast the image drifts sideways
    };

    const int NUM_LAYERS = 5;
    ParallaxLayer bgLayers[NUM_LAYERS];
    AEGfxVertexList* fullScreenMesh = nullptr;

}


namespace BackgroundParallex{
    
    void MenuParallax_Init() 
    {
        float winW = (float)AEGfxGetWindowWidth();                                                                                    // store window width
        float winH = (float)AEGfxGetWindowHeight();

        fullScreenMesh = Rect_Mesh(winW, winH, 0xFFFFFFFF);                     // Assuming you use your custom function
        
        bgLayers[0].texture = AEGfxTextureLoad("Assets/sun.png");
        AE_ASSERT_MESG(bgLayers[0].texture, "Failed to load sun.png");

        // Clouds
        bgLayers[1].texture = AEGfxTextureLoad("Assets/clouds.png");
        AE_ASSERT_MESG(bgLayers[1].texture, "Failed to load clouds.png");

        // Far Mountains 
        bgLayers[2].texture = AEGfxTextureLoad("Assets/layer1.png");
        AE_ASSERT_MESG(bgLayers[2].texture, "Failed to load layer1.png");

        // Middle Mountains
        bgLayers[3].texture = AEGfxTextureLoad("Assets/layer2.png");
        AE_ASSERT_MESG(bgLayers[3].texture, "Failed to load layer2.png");

        // Foreground Mountains 
        bgLayers[4].texture = AEGfxTextureLoad("Assets/layer3.png");
        AE_ASSERT_MESG(bgLayers[4].texture, "Failed to load layer3.png");                                                     // check texture success


        bgLayers[0].depthFactor = 0.1f;
        bgLayers[1].depthFactor = 0.5f;
        bgLayers[2].depthFactor = 0.4f;
        bgLayers[3].depthFactor = 0.6f;
        bgLayers[4].depthFactor = 0.9f;

        for (int i = 0; i < NUM_LAYERS; ++i)
        {
            bgLayers[i].uOffset = 0.0f; // Start at 0
            bgLayers[i].uSpeed = 0.0f;  // Default: do not drift horizontally
        }
        bgLayers[1].uSpeed = 0.01f; // cloud speed
    }
    void MenuParallax_Update(float dt)
    {
        for (int i = 0; i < NUM_LAYERS; ++i)
        {
            // Increase the offset by the speed * delta time
            bgLayers[i].uOffset += bgLayers[i].uSpeed * dt;

            // Wrap the offset to prevent floating-point precision loss
            // If a player sits on the menu for 5 hours,  stops the texture from glitching out!
            if (bgLayers[i].uOffset > 1.0f)
            {
                bgLayers[i].uOffset -= 1.0f;
            }
        }
    }
    void MenuParallax_Draw(float virtualCamY)
    {
        float camX, camY;
        AEGfxGetCamPosition(&camX, &camY);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        for (int i = 0; i < NUM_LAYERS; ++i)
        {

            float renderY = camY - (virtualCamY * bgLayers[i].depthFactor);          // Add the physical camera so it stays properly aligned on the screen
            AEGfxTextureSet(bgLayers[i].texture, bgLayers[i].uOffset, 0.0f);
            Mesh_Renderer(fullScreenMesh, camX, renderY, 1.0f, 1.0f, 0.0f);
        }
    }

    void MenuParallax_Exit() {

        if (fullScreenMesh) {
            AEGfxMeshFree(fullScreenMesh);
            fullScreenMesh = nullptr;
        }

        for (int i = 0; i < NUM_LAYERS; ++i) {
            if (bgLayers[i].texture) {
                AEGfxTextureUnload(bgLayers[i].texture);
                bgLayers[i].texture = nullptr;
            }
        }
    }
}