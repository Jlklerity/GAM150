#include <AEEngine.h>
#include <AEMtx33.h>
#include "meshes.h"
#include "utils.h"


AEGfxVertexList* playerMesh = NULL;
AEGfxVertexList* plusHPMesh = NULL;
AEGfxVertexList* minusHPMesh = NULL;
AEGfxVertexList* layerHPBarMesh = NULL;
AEGfxVertexList* HPBarMesh = NULL;
AEGfxVertexList* segmentMesh = NULL;

static float playerX;
static float playerY;
static float player_radius;
static float minusHP_radius;
static float plusHP_radius;
static float minusHPX, minusHPY;
static float plusHPX, plusHPY;
static float HPBarX, HPBarY;
static float layerHPBarX, layerHPBarY;
static int TenHPBar[10];
static float segment_width, segment_height, segmentY;
static float HPBarScaleX, HPBarScaleY;
 

void Game_Init()
{
	//Mesh for player
	playerX = 0.0f, playerY = 0.0f;
	player_radius = 100.0f;
	int player_slices = 32;
	float player_angleStep = (2.0f * PI) / player_slices;
	u32 player_color = 0xFF0000FF;

	playerMesh = CircleMesh(player_slices, player_radius, player_angleStep, player_color);

	//Mesh for PlusHP
	int plusHP_slices = 32;
	plusHP_radius = 200.0f;
	plusHPX = -500.0f, plusHPY = -100.0f;
	float plusHP_angleStep = (2.0f * PI) / plusHP_slices;
	u32 plusHP_color = 0xFF00FF00;

	plusHPMesh = CircleMesh(plusHP_slices, plusHP_radius, plusHP_angleStep, plusHP_color);


	//Mesh for MinusHP
	int minusHP_slices = 32;
	float minusHP_angleStep = (2.0f * PI) / minusHP_slices;
	minusHP_radius = 200.0f;
	minusHPX = 500.0f, minusHPY = -100.0f;
	u32 minusHP_color = 0xFFFF0000;

	minusHPMesh = CircleMesh(minusHP_slices, minusHP_radius, minusHP_angleStep, minusHP_color);


	//Mesh for LayerHPBar
	float LayerHPBar_width = 6.0f;
	float LayerHPBar_height = 0.35f;
	layerHPBarX = 0.0f, layerHPBarY = 350.0f;
	u32 layerHPBar_color = 0xFFA52A2A;

	layerHPBarMesh = RectMesh(LayerHPBar_width, LayerHPBar_height, layerHPBar_color);

	//Mesh for HPBar
	float HPBar_width = 6.0f;
	float HPBar_height = 0.35f;
	HPBarScaleX = 200.0f, HPBarScaleY = 200.0f;
	HPBarX = 0.0f, HPBarY = 350.0f;
	u32 HPBar_color = 0xFFFF0000;

	HPBarMesh = RectMesh(HPBar_width, HPBar_height, HPBar_color);

	//Mesh for 10 HP bars
	segment_width = HPBar_width / 10.0f - 0.05f;
	segment_height = HPBar_height;
	u32 segment_color = 0xFFFF0000;
	segmentMesh = RectMesh(segment_width, segment_height, segment_color);
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

	AEGfxSetBackgroundColor(0.5f, 0.5f, 0.5f);

	//transformation = scaling * rotation * translation 
	AEMtx33 scale, rot, trans, player, plusHP, minusHP, layerHPBar, HPBar;


	AEMtx33Scale(&scale, 1.0f, 1.0f); //scaling
	AEMtx33Rot(&rot, 0.0f);  //rotation
	AEMtx33Trans(&trans, plusHPX, plusHPY); //translation
	AEMtx33Concat(&plusHP, &rot, &scale); //player = rot * scale
	AEMtx33Concat(&plusHP, &trans, &plusHP); //player = (rot * scale) * translate

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransform(plusHP.m);
	AEGfxMeshDraw(plusHPMesh, AE_GFX_MDM_TRIANGLES);


	AEMtx33Scale(&scale, 1.0f, 1.0f); //scaling
	AEMtx33Rot(&rot, 0.0f);  //rotation
	AEMtx33Trans(&trans, minusHPX, minusHPY); //translation
	AEMtx33Concat(&minusHP, &rot, &scale); //player = rot * scale
	AEMtx33Concat(&minusHP, &trans, &minusHP); //player = (rot * scale) * translate

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransform(minusHP.m);
	AEGfxMeshDraw(minusHPMesh, AE_GFX_MDM_TRIANGLES);


	AEMtx33Scale(&scale, 1.0f, 1.0f); //scaling
	AEMtx33Rot(&rot, 0.0f);  //rotation
	AEMtx33Trans(&trans, playerX, playerY); //translation
	AEMtx33Concat(&player, &rot, &scale); //player = rot * scale
	AEMtx33Concat(&player, &trans, &player); //player = (rot * scale) * translate

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransform(player.m);
	AEGfxMeshDraw(playerMesh, AE_GFX_MDM_TRIANGLES);


	AEMtx33Scale(&scale, 200.0f, 200.0f); //scaling
	AEMtx33Rot(&rot, 0.0f);  //rotation
	AEMtx33Trans(&trans, 0.0f, 350.0f); //translation
	AEMtx33Concat(&layerHPBar, &rot, &scale); //player = rot * scale
	AEMtx33Concat(&layerHPBar, &trans, &layerHPBar); //player = (rot * scale) * translate

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransform(layerHPBar.m);
	AEGfxMeshDraw(layerHPBarMesh, AE_GFX_MDM_TRIANGLES);

	AEMtx33Scale(&scale, HPBarScaleX, HPBarScaleY); //scaling
	AEMtx33Rot(&rot, 0.0f);  //rotation
	AEMtx33Trans(&trans, HPBarX, HPBarY); //translation
	AEMtx33Concat(&HPBar, &rot, &scale); //player = rot * scale
	AEMtx33Concat(&HPBar, &trans, &HPBar); //player = (rot * scale) * translate

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransform(HPBar.m);
	AEGfxMeshDraw(HPBarMesh, AE_GFX_MDM_TRIANGLES);

	for (int i = 0; i < 10; i++)
	{
		
		if (i < activeSegments)
		{

			float segX = -540.0f + (i * 120.0f);
			float segY = 250.0f;  

			 
			AEMtx33Rot(&rot, 0.0f);
			AEMtx33Trans(&trans, segX, segY);

			AEMtx33Concat(&HPBar, &rot, &scale);
			AEMtx33Concat(&HPBar, &trans, &HPBar);

			// 3. Set the transform and draw
			AEGfxSetRenderMode(AE_GFX_RM_COLOR);
			AEGfxSetTransform(HPBar.m);
			if (i < activeSegments)
			{
				// Active segments (Red)
				AEGfxMeshDraw(segmentMesh, AE_GFX_MDM_TRIANGLES);
			}
		}
	}

}

void Game_Exit()
{
	AEGfxMeshFree(playerMesh);
	AEGfxMeshFree(plusHPMesh);
	AEGfxMeshFree(minusHPMesh);
	AEGfxMeshFree(layerHPBarMesh);
	AEGfxMeshFree(HPBarMesh);
	AEGfxMeshFree(segmentMesh);
}