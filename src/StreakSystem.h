#pragma once

namespace StreakSystem
{
    void  Init();
    void  Update(float dt);
    void  Draw();
    void  Exit();

    void  OnWordCorrect();                    // call when enemy is killed (no position)
    void  OnWordCorrectAt(float x, float y);  // call with kill position for floating +1
    void  OnStreakReset();                    // call on wrong word or enemy landing

    int   GetStreak();
    float GetMultiplier();
    void  OnHealthLost();
}