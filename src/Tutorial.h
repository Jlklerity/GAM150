#pragma once
/*
@file           Tutorial.h
@author         YOU
@brief          Self-contained tutorial game state with 8 guided stages.

                The player sprite acts as the teacher with a speech bubble.
                Each stage gates gameplay until the objective is met,
                then a NEXT button appears to advance.

Stages:
    1. Basic typing       - Kill a normal enemy
    2. Kill before landing - Don't let it reach the ground
    3. Friendly NPC       - Free the princess
    4. Shielded enemy     - Break shield then kill
    5. Streak             - Reach a 3 word streak
    6. Power up           - Activate a dropped power up
    7. Weather            - Escape a blizzard freeze
    8. Boss warning       - Informational, then PLAY

Integration:
    GameStateManager.h  - Add GS_TUTORIAL to enum
    GameStateManager.cpp - Add case GS_TUTORIAL
    MainMenu.cpp        - Add tutorial button that sets gGameStateNext = GS_TUTORIAL
*/

void Tutorial_Init();
void Tutorial_Update();
void Tutorial_Exit();
