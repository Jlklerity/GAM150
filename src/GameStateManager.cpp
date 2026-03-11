#include "GameStateManager.h"
#include "MainMenu.h"
#include "game.h"
#include "Shop.h"
#include "TimeGame.h"
#include <AEEngine.h>

//define global variables
int gGameStateCurr = GS_NONE;
int gGameStatePrev = GS_NONE;
int gGameStateNext = GS_NONE;

// Function pointers for the current state's lifecycle
void (*gGameStateInit)(void) = nullptr;
void (*gGameStateUpdate)(void) = nullptr;
void (*gGameStateExit)(void) = nullptr;


void GSM_Init(int startingState)
{
	gGameStateCurr = GS_NONE;
	gGameStatePrev = GS_NONE;
	gGameStateNext = startingState;

	gGameStateInit = nullptr;
	gGameStateUpdate = nullptr;
	gGameStateExit = nullptr;
}

void GSM_Update()
{
	
	if (gGameStateCurr != gGameStateNext)
	{
		// exit the current state (if valid)
		if (gGameStateExit)
			gGameStateExit();

		// switch state variables
		gGameStatePrev = gGameStateCurr;
		gGameStateCurr = gGameStateNext;

		// set the function pointers based on the new state
		switch (gGameStateCurr)
		{
		case GS_MAIN_MENU:
			gGameStateInit = Main_Menu_Init;
			gGameStateUpdate = Main_Menu_Update;
			gGameStateExit = Main_Menu_Exit;
			break;

		case GS_GAME:
			gGameStateInit = Game_Init;
			gGameStateUpdate = Game_Update;
			gGameStateExit = Game_Exit;
			break;
		case GS_GAME_TIMEMODE: 
			gGameStateInit = GameTimeMode_Init; 
			gGameStateUpdate = GameTimeMode_Update;
			gGameStateExit = GameTimeMode_Exit;
			break;
		case GS_SHOP:
			gGameStateInit = Shop_Init;
			gGameStateUpdate = Shop_Update;
			gGameStateExit = Shop_Exit;
			break;
		case GS_QUIT:
			// will handle the actual quitting in main
			gGameStateInit = nullptr;
			gGameStateUpdate = nullptr;
			gGameStateExit = nullptr;
			break;

		case GS_RESTART:
			// switches immediately back to the previous state 
			gGameStateNext = gGameStatePrev;
			break;

		default:
			break;
		}

		// initialize the new state 
		if (gGameStateInit)
			gGameStateInit();

		AEFrameRateControllerReset();
	}
}