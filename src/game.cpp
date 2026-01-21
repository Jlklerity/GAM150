
#include "Game.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"


namespace {
	AEGfxTexture* avatarTexture = nullptr;
	AEGfxVertexList* avatarMesh = nullptr;

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

	//background mesh and texture
	AEGfxTexture* bgTexture = nullptr;
	AEGfxVertexList* bgMesh = nullptr;

}

void Game_Init()
{
	animTimer = 0.0f;
	animDuration = 0.1f; 
	currentFrameIndex = 0;
	uvOffsetX = 0.0f;
	uvOffsetY = 0.0f;
	 
	//Mesh for Sprite
	avatarTexture = AEGfxTextureLoad("Assets/characterSheet.png");
	AE_ASSERT_MESG(avatarTexture, "Failed to create characterSheet texture!!");
	if (avatarMesh != nullptr) AEGfxMeshFree(avatarMesh); 
	avatarMesh = Sprite_Mesh(SPRITE_UV_WIDTH, SPRITE_UV_HEIGHT);

	// Background
	bgTexture = AEGfxTextureLoad("Assets/background.png");
	AE_ASSERT_MESG(bgTexture, "Failed to load background texture");
	bgMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

}


void Game_Update()
{
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_NONE);
	AEGfxSetTransparency(1.0f);
	AEGfxTextureSet(bgTexture, 0.0f, 0.0f);

	// Full screen (Alpha Engine uses scale, not size)
	Mesh_Renderer(bgMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
	// Update timer
	animTimer += (float)AEFrameRateControllerGetFrameTime();

	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);

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

	AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND); // Allow transparency
	AEGfxSetTransparency(1.0f);


	//Draw Sprite
	AEGfxTextureSet(avatarTexture, uvOffsetX, uvOffsetY);
	Mesh_Renderer(avatarMesh, spriteX, spriteY, 150.0f, 150.0f, 0.0f);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);//switch back for non-sprite meshes

	

}

void Game_Exit()
{
	
	AEGfxMeshFree(avatarMesh);
	AEGfxMeshFree(bgMesh);
	bgMesh = nullptr;

	if (bgTexture)
	{
		AEGfxTextureUnload(bgTexture);
		bgTexture = nullptr;
	}


	if(avatarTexture)
	{
		AEGfxTextureUnload(avatarTexture);
		avatarTexture = nullptr;
	}
}