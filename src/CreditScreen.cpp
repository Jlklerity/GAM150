#include "CreditScreen.h"
#include "GameStateManager.h"
#include "SoundManager.h"
#include "MeshRenderer.h"
#include "Enemy.h"
#include <AEEngine.h>
#include <vector>
#include <string>

namespace
{
    float scrollY = 0.0f;
    float scrollSpeed = 0.35f;

    std::vector<std::string> creditLines = {
        "GAM150 PROJECT",
        "",
        "PRODUCED BY",
        "Your Name Here",
        "",
        "PROGRAMMING",
        "Your Name Here",
        "Teammate Name",
        "",
        "ART AND AUDIO",
        "Teammate Name",
        "",
        "SPECIAL THANKS",
        "DigiPen Institute of Technology",
        "Alpha Engine",
        "",
        "THANK YOU FOR PLAYING!"
    };

}

    void CreditScreen_Init()
    {
        // Check if the font is loaded, if not, load it safely
        if (Graphics::font_id < 0) {
            Graphics::font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 48);
        }

        // Start the text slightly below the bottom of the screen (-1.0 is the bottom edge in NDC)
        scrollY = -1.2f;
    }

    void CreditScreen_Update()
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();
        if (dt > 0.1f) dt = 0.01667f; // Safety clamp

        // ---------------------------------------------------------
        //  INPUT (Allow the player to skip the credits early)
        // ---------------------------------------------------------
        if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON))
        {
            SoundManager::PlayClickBtnSfx();
            gGameStateNext = GS_MAIN_MENU; // Go back to main menu
        }

        // ---------------------------------------------------------
        //  UPDATE SCROLLING LOGIC
        // ---------------------------------------------------------
        scrollY += scrollSpeed * dt; // Move the text UP every frame

        float lineSpacing = 0.15f;   // The gap between each line of text

        // Calculate if the credits are completely finished.
        // If the VERY LAST line of text goes off the top of the screen (1.0 is the top edge)
        float lastLineY = scrollY - ((creditLines.size() - 1) * lineSpacing);
        if (lastLineY > 1.2f)
        {
            gGameStateNext = GS_MAIN_MENU; // Auto-return when finished
        }

        // ---------------------------------------------------------
        //  DRAW / RENDER
        // ---------------------------------------------------------
        AEGfxSetCamPosition(0.0f, 0.0f);
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f); // Cinematic black background

        if (Graphics::font_id >= 0)
        {
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);

            for (size_t i = 0; i < creditLines.size(); ++i)
            {
                if (creditLines[i].empty()) continue; // Don't draw blank lines

                float textW = 0.0f, textH = 0.0f;
                float scale = 1.0f;

                // Measure the width of the text so we can perfectly center it
                AEGfxGetPrintSize(Graphics::font_id, creditLines[i].c_str(), scale, &textW, &textH);
                float ndcX = 0.0f - (textW * 0.5f);

                // Calculate this specific line's Y position
                // Subsequent lines render lower down (-Y) from the main scrollY point
                float ndcY = scrollY - (i * lineSpacing);

                // OPTIMIZATION: Only draw the text if it is currently visible on the screen
                if (ndcY > -1.2f && ndcY < 1.2f)
                {
                    // Tint the headers differently (optional polish!)
                    if (creditLines[i] == "PRODUCED BY" || creditLines[i] == "PROGRAMMING" ||
                        creditLines[i] == "ART AND AUDIO" || creditLines[i] == "SPECIAL THANKS") {
                        AEGfxPrint(Graphics::font_id, creditLines[i].c_str(), ndcX, ndcY, scale, 1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                    }
                    else {
                        AEGfxPrint(Graphics::font_id, creditLines[i].c_str(), ndcX, ndcY, scale, 1.0f, 1.0f, 1.0f, 1.0f); // White
                    }
                }
            }
        }

    }