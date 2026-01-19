// ---------------------------------------------------------------------------
// includes

#include "pch.h"
#include <crtdbg.h> // To check for memory leaks
#include "game.h"
#include "utils.h"

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
	
	// Initialization of your own variables go here

	// Using custom window procedure
	AESysInit(hInstance, nCmdShow, 1600, 900, 1, 60, false, nullptr);

	// Changing the window title
	AESysSetWindowTitle("My New Demo!");

	// reset the system modules
	AESysReset();

	Game_Init();

	// Game Loop
	while (gGameRunning)
	{
		// Informing the system about the loop's start
		// Basic way to trigger exiting the application
		// when ESCAPE is hit or when the window is closed
		AESysFrameStart();
		if (AEInputCheckTriggered(AEVK_ESCAPE) || 0 == AESysDoesWindowExist())
			gGameRunning = 0;
		Game_Update();
		// Informing the system about the loop's end
		AESysFrameEnd();

	}

	Game_Exit();

	// free the system
	AESysExit();
}