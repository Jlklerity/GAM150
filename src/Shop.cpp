#include "GameStateManager.h"
#include "Shop.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "Leaderboard.h"
#include "Enemy.h"
#include <AEEngine.h>


int gShopSkinID = 0;
int gShopBgID = 0;

namespace {
	AEGfxTexture* pTexShopBg = nullptr;
	AEGfxVertexList* pShop= nullptr;
	AEGfxVertexList* pBack = nullptr;

	float winW;
	float winH;
	float worldX;
	float worldY;

	float SHOP_BG_WIDTH;
	float SHOP_BG_HEIGHT;
	float BACK_BTN_WIDTH;
	float BACK_BTN_HEIGHT;
	
	int SCORE_AMOUNT;

	float bgLeftX;
	float bgTopY;
	float textX;
	float textY;
	float offsetX;
	float offsetY;

	const float SHOP_BG_Y = 0.0f;
	const float SHOP_BG_X = 0.0f;
	float BACK_BTN_X;
	float BACK_BTN_Y;

}

void DisplayScore() {

	if (Graphics::font_id < 0) return;
	bgLeftX = SHOP_BG_X - (SHOP_BG_WIDTH * 0.5f);
	bgTopY = SHOP_BG_Y + (SHOP_BG_HEIGHT * 0.5f);

	offsetX = SHOP_BG_WIDTH * 0.1f;
	offsetY = SHOP_BG_HEIGHT * 0.27f;

	textX = (bgLeftX + offsetX) / (winW * 0.5f);
	textY = (bgTopY - offsetY) / (winH * 0.5f);
	char s[128];      // text buffer for score
	std::snprintf(s, sizeof(s), "SCORE: %d", SCORE_AMOUNT);    // format score line

	AEGfxSetBlendMode(AE_GFX_BM_BLEND);  // enable blending for text
	AEGfxPrint(Graphics::font_id, s, textX, textY, 1.0f, 1, 1, 1, 1);
}

void Shop_Init() {
	winW = (float)AEGfxGetWindowWidth();
	winH = (float)AEGfxGetWindowHeight();

	SHOP_BG_WIDTH = winH * 1.2f;
	SHOP_BG_HEIGHT = winH * 1.0f;
	BACK_BTN_WIDTH = winW * 0.05f;
	BACK_BTN_HEIGHT = winH * 0.05f;

	BACK_BTN_X = winW * -0.23f;
	BACK_BTN_Y = winH * -0.3f;
	pTexShopBg = AEGfxTextureLoad("Assets/shopBackground.png");
	AE_ASSERT_MESG(pTexShopBg, "Failed to load shopBackground.png");

	pShop = Sprite_Mesh(1.0f, 1.0f);
	pBack = Rect_Mesh(BACK_BTN_WIDTH, BACK_BTN_HEIGHT, 0xFFFFFFFF);

	SCORE_AMOUNT = Leaderboard_System::GetHighestScore();
}

void Shop_Update(){

	AEGfxSetCamPosition(0.0f, 0.0f);
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

	s32 mouseX, mouseY;
	AEInputGetCursorPosition(&mouseX, &mouseY);

	winW = (float)AEGfxGetWindowWidth();
	winH = (float)AEGfxGetWindowHeight();
	// Convert screen coordinates (Top-Left 0,0) to World Coordinates (Center 0,0)
	worldX = (float)mouseX - (winW * 0.5f);
	worldY = (winH * 0.5f) - (float)mouseY;

	if (AEInputCheckCurr(AEVK_ESCAPE)) gGameStateNext = GS_MAIN_MENU;

	if (AEInputCheckTriggered(AEVK_LBUTTON)) {
		if (worldX >= BACK_BTN_X - (BACK_BTN_WIDTH * 0.5f) && worldX <= BACK_BTN_X + (BACK_BTN_WIDTH * 0.5f) &&
			worldY >= BACK_BTN_Y - (BACK_BTN_HEIGHT * 0.5f) && worldY <= BACK_BTN_Y + (BACK_BTN_HEIGHT * 0.5f))
		{
			gGameStateNext = GS_MAIN_MENU;
		}
	}
	if (pTexShopBg)
	{
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(1.0f);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);

		AEGfxTextureSet(pTexShopBg, 0.0f, 0.0f);
		Mesh_Renderer(pShop, SHOP_BG_X, SHOP_BG_Y, SHOP_BG_WIDTH, SHOP_BG_HEIGHT, 0.0f);
	}
	if (pBack)
	{
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);//exit button
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetColorToMultiply(1.0f, 0.0f, 0.0f, 1.0f);
		// Reusing your Exit Button mesh temporarily just to get a box on screen
		Mesh_Renderer(pBack, BACK_BTN_X, BACK_BTN_Y, 1.0f, 1.0f, 0.0f);
	}

	DisplayScore();
}

void Shop_Exit(){
	if (pTexShopBg) {
		AEGfxTextureUnload(pTexShopBg);
		pTexShopBg = nullptr;
	}
	if (pShop) {
		AEGfxMeshFree(pShop);
		pShop = nullptr;
	}
}