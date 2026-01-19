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

	//Mesh for player
	playerX = 0.0f, playerY = 0.0f;
	player_radius = 100.0f;
	int player_slices = 32;
	float player_angleStep = (2.0f * PI) / player_slices;
	u32 player_color = 0xFF0000FF;

	playerMesh = Circle_Mesh(player_slices, player_radius, player_angleStep, player_color);

	//Mesh for PlusHP
	int plusHP_slices = 32;
	plusHP_radius = 200.0f;
	plusHPX = -500.0f, plusHPY = -100.0f;
	float plusHP_angleStep = (2.0f * PI) / plusHP_slices;
	u32 plusHP_color = 0xFF00FF00;

	plusHPMesh = Circle_Mesh(plusHP_slices, plusHP_radius, plusHP_angleStep, plusHP_color);


	//Mesh for MinusHP
	int minusHP_slices = 32;
	float minusHP_angleStep = (2.0f * PI) / minusHP_slices;
	minusHP_radius = 200.0f;
	minusHPX = 500.0f, minusHPY = -100.0f;
	u32 minusHP_color = 0xFFFF0000;

	minusHPMesh = Circle_Mesh(minusHP_slices, minusHP_radius, minusHP_angleStep, minusHP_color);


	//Mesh for LayerHPBar
	float LayerHPBar_width = 6.0f;
	float LayerHPBar_height = 0.35f;
	layerHPBarX = 0.0f, layerHPBarY = 350.0f;
	u32 layerHPBar_color = 0xFFA52A2A;

	layerHPBarMesh = Rect_Mesh(LayerHPBar_width, LayerHPBar_height, layerHPBar_color);

	//Mesh for HPBar
	float HPBar_width = 6.0f;
	float HPBar_height = 0.35f;
	HPBarScaleX = 200.0f, HPBarScaleY = 200.0f;
	HPBarX = 0.0f, HPBarY = 350.0f;
	u32 HPBar_color = 0xFFFF0000;

	HPBarMesh = Rect_Mesh(HPBar_width, HPBar_height, HPBar_color);

	//Mesh for 10 HP bars
	segment_width = HPBar_width / 10.0f - 0.05f;
	segment_height = HPBar_height;
	u32 segment_color = 0xFFFF0000;
	segmentMesh = Rect_Mesh(segment_width, segment_height, segment_color);
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
	if (HPBarScaleX < 0.0f) HPBarScaleX = 0.0f;
	if (HPBarScaleX > 200.0f) HPBarScaleX = 200.0f;

	if (HPBarX < -600.0f) HPBarX = -600.0f;
	if (HPBarX > 0.0f) HPBarX = 0.0f;

	int activeSegments = (int)ceil((HPBarScaleX / 200.0f) * 10.0f);

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

	// Draw PlusHP
	Mesh_Renderer(plusHPMesh, plusHPX, plusHPY, 1.0f, 1.0f, 0.0f);
	// Draw MinusHP
	Mesh_Renderer(minusHPMesh, minusHPX, minusHPY, 1.0f, 1.0f, 0.0f);
	// Draw Player
	Mesh_Renderer(playerMesh, playerX, playerY, 1.0f, 1.0f, 0.0f);
	// Draw Layer HP Bar (Background)
	Mesh_Renderer(layerHPBarMesh, layerHPBarX, layerHPBarY, 200.0f, 200.0f, 0.0f);
	// Draw Active HP Bar
	Mesh_Renderer(HPBarMesh, HPBarX, HPBarY, HPBarScaleX, HPBarScaleY, 0.0f);

	for (int i = 0; i < 10; i++)
	{
		if (i < activeSegments)
		{
			float segX = -540.0f + (i * 120.0f);
			float segY = 250.0f;  	 
			Mesh_Renderer(segmentMesh, segX, segY, HPBarScaleX, HPBarScaleY, 0.0f);
			
		}
	}
}

void Game_Exit()
{
	AEGfxMeshFree(playerMesh);
	AEGfxMeshFree(avatarMesh);
	AEGfxMeshFree(plusHPMesh);
	AEGfxMeshFree(minusHPMesh);
	AEGfxMeshFree(layerHPBarMesh);
	AEGfxMeshFree(HPBarMesh);
	AEGfxMeshFree(segmentMesh);
	if(avatarTexture)
	{
		AEGfxTextureUnload(avatarTexture);
		avatarTexture = nullptr;
	}
}