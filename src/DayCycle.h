#pragma once

namespace DayCycle
{
    void init();
    void update(float dt);
    void draw();         // call BEFORE enemies, AFTER background clear
    void cleanup();

    void trigger_transition(); // call when boss spawns

    bool is_night();           // true once transition is fully complete
    float get_night_alpha();    // 0.0 = full day, 1.0 = full night (smooth)
    void draw_sky();
    void draw_bodies();
    void trigger_day_transition(); // call when boss is defeated
    bool is_day();                // true once fully back to day
}