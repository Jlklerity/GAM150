// File: GameStateManager.cpp
// Author: Jeremy

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


void gsm_init(int starting_state)
{
    g_game_state_curr = GS_NONE;
    g_game_state_prev = GS_NONE;
    g_game_state_next = starting_state;

    g_game_state_init = nullptr;
    g_game_state_update = nullptr;
    g_game_state_exit = nullptr;
}

void gsm_update()
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
            g_game_state_init = main_menu_init;
            g_game_state_update = main_menu_update;
            g_game_state_exit = main_menu_exit;
            break;

        case GS_GAME:
            g_game_state_init = game_init;
            g_game_state_update = game_update;
            g_game_state_exit = game_exit;
            break;

        case GS_GAME_TIMEMODE:
            g_game_state_init = game_time_mode_init;
            g_game_state_update = game_time_mode_update;
            g_game_state_exit = game_time_mode_exit;
            break;

        case GS_SHOP:
            g_game_state_init = shop_init;
            g_game_state_update = shop_update;
            g_game_state_exit = shop_exit;
            break;

        case GS_CREDITS:
            g_game_state_init = credit_screen_init;
            g_game_state_update = credit_screen_update;
            g_game_state_exit = credit_screen_exit;
            break;

        case GS_QUIT:
            // Will handle the actual quitting in main
            g_game_state_init = nullptr;
            g_game_state_update = nullptr;
            g_game_state_exit = nullptr;
            break;

        case GS_TUTORIAL:
            g_game_state_init = tutorial_init;
            g_game_state_update = tutorial_update;
            g_game_state_exit = tutorial_exit;
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