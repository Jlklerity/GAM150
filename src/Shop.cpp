#include "pch.h"
#include "Shop.h"
#include "ButtonTween.h"



int gShopSkinID = 0;
int gShopBgID = 0;

namespace {
	AEGfxTexture* pTexShopBg = nullptr;
	AEGfxTexture* pTexBgOptions[3] = { nullptr };
	AEGfxTexture* pTexSkinOptions[3] = { nullptr };
	AEGfxTexture* pTexItemFrame = nullptr;
	AEGfxTexture* pTexBgFrame = nullptr;
	AEGfxTexture* pTexBack = nullptr;

	AEGfxVertexList* pBack = nullptr;
	AEGfxVertexList* pShop = nullptr;
	AEGfxVertexList* pMeshBgItem = nullptr;
	AEGfxVertexList* pMeshSkinItem = nullptr;


	float winW;
	float winH;
	float worldX;
	float worldY;

	float SHOP_BG_WIDTH;
	float SHOP_BG_HEIGHT;
	float BACK_BTN_WIDTH;
	float BACK_BTN_HEIGHT;

	float BG_ITEM_WIDTH;
	float BG_ITEM_HEIGHT;
	float SKIN_ITEM_WIDTH;
	float SKIN_ITEM_HEIGHT;

	float bgLeftX;
	float bgTopY;
	float textX;
	float textY;
	float offsetX;
	float offsetY;

	const float SHOP_BG_Y = 0.0f;
	const float SHOP_BG_X = 0.0f;

	const int SKIN_UNLOCK[3] = { 0, 1500, 3000 };
	const int BG_UNLOCK[3] = { 0, 2000, 5000 };

	float BACK_BTN_X;
	float BACK_BTN_Y;

	float BG_ITEM_X[3];
	float BG_ITEM_Y;

	float SKIN_ITEM_X[3];
	float SKIN_ITEM_Y;
	int SCORE_AMOUNT;

	bool isTransitioningIn = false;
	bool isTransitioningOut = false;
	float shopUiOffsetX = 0.0f;

	// ---------------------------------------------------------------------------
	// BUTTON TWEENS
	// ---------------------------------------------------------------------------
	ButtonTween tweenBack;
	ButtonTween tweenBgItem[3];
	ButtonTween tweenSkinItem[3];
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

	AEGfxSetBlendMode(AE_GFX_BM_BLEND);
	AEGfxPrint(Graphics::font_id, s, textX + shopUiOffsetX, textY, 1.0f, 1, 1, 1, 1);
}

