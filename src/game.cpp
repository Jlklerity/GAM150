#include "pch.h"
#include "Game.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Utils.h"

namespace {
	AEGfxTexture* avatarTexture = nullptr;
	AEGfxVertexList* avatarMesh = nullptr;
	AEGfxVertexList* playerMesh = nullptr;
	AEGfxVertexList* plusHPMesh = nullptr;
	AEGfxVertexList* minusHPMesh = nullptr;
	AEGfxVertexList* layerHPBarMesh = nullptr;
	AEGfxVertexList* HPBarMesh = nullptr;
	AEGfxVertexList* segmentMesh = nullptr;

	float spriteX, spriteY;
	float playerX, playerY;
	float player_radius;
	float minusHP_radius;
	float plusHP_radius;
	float minusHPX, minusHPY;
	float plusHPX, plusHPY;
	float HPBarX, HPBarY;
	float layerHPBarX, layerHPBarY;
	int TenHPBar[10];
	float segment_width, segment_height, segmentY;
	float HPBarScaleX, HPBarScaleY;
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

}


void Game_Update()
{

	if (AEInputCheckCurr(AEVK_W)) {
		playerY += 10.0f;
	}
	if (AEInputCheckCurr(AEVK_S)) {
		playerY -= 10.0f;
	}
	if (AEInputCheckCurr(AEVK_A)) {
		playerX -= 10.0f;
	}
	if (AEInputCheckCurr(AEVK_D)) {
		playerX += 10.0f;
	}
	
	if (AreCirclesIntersecting(player_radius, minusHP_radius, playerX, playerY, minusHPX, minusHPY)) {
		HPBarMinus(&HPBarX, &HPBarScaleX);
	 
	}
	if (AreCirclesIntersecting(player_radius, plusHP_radius, playerX, playerY, plusHPX, plusHPY)) {
		HPBarPlus(&HPBarX, &HPBarScaleX);
		 
	}

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

	AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);
	AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND); // Allow transparency
	AEGfxSetTransparency(1.0f);


	//Draw Sprite
	AEGfxTextureSet(avatarTexture, uvOffsetX, uvOffsetY);
	Mesh_Renderer(avatarMesh, spriteX, spriteY, 100.0f, 100.0f, 0.0f);

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);//switch back for non-sprite meshes

	//// Draw PlusHP
	//Mesh_Renderer(plusHPMesh, plusHPX, plusHPY, 1.0f, 1.0f, 0.0f);
	//// Draw MinusHP
	//Mesh_Renderer(minusHPMesh, minusHPX, minusHPY, 1.0f, 1.0f, 0.0f);
	//// Draw Player
	//Mesh_Renderer(playerMesh, playerX, playerY, 1.0f, 1.0f, 0.0f);
	//// Draw Layer HP Bar (Background)
	//Mesh_Renderer(layerHPBarMesh, layerHPBarX, layerHPBarY, 200.0f, 200.0f, 0.0f);
	//// Draw Active HP Bar
	//Mesh_Renderer(HPBarMesh, HPBarX, HPBarY, HPBarScaleX, HPBarScaleY, 0.0f);

}

void Game_Exit()
{
	
	AEGfxMeshFree(avatarMesh);
	
	if(avatarTexture)
	{
		AEGfxTextureUnload(avatarTexture);
		avatarTexture = nullptr;
	}
}