#pragma once

// Enum for all possible states
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

// Global variables to manage the state
extern int g_game_state_curr;
extern int g_game_state_prev;
extern int g_game_state_next;

// Function pointers for the current state's lifecycle functions
extern void (*g_game_state_init)(void);
extern void (*g_game_state_update)(void);
extern void (*g_game_state_exit)(void);

void gsm_init(int starting_state);
void gsm_update();