void Shop_Init() {

	isTransitioningIn = true;
	shopUiOffsetX = (float)AEGfxGetWindowWidth(); // Start the UI completely off-screen to the right
	ScreenTransitions::ScreenTransition_Init(); // Restart the timer for the arrival pan

	winW = (float)AEGfxGetWindowWidth();
	winH = (float)AEGfxGetWindowHeight();

	SHOP_BG_WIDTH = winH * 1.2f;
	SHOP_BG_HEIGHT = winH * 1.0f;
	BACK_BTN_WIDTH = 80.0f;
	BACK_BTN_HEIGHT = 80.0f;
	BG_ITEM_WIDTH = winW * 0.2f;
	BG_ITEM_HEIGHT = BG_ITEM_WIDTH * (120.0f / 450.0f);
	SKIN_ITEM_WIDTH = winW * 0.25f;
	SKIN_ITEM_HEIGHT = SKIN_ITEM_WIDTH;

	BG_ITEM_Y = winH * 0.15f;
	SKIN_ITEM_Y = winH * -0.15f;
	BACK_BTN_X = winW * -0.23f;
	BACK_BTN_Y = winH * -0.3f;

	// Backgrounds
	BG_ITEM_X[0] = winW * -0.2f; // Left
	BG_ITEM_X[1] = 0.0f;          // Center
	BG_ITEM_X[2] = winW * 0.2f;  // Right

	// Skins
	SKIN_ITEM_X[0] = winW * -0.20f; // Left
	SKIN_ITEM_X[1] = 0.0f;          // Center
	SKIN_ITEM_X[2] = winW * 0.20f;  // Right


	pTexShopBg = AEGfxTextureLoad("Assets/shopBackground.png");
	AE_ASSERT_MESG(pTexShopBg, "Failed to load shopBackground.png");

	pTexItemFrame = AEGfxTextureLoad("Assets/shopItemFrame.png");
	AE_ASSERT_MESG(pTexItemFrame, "Failed to load shopItemFrame.png");

	pTexBack = AEGfxTextureLoad("Assets/LeftRightButtons.png");
	AE_ASSERT_MESG(pTexBack, "Failed to load LeftRightButtons.png");


	pTexBgFrame = AEGfxTextureLoad("Assets/BackgroundSelectorFrame.png");
	AE_ASSERT_MESG(pTexBgFrame, "Failed to load BackgroundSelectorFrame.png");

	pTexBgOptions[0] = AEGfxTextureLoad("Assets/Background1.png");
	AE_ASSERT_MESG(pTexBgOptions, "Failed to load Background1.png");
	pTexBgOptions[1] = pTexBgOptions[0]; // TODO: Change to bg1.png later
	pTexBgOptions[2] = pTexBgOptions[0]; // TODO: Change to bg2.png later

	// Load Skin Options (Using a placeholder for now)
	pTexSkinOptions[0] = AEGfxTextureLoad("Assets/skin1.png");
	AE_ASSERT_MESG(pTexSkinOptions, "Failed to load skin1.png");
	pTexSkinOptions[1] = AEGfxTextureLoad("Assets/skin2.png");
	AE_ASSERT_MESG(pTexSkinOptions, "Failed to load skin2.png");
	pTexSkinOptions[2] = AEGfxTextureLoad("Assets/skin3.png");
	AE_ASSERT_MESG(pTexSkinOptions, "Failed to load skin3.png");



	pShop = Sprite_Mesh(1.0f, 1.0f);

	pMeshBgItem = Sprite_Mesh(1.0f, 1.0f);
	pMeshSkinItem = Sprite_Mesh(1.0f, 1.0f);
	pBack = Spritesheet_Mesh(1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);

	SCORE_AMOUNT = Leaderboard_System::GetHighestScore();

	ButtonTween_Reset(tweenBack);
	for (int i = 0; i < 3; i++) { ButtonTween_Reset(tweenBgItem[i]); ButtonTween_Reset(tweenSkinItem[i]); }
}

