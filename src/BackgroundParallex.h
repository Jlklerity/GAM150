#pragma once

namespace BackgroundParallax {
    
    void MenuParallax_SetPanOffset(float offset);
    void MenuParallax_Init();
    void MenuParallax_Draw(float virtualCamY);
    void MenuParallax_Update(float dt);
    void MenuParallax_Exit();
}