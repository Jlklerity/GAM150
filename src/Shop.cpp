#include "GameStateManager.h"
#include "Shop.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include <AEEngine.h>

namespace {

	AEGfxVertexList* pMeshShopOverlay = nullptr;
	AEGfxVertexList* pMeshShopBtn = nullptr;

	bool DrawAndCheckButton(float btnX, float btnY, u32 color, float mouseX, float mouseY, bool mouseClicked)
	{
		// 1. Draw the Button Box
		// Extract RGB
		float r = ((color >> 16) & 0xFF) / 255.0f;
		float g = ((color >> 8) & 0xFF) / 255.0f;
		float b = ((color) & 0xFF) / 255.0f;

		AEGfxSetColorToMultiply(r, g, b, 1.0f);
		Mesh_Renderer(pMeshShopBtn, btnX, btnY, 1.0f, 1.0f, 0.0f);

		// 2. Check Collision (AABB)
		// Button size is 200x50
		if (mouseClicked &&
			mouseX >= btnX - 100.0f && mouseX <= btnX + 100.0f &&
			mouseY >= btnY - 25.0f && mouseY <= btnY + 25.0f)
		{
			return true; // clicked
		}

		return false; // not clicked
	
	}
}

int gShopSkinID = 0;
int gShopBgID = 0;

void Shop_Init() {

	gShopSkinID = 0;
	gShopBgID = 0;

	// --- 4. Init Shop UI ---
	pMeshShopOverlay = Rect_Mesh(1600.0f, 900.0f, 0xAA000000);
	pMeshShopBtn = Rect_Mesh(200.0f, 50.0f, 0xFFFFFFFF);
}

void Shop_Update(){
	 
	if (AEInputCheckTriggered(AEVK_ESCAPE)) {
		gGameStateNext = GS_MAIN_MENU;
	}
	// Draw Overlay
	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxSetTransparency(1.0f);

	// Reset any tinting
	AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
	AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

	Mesh_Renderer(pMeshShopOverlay, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	// Mouse Input
	s32 mouseX, mouseY;
	AEInputGetCursorPosition(&mouseX, &mouseY);
	// Convert to World Coords
	float worldX = (float)mouseX - 1600.0f / 2.0f;
	float worldY = -((float)mouseY - 900.0f / 2.0f);
	bool clicked = AEInputCheckTriggered(AEVK_LBUTTON);

	// Green Button -> Select Skin 0
	if (DrawAndCheckButton(-200.0f, 100.0f, 0x00FF00, worldX, worldY, clicked))
	{
		gShopSkinID = 0;
	}
	// Red Button -> Select Skin 1
	if (DrawAndCheckButton(200.0f, 100.0f, 0xFF0000, worldX, worldY, clicked))
	{
		gShopSkinID = 1;
	}
	// Light Grey Button -> Select BG 0
	if (DrawAndCheckButton(-200.0f, -100.0f, 0xCCCCCC, worldX, worldY, clicked))
	{
		gShopBgID = 0;
	}
	// Dark Blue Button -> Select BG 1
	if (DrawAndCheckButton(200.0f, -100.0f, 0x0000AA, worldX, worldY, clicked))
	{
		gShopBgID = 1;
	}
}

void Shop_Exit(){

	if (pMeshShopOverlay) {
		AEGfxMeshFree(pMeshShopOverlay);
		pMeshShopOverlay = nullptr;
	}
	if (pMeshShopBtn) {
		AEGfxMeshFree(pMeshShopBtn);
		pMeshShopBtn = nullptr;
	}
}