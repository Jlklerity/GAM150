#pragma once

// initialises all tutorial subsystems from a clean slate 
void Tutorial_Init();

// runs the tutorial game loop each frame
void Tutorial_Update();

// shuts down every subsystem initialised by Tutorial_Init
void Tutorial_Exit();