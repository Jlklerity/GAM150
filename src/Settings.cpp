#include "pch.h"
#include "Settings.h"

namespace
{
    AEGfxVertexList* s_quad = nullptr;

    float bgmVol = 0.5f;
    float sfxVol = 0.8f;
    float ambVol = 0.6f;

    int draggingSlider = -1;

    const float SLIDER_W = 400.0f;
    const float SLIDER_H = 15.0f;
    const float KNOB_W = 20.0f;
    const float KNOB_H = 40.0f;

    const float BGM_Y = 120.0f;
    const float SFX_Y = -10.0f;
    const float AMB_Y = -140.0f;

    const float CLOSE_W = 200.0f;
    const float CLOSE_H = 70.0f;
    const float CLOSE_Y = -300.0f;

    const float BORDER = 5.0f;

    float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

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
        if (!s_quad)
            s_quad = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

        bgmVol = SoundManager::GetBGMVolume();
        sfxVol = SoundManager::GetSFXVolume();
        ambVol = SoundManager::GetAmbientVolume();
    }

    bool DisplaySettings()
    {
        if (!s_quad) Init();

        s32 mouseX, mouseY;
        AEInputGetCursorPosition(&mouseX, &mouseY);
        float winW = (float)AEGfxGetWindowWidth();
        float winH = (float)AEGfxGetWindowHeight();
        float worldX = (float)mouseX - (winW * 0.5f);
        float worldY = (winH * 0.5f) - (float)mouseY;
        float ndcScale = winH > 0 ? (2.0f / winH) : 0.0022f;

        bool mouseHeld = (AEInputCheckCurr(AEVK_LBUTTON) != 0);
        bool mouseTriggered = (AEInputCheckTriggered(AEVK_LBUTTON) != 0);
        float startX = -(SLIDER_W * 0.5f);

        if (!mouseHeld) draggingSlider = -1;

        // Close button / ESC
        if ((mouseTriggered && draggingSlider == -1 &&
            worldX >= -(CLOSE_W * 0.5f) && worldX <= (CLOSE_W * 0.5f) &&
            worldY >= CLOSE_Y - (CLOSE_H * 0.5f) && worldY <= CLOSE_Y + (CLOSE_H * 0.5f))
            || AEInputCheckTriggered(AEVK_ESCAPE))
        {
            SoundManager::PlayClickBtnSfx();
            return false;
        }

        // Grab slider
        if (mouseTriggered)
        {
            float hitBoxW = SLIDER_W + KNOB_W;
            if (worldX >= -(hitBoxW * 0.5f) && worldX <= (hitBoxW * 0.5f))
            {
                if (worldY >= BGM_Y - KNOB_H && worldY <= BGM_Y + KNOB_H) draggingSlider = 0;
                else if (worldY >= SFX_Y - KNOB_H && worldY <= SFX_Y + KNOB_H) draggingSlider = 1;
                else if (worldY >= AMB_Y - KNOB_H && worldY <= AMB_Y + KNOB_H) draggingSlider = 2;
            }
        }

        // Apply drag
        if (draggingSlider != -1)
        {
            float percent = Clamp01((worldX - startX) / SLIDER_W);
            if (draggingSlider == 0) { bgmVol = percent; SoundManager::SetBGMVolume(bgmVol); }
            else if (draggingSlider == 1) { sfxVol = percent; SoundManager::SetSFXVolume(sfxVol); }
            else if (draggingSlider == 2) { ambVol = percent; SoundManager::SetAmbientVolume(ambVol); }
        }

        // ----------------------------------------------------------------
        // RENDERING
        // ----------------------------------------------------------------

        // Fullscreen dark purple overlay
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.75f);
        AEGfxSetColorToMultiply(0.05f, 0.03f, 0.08f, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, 0.0f, winW, winH, 0.0f);

        // Gold border panel
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f); // gold
        Mesh_Renderer(s_quad, 0.0f, -20.0f, 620.0f, 820.0f, 0.0f);

        // Dark purple panel fill
        AEGfxSetColorToMultiply(0.07f, 0.05f, 0.10f, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, -20.0f, 608.0f, 808.0f, 0.0f);

        // Gold divider under title
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, 155.0f, 540.0f, 3.0f, 0.0f);

        // Title — shadow then gold
        // Shadow
        DrawCentredText("SOUND SETTINGS", 0.68f, 0.9f, 0.0f, 0.0f, 0.0f, 1.0f);
        // Gold
        DrawCentredText("SOUND SETTINGS", 0.69f, 0.9f, 1.0f, 0.82f, 0.1f, 1.0f);

        // Sliders
        auto DrawSlider = [&](float yPos, float currentVol, const char* label)
            {
                // Label
                DrawCentredText(label, (yPos + 50.0f) * ndcScale, 0.55f, 0.85f, 0.70f, 0.15f, 1.0f); // gold label

                // Track — gold border then dark fill
                AEGfxSetRenderMode(AE_GFX_RM_COLOR);
                AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                AEGfxSetTransparency(1.0f);
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                Mesh_Renderer(s_quad, 0.0f, yPos, SLIDER_W + BORDER * 2, SLIDER_H + BORDER * 2, 0.0f);

                AEGfxSetColorToMultiply(0.12f, 0.10f, 0.18f, 1.0f);
                Mesh_Renderer(s_quad, 0.0f, yPos, SLIDER_W, SLIDER_H, 0.0f);

                // Gold fill
                float fillW = SLIDER_W * currentVol;
                float fillX = startX + (fillW * 0.5f);
                if (fillW > 0.0f)
                {
                    AEGfxSetColorToMultiply(0.95f, 0.78f, 0.10f, 1.0f);
                    Mesh_Renderer(s_quad, fillX, yPos, fillW, SLIDER_H, 0.0f);
                }

                // Knob — gold border + dark fill
                float knobX = startX + fillW;
                AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
                Mesh_Renderer(s_quad, knobX, yPos, KNOB_W + BORDER, KNOB_H + BORDER, 0.0f);
                AEGfxSetColorToMultiply(0.15f, 0.12f, 0.20f, 1.0f);
                Mesh_Renderer(s_quad, knobX, yPos, KNOB_W, KNOB_H, 0.0f);
            };

        DrawSlider(BGM_Y, bgmVol, "MUSIC VOLUME");
        DrawSlider(SFX_Y, sfxVol, "SFX VOLUME");
        DrawSlider(AMB_Y, ambVol, "AMBIENT VOLUME");

        // Close button — gold border + dark fill
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(0.85f, 0.70f, 0.15f, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, CLOSE_Y, CLOSE_W + BORDER * 2, CLOSE_H + BORDER * 2, 0.0f);
        AEGfxSetColorToMultiply(0.10f, 0.08f, 0.14f, 1.0f);
        Mesh_Renderer(s_quad, 0.0f, CLOSE_Y, CLOSE_W, CLOSE_H, 0.0f);

        // TEXT LABELS
        float ndcScale = winH > 0 ? (2.0f / winH) : 0.0022f;

        DrawCentredText("SOUND SETTINGS", 0.75f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("BACKGROUND VOLUME", (BGM_Y + 45.0f) * ndcScale, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("SFX VOLUME", (SFX_Y + 45.0f) * ndcScale, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("AMBIENT VOLUME", (AMB_Y + 45.0f) * ndcScale, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f);
        DrawCentredText("CLOSE", (CLOSE_Y)*ndcScale, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f);

        return true;
    }

    void Exit()
    {
        if (s_quad) { AEGfxMeshFree(s_quad); s_quad = nullptr; }
    }
}