#pragma once

namespace StreakSystem
{
    // resets streak state and loads flame assets, call once in Game_Init()
    void  Init();

    // advances bob animation, pop scale decay and floating popup, param is dt in seconds
    void  Update(float dt);

    // draws the flame sprite, streak counter and floating popup
    void  Draw();

    // frees flame mesh and texture, call in Game_Exit()
    void  Exit();

    // increments streak and triggers pop animation, call on kill without a known position
    void  OnWordCorrect();

    // increments streak and spawns a floating popup at the given world position
    void  OnWordCorrectAt(float x, float y);

    // resets streak to zero, call when an enemy lands or the player types incorrectly
    void  OnStreakReset();

    // returns the current streak count
    int   GetStreak();

    // returns the score multiplier for the current tier (1.0x to 5.0x)
    float GetMultiplier();

    // resets the streak when the player loses health
    void  OnHealthLost();
}