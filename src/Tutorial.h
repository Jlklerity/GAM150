#pragma once

// initialises all tutorial subsystems from a clean slate 
void tutorial_init();

// runs the tutorial game loop each frame
void tutorial_update();

// shuts down every subsystem initialised by Tutorial_Init
void tutorial_exit();