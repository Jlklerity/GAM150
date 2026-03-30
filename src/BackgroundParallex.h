#pragma once
#include <AEEngine.h>

namespace BackgroundParallax {

    void MenuParallax_SetPanOffset(float offset);
    void MenuParallax_Init();
    void MenuParallax_Draw(float virtual_cam_y);
    void MenuParallax_Update(float dt);
    void MenuParallax_Exit();

    int           MenuParallax_GetLayerCount();
    AEGfxTexture* MenuParallax_GetLayerTexture(int index);
    float         MenuParallax_GetLayerDepth(int index);
    float         MenuParallax_GetLayerUOffset(int index);
    float         MenuParallax_GetManualPanOffset();
}