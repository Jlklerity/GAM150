#include "Settings.h"
#include "MainMenu.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include "SoundManager.h"
#include <AEEngine.h>

namespace {
	AEGfxVertexList* pSettingsMeshOverlay = nullptr;
	bool isInit = false;
}

namespace SysSettings {
	void Settings_Exit()
	{
		if (pSettingsMeshOverlay)
		{
			AEGfxMeshFree(pSettingsMeshOverlay);
			pSettingsMeshOverlay = nullptr;
		}
		isInit = false;
	}

	bool DisplaySettings() {

		if (!isInit) {
			pSettingsMeshOverlay = Sprite_Mesh(1.0f, 1.0f);
			isInit = true;
		}
		// ... inside update/input logic ...
		//if (/* User clicked Volume UP button */)
		//{
		//	currentVolume += 0.1f;
		//	SoundManager::SetMasterVolume(currentVolume);
		//}
		float winW = (float)AEGfxGetWindowWidth();
		float winH = (float)AEGfxGetWindowHeight();

		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(0.7f);  
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);  

		Mesh_Renderer(pSettingsMeshOverlay, 0.0f, 0.0f, winW, winH, 0.0f);

		AEGfxSetTransparency(1.0f); // Reset transparency for UI drawn 

		AEGfxSetCamPosition(0.0f, 0.0f);
		AEGfxSetBackgroundColor(0.0f, 0.0f, 0.0f);

		if (AEInputCheckCurr(AEVK_ESCAPE)) return false;

		return true;
	}

}