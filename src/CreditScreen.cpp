#include "pch.h"
#include "CreditScreen.h"


namespace
{
    float scrollY = 0.0f;
    float scrollSpeed = 0.35f;

    const char* creditLines[] = {
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
    const int numCreditLines = sizeof(creditLines) / sizeof(creditLines[0]);
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
        float lastLineY = scrollY - ((numCreditLines - 1) * lineSpacing);
        if (lastLineY > 1.2f)
        {
            gGameStateNext = GS_MAIN_MENU; // Auto-return when finished
        }

        // ---------------------------------------------------------
        //  DRAW / RENDER
        // ---------------------------------------------------------
        AEGfxSetCamPosition(0.0f, 0.0f);
        AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f); // Cinematic black background

        // 4. Draw the Text
        if (Graphics::font_id >= 0)
        {
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);

            // Changed to numCreditLines
            for (int i = 0; i < numCreditLines; ++i)
            {
                std::string line = creditLines[i]; // Temporary string for easy checking
                if (line.empty()) continue;

                float textW = 0.0f, textH = 0.0f;
                float scale = 1.0f;

                AEGfxGetPrintSize(Graphics::font_id, creditLines[i], scale, &textW, &textH);
                float ndcX = 0.0f - (textW * 0.5f);
                float ndcY = scrollY - (i * lineSpacing);

                if (ndcY > -1.2f && ndcY < 1.2f)
                {
                    if (line == "PRODUCED BY" || line == "PROGRAMMING" ||
                        line == "ART AND AUDIO" || line == "SPECIAL THANKS") {
                        AEGfxPrint(Graphics::font_id, creditLines[i], ndcX, ndcY, scale, 1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                    }
                    else {
                        AEGfxPrint(Graphics::font_id, creditLines[i], ndcX, ndcY, scale, 1.0f, 1.0f, 1.0f, 1.0f); // White
                    }
                }
            }
        }

    }

    void CreditScreen_Exit()
    {
        if (Graphics::font_id >= 0)
        {
            AEGfxDestroyFont(Graphics::font_id);
            Graphics::font_id = -1;
        }
    }

 