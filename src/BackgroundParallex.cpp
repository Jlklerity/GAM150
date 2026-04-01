#include "pch.h"
#include "BackgroundParallex.h"


namespace
{
    struct ParallaxLayer
    {
        AEGfxTexture* texture;
        float depth_factor;  // 1.0 = foreground, 0.0 = infinitely far away
        float u_offset;      // Tracks the current horizontal position of the image
        float u_speed;       // How fast the image drifts sideways
    };

    const int NUM_LAYERS = 6;
    static ParallaxLayer    s_bg_layers[NUM_LAYERS];
    static AEGfxVertexList* s_full_screen_mesh = nullptr;
    static float            s_manual_pan_offset = 0.0f;
}


namespace BackgroundParallax
{
    void menu_parallax_set_pan_offset(float offset)
    {
        s_manual_pan_offset = offset;
    }

    void menu_parallax_init()
    {
        if (s_full_screen_mesh) return;

        float win_w = (float)AEGfxGetWindowWidth();
        float win_h = (float)AEGfxGetWindowHeight();

        s_full_screen_mesh = Rect_Mesh(win_w, win_h, 0xFFFFFFFF);

        s_bg_layers[0].texture = AEGfxTextureLoad("Assets/sun.png");
        AE_ASSERT_MESG(s_bg_layers[0].texture, "Failed to load sun.png");

        s_bg_layers[1].texture = AEGfxTextureLoad("Assets/cloudextra.png");
        AE_ASSERT_MESG(s_bg_layers[1].texture, "Failed to load cloudextra.png");

        s_bg_layers[2].texture = AEGfxTextureLoad("Assets/layer1.png");
        AE_ASSERT_MESG(s_bg_layers[2].texture, "Failed to load layer1.png");

        s_bg_layers[3].texture = AEGfxTextureLoad("Assets/layer2.png");
        AE_ASSERT_MESG(s_bg_layers[3].texture, "Failed to load layer2.png");

        s_bg_layers[4].texture = AEGfxTextureLoad("Assets/mist.png");
        AE_ASSERT_MESG(s_bg_layers[4].texture, "Failed to load mist.png");

        s_bg_layers[5].texture = AEGfxTextureLoad("Assets/layer3.png");
        AE_ASSERT_MESG(s_bg_layers[4].texture, "Failed to load layer3.png");

        s_bg_layers[0].depth_factor = 0.1f;
        s_bg_layers[1].depth_factor = 0.3f;  // clouds
        s_bg_layers[2].depth_factor = 0.4f;
        s_bg_layers[3].depth_factor = 0.6f;
        s_bg_layers[4].depth_factor = 0.8f;  // mist
        s_bg_layers[5].depth_factor = 0.9f;

        for (int i = 0; i < NUM_LAYERS; ++i)
        {
            s_bg_layers[i].u_offset = 0.0f;
            s_bg_layers[i].u_speed = 0.0f;
        }
        s_bg_layers[1].u_speed = 0.01f; // cloud speed
        s_bg_layers[1].u_offset = 0.0f;
        s_bg_layers[4].u_speed = 0.04f; // mist drifting
        s_bg_layers[4].u_offset = 0.0f;
    }

    void menu_parallax_update(float dt)
    {
        for (int i = 0; i < NUM_LAYERS; ++i)
        {
            s_bg_layers[i].u_offset += s_bg_layers[i].u_speed * dt;

            // Wrap the offset to prevent floating-point precision loss
            if (s_bg_layers[i].u_offset > 1.0f)
                s_bg_layers[i].u_offset -= 1.0f;
            else if (s_bg_layers[i].u_offset < 0.0f)
                s_bg_layers[i].u_offset += 1.0f;
        }
    }

    void menu_parallax_draw(float virtual_cam_y)
    {
        float cam_x, cam_y;
        AEGfxGetCamPosition(&cam_x, &cam_y);

        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

        for (int i = 0; i < NUM_LAYERS; ++i)
        {
            float render_y = cam_y - (virtual_cam_y * s_bg_layers[i].depth_factor);
            float render_offset = s_bg_layers[i].u_offset - (s_manual_pan_offset * s_bg_layers[i].depth_factor);
            AEGfxTextureSet(s_bg_layers[i].texture, render_offset, 0.0f);
            mesh_renderer(s_full_screen_mesh, cam_x, render_y, 1.0f, 1.0f, 0.0f);
        }
    }

    void menu_parallax_exit()
    {
        if (s_full_screen_mesh)
        {
            AEGfxMeshFree(s_full_screen_mesh);
            s_full_screen_mesh = nullptr;
        }

        for (int i = 0; i < NUM_LAYERS; ++i)
        {
            if (s_bg_layers[i].texture)
            {
                AEGfxTextureUnload(s_bg_layers[i].texture);
                s_bg_layers[i].texture = nullptr;
            }
        }
    }

    int menu_parallax_get_layer_count()
    {
        return NUM_LAYERS;
    }

    AEGfxTexture* menu_parallax_get_layer_texture(int index)
    {
        if (index < 0 || index >= NUM_LAYERS) return nullptr;
        return s_bg_layers[index].texture;
    }

    float menu_parallax_get_layer_depth(int index)
    {
        if (index < 0 || index >= NUM_LAYERS) return 0.0f;
        return s_bg_layers[index].depth_factor;
    }

    float menu_parallax_get_layer_u_offset(int index)
    {
        if (index < 0 || index >= NUM_LAYERS) return 0.0f;
        return s_bg_layers[index].u_offset;
    }

    float menu_parallax_get_manual_pan_offset()
    {
        return s_manual_pan_offset;
    }
}