void Shop_Update() {

	AEGfxSetCamPosition(0.0f, 0.0f);
	AEGfxSetBackgroundColor(0.53f, 0.81f, 0.92f);


	float dt = (float)AEFrameRateControllerGetFrameTime();
	if (dt > 0.1f) dt = 0.01667f;
	BackgroundParallax::MenuParallax_Update(dt);

	s32 mouseX, mouseY;
	AEInputGetCursorPosition(&mouseX, &mouseY);

	winW = (float)AEGfxGetWindowWidth();
	winH = (float)AEGfxGetWindowHeight();

	worldX = (float)mouseX - (winW * 0.5f);
	worldY = (winH * 0.5f) - (float)mouseY;

	// ---------------------------------------------------------------------------
	// BUTTON TWEEN UPDATES
	// ---------------------------------------------------------------------------
	if (!isTransitioningIn && !isTransitioningOut)
	{
		float dt_tween = (float)AEFrameRateControllerGetFrameTime();
		bool click = AEInputCheckTriggered(AEVK_LBUTTON);

		bool hBack = (worldX >= BACK_BTN_X - (BACK_BTN_WIDTH * 0.5f) && worldX <= BACK_BTN_X + (BACK_BTN_WIDTH * 0.5f) &&
			worldY >= BACK_BTN_Y - (BACK_BTN_HEIGHT * 0.5f) && worldY <= BACK_BTN_Y + (BACK_BTN_HEIGHT * 0.5f));
		ButtonTween_Update(tweenBack, hBack, hBack && click, dt_tween);

		for (int i = 0; i < 3; i++)
		{
			bool hBg = (worldX >= BG_ITEM_X[i] - (BG_ITEM_WIDTH * 0.5f) && worldX <= BG_ITEM_X[i] + (BG_ITEM_WIDTH * 0.5f) &&
				worldY >= BG_ITEM_Y - (BG_ITEM_HEIGHT * 0.5f) && worldY <= BG_ITEM_Y + (BG_ITEM_HEIGHT * 0.5f));
			ButtonTween_Update(tweenBgItem[i], hBg, hBg && click, dt_tween);

			float hitBoxW = SKIN_ITEM_WIDTH * 0.65f;
			float hitBoxH = SKIN_ITEM_HEIGHT * 0.65f;
			float hitBoxX = SKIN_ITEM_X[i] - 10.0f;
			float hitBoxY = SKIN_ITEM_Y + 60.0f;
			bool hSkin = (worldX >= hitBoxX - (hitBoxW * 0.5f) && worldX <= hitBoxX + (hitBoxW * 0.5f) &&
				worldY >= hitBoxY - (hitBoxH * 0.5f) && worldY <= hitBoxY + (hitBoxH * 0.5f));
			ButtonTween_Update(tweenSkinItem[i], hSkin, hSkin && click, dt_tween);
		}
	}

	if (isTransitioningIn)
	{
		ScreenTransitions::ScreenTransition_Update(dt);

		float smoothPan = ScreenTransitions::EaseOutExpo(ScreenTransitions::result);

		// Slide from the right side towards 0
		shopUiOffsetX = winW * (1.0f - smoothPan);

		BackgroundParallax::MenuParallax_SetPanOffset(-0.2f - (0.2f * smoothPan));

		if (ScreenTransitions::result >= 0.3f)
		{
			shopUiOffsetX = 0.0f;
			isTransitioningIn = false;
			BackgroundParallax::MenuParallax_SetPanOffset(-0.4f); // Lock securely to center
		}
	}
	BackgroundParallax::MenuParallax_Draw(0.0f);

	if (!isTransitioningIn && AEInputCheckTriggered(AEVK_ESCAPE)) {
		isTransitioningOut = true;
		ScreenTransitions::ScreenTransition_Init();
	}

	if (!isTransitioningIn && AEInputCheckTriggered(AEVK_LBUTTON)) {
		// Check Back Button
		if (worldX >= BACK_BTN_X - (BACK_BTN_WIDTH * 0.5f) && worldX <= BACK_BTN_X + (BACK_BTN_WIDTH * 0.5f) &&
			worldY >= BACK_BTN_Y - (BACK_BTN_HEIGHT * 0.5f) && worldY <= BACK_BTN_Y + (BACK_BTN_HEIGHT * 0.5f))
		{
			SoundManager::PlayClickBtnSfx();
			isTransitioningOut = true;
			ScreenTransitions::ScreenTransition_Init();
		}

		// --- CHECK BACKGROUND SELECTION ---
		for (int i = 0; i < 3; i++) {
			if (worldX >= BG_ITEM_X[i] - (BG_ITEM_WIDTH * 0.5f) && worldX <= BG_ITEM_X[i] + (BG_ITEM_WIDTH * 0.5f) &&
				worldY >= BG_ITEM_Y - (BG_ITEM_HEIGHT * 0.5f) && worldY <= BG_ITEM_Y + (BG_ITEM_HEIGHT * 0.5f)) {
				SoundManager::PlayClickBtnSfx();
				if (SCORE_AMOUNT >= BG_UNLOCK[i]) {
					gShopBgID = i;
				}
			}
		}

		// --- CHECK SKIN SELECTION ---
		for (int i = 0; i < 3; i++) {
			float hitBoxW = SKIN_ITEM_WIDTH * 0.65f;
			float hitBoxH = SKIN_ITEM_HEIGHT * 0.65f;
			float hitBoxX = SKIN_ITEM_X[i] - 10.0f;
			float hitBoxY = SKIN_ITEM_Y + 60.0f;

			if (worldX >= hitBoxX - (hitBoxW * 0.5f) && worldX <= hitBoxX + (hitBoxW * 0.5f) &&
				worldY >= hitBoxY - (hitBoxH * 0.5f) && worldY <= hitBoxY + (hitBoxH * 0.5f)) {
				SoundManager::PlayClickBtnSfx();
				if (SCORE_AMOUNT >= SKIN_UNLOCK[i]) {
					gShopSkinID = i;
				}
			}
		}
	}

	if (isTransitioningOut)
	{
		ScreenTransitions::ScreenTransition_Update(dt);
		float smoothPan = ScreenTransitions::EaseOutExpo(ScreenTransitions::result);

		// Slide the Shop UI out to the right
		shopUiOffsetX = winW * smoothPan;
		// Pan the background backwards 
		BackgroundParallax::MenuParallax_SetPanOffset(-0.4f + (0.2f * smoothPan));

		if (ScreenTransitions::result >= 0.3f)
		{

			isTransitioningOut = false;
			BackgroundParallax::MenuParallax_SetPanOffset(0.2f);
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
		Mesh_Renderer(pShop, SHOP_BG_X + shopUiOffsetX, SHOP_BG_Y, SHOP_BG_WIDTH, SHOP_BG_HEIGHT, 0.0f);
	}

	// Draw the 3 Background Options
	for (int i = 0; i < 3; i++) {
		if (pTexBgOptions[i]) {
			if (SCORE_AMOUNT >= BG_UNLOCK[i]) {
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Bright (Unlocked)
			}
			else {
				AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f); // Dark (Locked)
			}
			AEGfxTextureSet(pTexBgOptions[i], 0.0f, 0.0f);
			Mesh_Renderer(pMeshBgItem, BG_ITEM_X[i] + shopUiOffsetX, BG_ITEM_Y, BG_ITEM_WIDTH * 0.7f * tweenBgItem[i].scale, BG_ITEM_HEIGHT * 0.7f * tweenBgItem[i].scale, 0.0f);

			// Only draw the gold item frame if THIS background is the equipped one
			if (gShopBgID == i && pTexBgFrame) {
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
				AEGfxTextureSet(pTexBgFrame, 0.0f, 0.0f);
				Mesh_Renderer(pMeshBgItem, SKIN_ITEM_X[i] + shopUiOffsetX, BG_ITEM_Y, BG_ITEM_WIDTH * 0.7f, BG_ITEM_HEIGHT * 0.7f, 0.0f);
			}

			if ((SCORE_AMOUNT < BG_UNLOCK[i]) && Graphics::font_id >= 0) {
				char priceStr[32];
				std::snprintf(priceStr, sizeof(priceStr), "%d", BG_UNLOCK[i]);

				float textScale = 0.8f;
				float textW = 0.0f, textH = 0.0f;
				AEGfxGetPrintSize(Graphics::font_id, priceStr, textScale, &textW, &textH);


				float textNdcX = (BG_ITEM_X[i] / (winW * 0.5f)) - (textW * 0.5f);
				float textNdcY = (BG_ITEM_Y / (winH * 0.5f)) - 0.025f;
				AEGfxSetBlendMode(AE_GFX_BM_BLEND);
				AEGfxPrint(Graphics::font_id, priceStr, textNdcX + shopUiOffsetX, textNdcY, textScale, 1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
	}

	// Draw the 3 Skin Options
	for (int i = 0; i < 3; i++) {
		if (pTexSkinOptions[i]) {
			if (SCORE_AMOUNT >= SKIN_UNLOCK[i]) {
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f); // Bright (Unlocked)
			}
			else {
				AEGfxSetColorToMultiply(0.3f, 0.3f, 0.3f, 1.0f); // Dark (Locked)
			}
			AEGfxTextureSet(pTexSkinOptions[i], 0.0f, 0.0f);
			Mesh_Renderer(pMeshSkinItem, SKIN_ITEM_X[i] + shopUiOffsetX, SKIN_ITEM_Y, SKIN_ITEM_WIDTH * tweenSkinItem[i].scale, SKIN_ITEM_HEIGHT * tweenSkinItem[i].scale, 0.0f);


			if (gShopSkinID == i && pTexItemFrame) {
				AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
				AEGfxTextureSet(pTexItemFrame, 0.0f, 0.0f);
				float frameScale = 0.65f;

				Mesh_Renderer(pMeshSkinItem, SKIN_ITEM_X[i] - 10.0f + shopUiOffsetX, SKIN_ITEM_Y + 60.0f, SKIN_ITEM_WIDTH * frameScale, SKIN_ITEM_HEIGHT * frameScale, 0.0f);
			}

			if ((SCORE_AMOUNT < SKIN_UNLOCK[i]) && Graphics::font_id >= 0) {
				char priceStr[32];
				std::snprintf(priceStr, sizeof(priceStr), "%d", SKIN_UNLOCK[i]);

				float textScale = 0.8f;
				float textW = 0.0f, textH = 0.0f;
				AEGfxGetPrintSize(Graphics::font_id, priceStr, textScale, &textW, &textH);

				float textNdcX = (SKIN_ITEM_X[i] / (winW * 0.5f)) - (textW * 0.5f);
				float textNdcY = ((SKIN_ITEM_Y + 60.0f) / (winH * 0.5f)) - 0.025f;
				AEGfxSetBlendMode(AE_GFX_BM_BLEND);
				AEGfxPrint(Graphics::font_id, priceStr, textNdcX + shopUiOffsetX, textNdcY, textScale, 1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
	}

	if (pBack && pTexBack)
	{
		AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);//exit button
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
		AEGfxTextureSet(pTexBack, 0.0f, 0.0f);
		Mesh_Renderer(pBack, BACK_BTN_X + shopUiOffsetX, BACK_BTN_Y, BACK_BTN_WIDTH * tweenBack.scale, BACK_BTN_HEIGHT * tweenBack.scale, 0.0f);
	}

	DisplayScore();
}

const char* Shop_GetActiveSkinPath()
{
	if (gShopSkinID == 0) return "Assets/characterSheet.png";
	if (gShopSkinID == 1) return "Assets/character2Sheet.png";
	if (gShopSkinID == 2) return "Assets/catskin-Sheet.png";

	return "Assets/characterSheet.png"; // Safe fallback to default skin
}

void Shop_Exit() {
	if (gGameStateNext != GS_MAIN_MENU) {
		BackgroundParallax::MenuParallax_Exit();
	}

	if (pTexShopBg) {
		AEGfxTextureUnload(pTexShopBg);
		pTexShopBg = nullptr;
	}
	if (pTexItemFrame) {
		AEGfxTextureUnload(pTexItemFrame);
		pTexItemFrame = nullptr;
	}
	if (pTexBgFrame) {
		AEGfxTextureUnload(pTexBgFrame);
		pTexBgFrame = nullptr;
	}

	// Null out any aliases BEFORE unloading to prevent double-free
	// Currently [1] and [2] alias [0], so clear them first
	if (pTexBgOptions[1] == pTexBgOptions[0]) pTexBgOptions[1] = nullptr;
	if (pTexBgOptions[2] == pTexBgOptions[0]) pTexBgOptions[2] = nullptr;
	if (pTexBgOptions[0]) {
		AEGfxTextureUnload(pTexBgOptions[0]);
		pTexBgOptions[0] = nullptr;
	}
	// These will only fire if real unique textures were assigned (i.e. TODOs completed)
	if (pTexBgOptions[1]) { AEGfxTextureUnload(pTexBgOptions[1]); pTexBgOptions[1] = nullptr; }
	if (pTexBgOptions[2]) { AEGfxTextureUnload(pTexBgOptions[2]); pTexBgOptions[2] = nullptr; }

	// Same alias-safe pattern for skin options
	// Currently [2] aliases [0]
	if (pTexSkinOptions[2] == pTexSkinOptions[0]) pTexSkinOptions[2] = nullptr;
	if (pTexSkinOptions[0]) {
		AEGfxTextureUnload(pTexSkinOptions[0]);
		pTexSkinOptions[0] = nullptr;
	}
	if (pTexSkinOptions[1]) {
		AEGfxTextureUnload(pTexSkinOptions[1]);
		pTexSkinOptions[1] = nullptr;
	}
	// Will only fire if a real unique texture was assigned to [2]
	if (pTexSkinOptions[2]) { AEGfxTextureUnload(pTexSkinOptions[2]); pTexSkinOptions[2] = nullptr; }

	if (pTexBack) {
		AEGfxTextureUnload(pTexBack);
		pTexBack = nullptr;
	}
	if (pShop) {
		AEGfxMeshFree(pShop);
		pShop = nullptr;
	}
	if (pBack) {
		AEGfxMeshFree(pBack);
		pBack = nullptr;
	}
	if (pMeshBgItem) {
		AEGfxMeshFree(pMeshBgItem);
		pMeshBgItem = nullptr;
	}
	if (pMeshSkinItem) {
		AEGfxMeshFree(pMeshSkinItem);
		pMeshSkinItem = nullptr;
	}
}