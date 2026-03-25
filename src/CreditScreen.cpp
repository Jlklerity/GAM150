#include "pch.h"
#include "CreditScreen.h"


namespace
{
    float scrollY = 0.0f;
    float scrollSpeed = 0.35f;
    AEGfxVertexList* pMeshCreditsLogo = nullptr;
    AEGfxTexture* pTexCreditsLogo = nullptr;

    const float LOGO_WIDTH = 1000.0f;                                                                                        // width of title logo
    const float LOGO_HEIGHT = 600.0f;

    const char* creditLines[] = {
        "BY",
        "FAREWELL YK",
        "",
        "All content (c) 2026 DigiPen Institute of Technology Singapore.",
        "All Rights Reserved.",
        "",
        "PROGRAMMERS",
        "ZI YUAN",
        "PENG FEI",
        "AZFAR",
        "JEREMY LIM",
        "",
        "ART DESIGNERS",
        "PENG FEI",
        "AZFAR",
        "",
        "SOUND DESIGNERS",
        "ZI YUAN",
        "PENG FEI",
        "AZFAR",
        "JEREMY LIM",
        "",
        "TOOLS AND LIBRARIES",
        "ALPHA ENGINE",
        "metadigger",
        "aseprite on steam",
        "piskel",
        "",
        "FACULTY AND ADVISORS",
        "GERALD WONG HAN FENG",
        "DR. SOROOR MALEKMOHAMMADAI FARADOUNBEH",
        "TOMMY TAN CHEE WEI",
        "",
        "",
        "CREATED AT DIGIPEN INSTITUTE OF TECHNOLOGY SINGAPORE",
        "",
        "",
        "PRESIDENT",
        "Claude Comair",
        "",
        "EXECUTIVES",
        "Jason Chu",
        "Samir Abou Samra",
        "Michele Comair",
        "Angela Kugler",
        "Erik Mohrmann",
        "Benjamin Ellinger",
        "Melvin Gonsalvez",
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

        // ---> NEW: Load the logo texture and mesh <---
        pTexCreditsLogo = AEGfxTextureLoad("Assets/title.png");
        pMeshCreditsLogo = Sprite_Mesh(1.0f, 1.0f); // Assuming Sprite_Mesh is used globally
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

        if (pTexCreditsLogo && pMeshCreditsLogo)
        {
            float winH = (float)AEGfxGetWindowHeight();

            // Map the NDC scrollY (-1.2 to 1.2) into World Y coordinates
            // We add a slight offset (+0.3f) so it sits perfectly in the empty space we made
            float logoWorldY = (scrollY + 0.3f) * (winH * 0.5f);
            float logoWorldX = 10.0f;

            AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

            AEGfxTextureSet(pTexCreditsLogo, 0.0f, 0.0f);

            // Render the logo (X: 0, Y: logoWorldY, Width: 600, Height: 360)
            Mesh_Renderer(pMeshCreditsLogo, logoWorldX, logoWorldY, LOGO_WIDTH, LOGO_HEIGHT, 0.0f);

            // Reset back to color mode for the text
            AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        }

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
                    
                     if (line == "TEAM MEMBERS" || line == "FACULTY AND ADVISORS" || 
                        line == "PROGRAMMERS" || line == "ART DESIGNERS" || line == "SOUND DESIGNERS" ||
                        line == "SPECIAL THANKS" || line == "TOOLS AND LIBRARIES" ||
          
                        line == "PRESIDENT" || line == "EXECUTIVES")
                        {
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
        if (pMeshCreditsLogo) {
            AEGfxMeshFree(pMeshCreditsLogo);
            pMeshCreditsLogo = nullptr;
        }
        if (pTexCreditsLogo) {
            AEGfxTextureUnload(pTexCreditsLogo);
            pTexCreditsLogo = nullptr;
        }
    }

 