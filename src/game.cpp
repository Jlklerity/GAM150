#include "Game.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Typing.h"
#include "Shop.h"
#include "GameStateManager.h"

namespace {
	//avatar mesh and texture
	AEGfxTexture* playerTexture1 = nullptr; // Default Skin
	AEGfxTexture* playerTexture2 = nullptr; // Alternate Skin
	AEGfxVertexList* playerMesh = nullptr;

	//background mesh and texture
	AEGfxTexture* bgTexture1 = nullptr; // Default BG
	AEGfxTexture* bgTexture2 = nullptr; // Alternate BG
	AEGfxVertexList* bgMesh = nullptr;

	const float spriteX = -600.0f;
	const float spriteY = -200.0f;

	// Animation State
	float animTimer;
	float animDuration; // Speed of animation
	int currentFrameIndex;
	float uvOffsetX;
	float uvOffsetY;

	// Constants for the spritesheet
	const int SPRITE_COLS = 8;
	const int SPRITE_ROWS = 1;
	const float SPRITE_UV_WIDTH = 1.0f / SPRITE_COLS;
	const float SPRITE_UV_HEIGHT = 1.0f / SPRITE_ROWS;
  
	// Toggle for Shop state
	bool isShopOpen = false;
}

void Game_Init()
{
	animTimer = 0.0f;
	animDuration = 0.1f; 
	currentFrameIndex = 0;
	uvOffsetX = 0.0f;
	uvOffsetY = 0.0f;
	
	//player
	playerTexture1 = AEGfxTextureLoad("Assets/characterSheet.png");
	playerTexture2 = AEGfxTextureLoad("Assets/characterSheet.png");//for ltr when we have more player skins
	AE_ASSERT_MESG(playerTexture1, "Failed to load characterSheet!");


	if (playerMesh != nullptr) AEGfxMeshFree(playerMesh);
	playerMesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

	// Background
	bgTexture1 = AEGfxTextureLoad("Assets/background.png");
	bgTexture2 = AEGfxTextureLoad("Assets/background.png"); //for ltr when we have more bg skins
	AE_ASSERT_MESG(bgTexture1, "Failed to load background!");

	
	bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

	Typing_Init();
	Shop_Init();
}


void Game_Update()
{
	if (AEInputCheckTriggered(AEVK_TAB)) {
		isShopOpen = !isShopOpen; //toggle shop menu and resume gameplay
	}

	if (AEInputCheckTriggered(AEVK_ESCAPE)) {//press q to return to main menu
		gGameStateNext = GS_MAIN_MENU;
	}
	//background
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_NONE);
	AEGfxSetTransparency(1.0f);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Reset Color

	AEGfxTexture* activeBg = (gShopBgID == 0) ? bgTexture1 : bgTexture2;
	AEGfxTextureSet(activeBg, 0.0f, 0.0f);
	// Optional tint for demo purposes if textures are identical
	if (gShopBgID == 1) AEGfxSetColorToMultiply(0.5f, 0.5f, 0.7f, 1.0f); // change background color
	Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

	//player
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

	AEGfxTexture* activePlayer = (gShopSkinID == 0) ? playerTexture1 : playerTexture2;
	AEGfxTextureSet(activePlayer, uvOffsetX, uvOffsetY);

	if (gShopSkinID == 1) AEGfxSetColorToMultiply(1.0f, 0.6f, 0.6f, 1.0f); // change player skin color
	Mesh_Renderer(playerMesh, spriteX, spriteY, 150.0f, 150.0f, 0.0f);

	if (isShopOpen)
	{
		// If shop is open, just run the Shop Module
		Shop_Update();
	}
	else 
	{
		Typing_Update();
		Typing_Render();

		// Update timer
		animTimer += (float)AEFrameRateControllerGetFrameTime();

		if (animTimer >= animDuration)
		{
			animTimer = 0.0f;

			// Move to next frame
			currentFrameIndex++;
			if (currentFrameIndex >= 8) // Max sprites
				currentFrameIndex = 0;

			// Calculate UV Offset (Which "window" of the image to look at)
			int col = currentFrameIndex % SPRITE_COLS;
			int row = currentFrameIndex / SPRITE_COLS;

			uvOffsetX = col * SPRITE_UV_WIDTH;
			uvOffsetY = row * SPRITE_UV_HEIGHT;
		}
	}
}

void Game_Exit()
{
	Typing_Exit();
	Shop_Exit();
	if (playerMesh) {
		AEGfxMeshFree(playerMesh);
		playerMesh = nullptr;
	}

	if (bgMesh) {
		AEGfxMeshFree(bgMesh);
		bgMesh = nullptr;
	}

	if (bgTexture1)
	{
		AEGfxTextureUnload(bgTexture1);
		bgTexture1 = nullptr;
	}
	if (bgTexture2)
	{
		AEGfxTextureUnload(bgTexture2);
		bgTexture2 = nullptr;
	}

	if(playerTexture1)
	{
		AEGfxTextureUnload(playerTexture1);
		playerTexture1 = nullptr;
	}
	if (playerTexture2)
	{
		AEGfxTextureUnload(playerTexture2);
		playerTexture2 = nullptr;
	}
}