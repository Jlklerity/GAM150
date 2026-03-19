#pragma once

namespace DayCycle
{
    void Init();
    void Update(float dt);
    void Draw();         // call BEFORE enemies, AFTER background clear
    void Exit();

    void TriggerTransition(); // call when boss spawns

    bool IsNight();           // true once transition is fully complete
    float GetNightAlpha();    // 0.0 = full day, 1.0 = full night (smooth)
    void DrawSky();
    void DrawBodies();
    void TriggerDayTransition(); // call when boss is defeated
    bool IsDay();                // true once fully back to day
}
