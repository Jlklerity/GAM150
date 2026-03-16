#pragma once


namespace StreakSystem
{
    void  Init();
    void  Update(float dt);
    void  Draw();
    void  Exit();

    void  OnWordCorrect();   // call when enemy is killed
    void  OnStreakReset();   // call on wrong word or enemy landing

    int   GetStreak();
    float GetMultiplier();
    void OnHealthLost(); // call when player takes damage
}
