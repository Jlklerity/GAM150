// ---------------------------------------------------------------------------
// includes

#include <crtdbg.h> // To check for memory leaks
#include "GameStateManager.h"
#include "game.h"
#include "MainMenu.h"
#include "Enemy.h"
#include "SoundManager.h"

// ---------------------------------------------------------------------------
// main

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	int gGameRunning = 1;
	//_CrtSetBreakAlloc(61465);
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, nullptr);

	AESysSetWindowTitle("Revenge of YK");

	// reset the system modules
	AESysReset();
	GSM_Init(GS_MAIN_MENU);
	SoundManager::Init();   // load once, globally

	// Game Loop
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		// Basic way to trigger exiting the application
		// when ESCAPE is hit or when the window is closed
		AESysFrameStart();
		if (!AESysDoesWindowExist())
			gGameRunning = 0;

		GSM_Update();

		// run the current state's Update function
		if (gGameStateUpdate)
			gGameStateUpdate();

		// checks if the GSM set the state to quit
		if (gGameStateCurr == GS_QUIT)
			gGameRunning = 0;

		AESysFrameEnd();

	}
	// clean up the last active state
	if (gGameStateExit)
		gGameStateExit();

	if (Graphics::font_id >= 0) {
		AEGfxDestroyFont(Graphics::font_id);
		Graphics::font_id = -1;
	}
	SoundManager::Exit();

	// free the system
	AESysExit();

	_CrtDumpMemoryLeaks();
}