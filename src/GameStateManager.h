#pragma once

//enum for all possible states
enum GameState
{
	GS_NONE = 0,
	GS_MAIN_MENU,
	GS_GAME,
	GS_GAME_TIMEMODE,
	GS_SHOP,
	GS_CREDITS,
	GS_TUTORIAL,
	GS_QUIT,
	GS_RESTART
};

//global variables to manage the state
extern int gGameStateCurr;
extern int gGameStatePrev;
extern int gGameStateNext;

//function pointers for the current state's lifecycle functions
extern void (*gGameStateInit)(void);
extern void (*gGameStateUpdate)(void);
extern void (*gGameStateExit)(void);

void GSM_Init(int startingState);
void GSM_Update();