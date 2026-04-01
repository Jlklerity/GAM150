#pragma once

// initialises the timed game mode, call when entering this game state
void game_time_mode_init();

// runs the timed game loop each frame including countdown, spawning, rendering and death handling
void game_time_mode_update();

// frees all assets and systems, call when leaving this game state
void game_time_mode_exit();