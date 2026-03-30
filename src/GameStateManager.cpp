#include "pch.h"
#include "GameStateManager.h"


// Define global variables
int g_game_state_curr = GS_NONE;
int g_game_state_prev = GS_NONE;
int g_game_state_next = GS_NONE;

// Function pointers for the current state's lifecycle
void (*g_game_state_init)(void) = nullptr;
void (*g_game_state_update)(void) = nullptr;
void (*g_game_state_exit)(void) = nullptr;


void GSM_Init(int starting_state)
{
    g_game_state_curr = GS_NONE;
    g_game_state_prev = GS_NONE;
    g_game_state_next = starting_state;

    g_game_state_init = nullptr;
    g_game_state_update = nullptr;
    g_game_state_exit = nullptr;
}

void GSM_Update()
{
    if (g_game_state_curr != g_game_state_next)
    {
        // Exit the current state (if valid)
        if (g_game_state_exit)
            g_game_state_exit();

        // Switch state variables
        g_game_state_prev = g_game_state_curr;
        g_game_state_curr = g_game_state_next;

        // Set the function pointers based on the new state
        switch (g_game_state_curr)
        {
        case GS_MAIN_MENU:
            g_game_state_init = Main_Menu_Init;
            g_game_state_update = Main_Menu_Update;
            g_game_state_exit = Main_Menu_Exit;
            break;

        case GS_GAME:
            g_game_state_init = Game_Init;
            g_game_state_update = Game_Update;
            g_game_state_exit = Game_Exit;
            break;

        case GS_GAME_TIMEMODE:
            g_game_state_init = GameTimeMode_Init;
            g_game_state_update = GameTimeMode_Update;
            g_game_state_exit = GameTimeMode_Exit;
            break;

        case GS_SHOP:
            g_game_state_init = Shop_Init;
            g_game_state_update = Shop_Update;
            g_game_state_exit = Shop_Exit;
            break;

        case GS_CREDITS:
            g_game_state_init = CreditScreen_Init;
            g_game_state_update = CreditScreen_Update;
            g_game_state_exit = CreditScreen_Exit;
            break;

        case GS_QUIT:
            // Will handle the actual quitting in main
            g_game_state_init = nullptr;
            g_game_state_update = nullptr;
            g_game_state_exit = nullptr;
            break;

        case GS_TUTORIAL:
            g_game_state_init = Tutorial_Init;
            g_game_state_update = Tutorial_Update;
            g_game_state_exit = Tutorial_Exit;
            break;

        case GS_RESTART:
            // Switches immediately back to the previous state
            g_game_state_next = g_game_state_prev;
            g_game_state_init = nullptr;  // prevent double-init
            g_game_state_update = nullptr;  // prevent double-update
            g_game_state_exit = nullptr;
            break;

        default:
            break;
        }

        // Initialize the new state
        if (g_game_state_init)
            g_game_state_init();

        AEFrameRateControllerReset();
    }
}