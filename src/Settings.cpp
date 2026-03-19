#include "Settings.h"
#include "AEEngine.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "SoundManager.h"
#include "Enemy.h" // Grabs your Graphics::font_id

namespace
{
    AEGfxVertexList* s_quad = nullptr;

    // Current volume levels synced to SoundManager
    float bgmVol = 0.5f;
    float sfxVol = 0.8f;
    float ambVol = 0.6f;

    // Dragging state (-1 = none, 0 = BGM, 1 = SFX, 2 = AMB)
    int draggingSlider = -1;

    // Slider UI Dimensions
    const float SLIDER_W = 400.0f;
    const float SLIDER_H = 15.0f;
    const float KNOB_W = 20.0f;
    const float KNOB_H = 40.0f;

    const float BGM_Y = 120.0f;
    const float SFX_Y = -10.0f;
    const float AMB_Y = -140.0f;

    const float CLOSE_W = 200.0f;
    const float CLOSE_H = 80.0f;
    const float CLOSE_Y = -300.0f;

    // Helper to clamp float between 0 and 1
    float Clamp01(float v) {
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    // Helper to draw text
    void DrawCentredText(const char* text, float ndcY, float scale, float r, float g, float b, float alpha)
    {
        if (Graphics::font_id < 0) return;
        float w, h;
        AEGfxGetPrintSize(Graphics::font_id, text, scale, &w, &h);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::font_id, text, -w * 0.5f, ndcY - (h * 0.5f), scale, r, g, b, alpha);
    }
}

namespace SysSettings
{
    void Init()
    {
        if (!s_quad) {
            s_quad = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
        }

        // Sync local variables to current SoundManager volume
        bgmVol = SoundManager::GetBGMVolume();
        sfxVol = SoundManager::GetSFXVolume();
        ambVol = SoundManager::GetAmbientVolume();
    }

    bool DisplaySettings()
    {
        // Safety check to initialize mesh
        if (!s_quad) Init();

        // Get Mouse Position in World Space
        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);
        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        float worldX = (float)mouseX - (winW * 0.5f);
        float worldY = (winH * 0.5f) - (float)mouseY;

        // Handle Inputs
        bool mouseHeld = (AEInputCheckCurr(AEVK_LBUTTON) != 0);
        bool mouseTriggered = (AEInputCheckTriggered(AEVK_LBUTTON) != 0);

        float startX = -(SLIDER_W * 0.5f);
        float endX = (SLIDER_W * 0.5f);

        // Reset drag if mouse is released
        if (!mouseHeld) {
            draggingSlider = -1;
        }

        // Close button or Escape Key check
        if ((mouseTriggered && draggingSlider == -1 &&
            worldX >= -(CLOSE_W * 0.5f) && worldX <= (CLOSE_W * 0.5f) &&
            worldY >= CLOSE_Y - (CLOSE_H * 0.5f) && worldY <= CLOSE_Y + (CLOSE_H * 0.5f))
            || AEInputCheckTriggered(AEVK_ESCAPE))
        {
            SoundManager::PlayClickBtnSfx();
            return false; // Return false to exit the settings loop
        }

        // Check for grabbing a slider knob or track
        if (mouseTriggered)
        {
            float hitBoxW = SLIDER_W + KNOB_W;
            if (worldX >= -(hitBoxW * 0.5f) && worldX <= (hitBoxW * 0.5f)) {
                if (worldY >= BGM_Y - KNOB_H && worldY <= BGM_Y + KNOB_H) draggingSlider = 0;
                else if (worldY >= SFX_Y - KNOB_H && worldY <= SFX_Y + KNOB_H) draggingSlider = 1;
                else if (worldY >= AMB_Y - KNOB_H && worldY <= AMB_Y + KNOB_H) draggingSlider = 2;
            }
        }

        // Apply mouse position to volume if currently dragging
        if (draggingSlider != -1)
        {
            float percent = Clamp01((worldX - startX) / SLIDER_W);

            if (draggingSlider == 0) {
                bgmVol = percent;
                SoundManager::SetBGMVolume(bgmVol);
            }
            else if (draggingSlider == 1) {
                sfxVol = percent;
                SoundManager::SetSFXVolume(sfxVol);
            }
            else if (draggingSlider == 2) {
                ambVol = percent;
                SoundManager::SetAmbientVolume(ambVol);
            }
        }

        // RENDERING

        // Dark background overlay
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.7f); // 70% opacity
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f); // Black
        Mesh_Renderer(s_quad, 0.0f, 0.0f, winW, winH, 0.0f);

        // Panel Background
        AEGfxSetTransparency(0.95f);
        AEGfxSetColorToMultiply(0.1f, 0.15f, 0.25f, 1.0f); // Dark blue-grey
        Mesh_Renderer(s_quad, 0.0f, 0.0f, 600.0f, 800.0f, 0.0f);

        // Lambda helper to draw a single slider
        auto DrawSlider = [&](float yPos, float currentVol) {
            // Track (Dark Grey)
            AEGfxSetTransparency(1.0f);
            AEGfxSetColorToMultiply(0.2f, 0.2f, 0.2f, 1.0f);
            Mesh_Renderer(s_quad, 0.0f, yPos, SLIDER_W, SLIDER_H, 0.0f);

            // Fill (Bright Blue)
            float fillW = SLIDER_W * currentVol;
            float fillX = startX + (fillW * 0.5f);
            if (fillW > 0.0f) {
                AEGfxSetColorToMultiply(0.0f, 0.7f, 1.0f, 1.0f);
                Mesh_Renderer(s_quad, fillX, yPos, fillW, SLIDER_H, 0.0f);
            }

            // Knob (White)
            float knobX = startX + fillW;
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
            Mesh_Renderer(s_quad, knobX, yPos, KNOB_W, KNOB_H, 0.0f);
            };

        // Draw Sliders
        DrawSlider(BGM_Y, bgmVol);
        DrawSlider(SFX_Y, sfxVol);
        DrawSlider(AMB_Y, ambVol);

        // Draw Close Button
        AEGfxSetColorToMultiply(0.8f, 0.2f, 0.2f, 1.0f); // Red
        Mesh_Renderer(s_quad, 0.0f, CLOSE_Y, CLOSE_W, CLOSE_H, 0.0f);

        // TEXT LABELS
        float ndcScale = winH > 0 ? (2.0f / winH) : 0.0022f;

        DrawCentredText("SOUND SETTINGS", 0.75f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("MUSIC VOLUME", (BGM_Y + 45.0f) * ndcScale, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("SFX VOLUME", (SFX_Y + 45.0f) * ndcScale, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("AMBIENT VOLUME", (AMB_Y + 45.0f) * ndcScale, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("CLOSE", (CLOSE_Y)*ndcScale, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);

        return true;
    }

    void Exit()
    {
        if (s_quad) {
            AEGfxMeshFree(s_quad);
            s_quad = nullptr;
        }
    }
}