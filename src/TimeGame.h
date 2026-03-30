#pragma once

// initialises the timed game mode, call when entering this game state
void GameTimeMode_Init();

// runs the timed game loop each frame including countdown, spawning, rendering and death handling
void GameTimeMode_Update();

// frees all assets and systems, call when leaving this game state
void GameTimeMode_Exit();