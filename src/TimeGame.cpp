#include "TimeGame.h"
#include "GameStateManager.h"
#include <AEEngine.h>

void GameTimeMode_Init() {

}
void GameTimeMode_Update() {
	AEGfxSetCamPosition(0.0f, 0.0f);
	AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

	if (AEInputCheckCurr(AEVK_ESCAPE)) gGameStateNext = GS_MAIN_MENU;
}
void GameTimeMode_Exit() {

}