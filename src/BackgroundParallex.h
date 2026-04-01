#pragma once
#include <AEEngine.h>

namespace BackgroundParallax {

    void menu_parallax_set_pan_offset(float offset);
    void menu_parallax_init();
    void menu_parallax_draw(float virtual_cam_y);
    void menu_parallax_update(float dt);
    void menu_parallax_exit();

    int           menu_parallax_get_layer_count();
    AEGfxTexture* menu_parallax_get_layer_texture(int index);
    float         menu_parallax_get_layer_depth(int index);
    float         menu_parallax_get_layer_u_offset(int index);
    float         menu_parallax_get_manual_pan_offset();
}