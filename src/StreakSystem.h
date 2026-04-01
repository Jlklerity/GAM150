#pragma once

namespace StreakSystem
{
    // resets streak state and loads flame assets, call once in game_init()
    void  init();

    // advances bob animation, pop scale decay and floating popup, param is dt in seconds
    void  update(float dt);

    // draws the flame sprite, streak counter and floating popup
    void  draw();

    // frees flame mesh and texture, call in game_exit()
    void  cleanup();

    // increments streak and triggers pop animation, call on kill without a known position
    void  on_word_correct();

    // increments streak and spawns a floating popup at the given world position
    void  on_word_correct_at(float x, float y);

    // resets streak to zero, call when an enemy lands or the player types incorrectly
    void  on_streak_reset();

    // returns the current streak count
    int   get_streak();

    // returns the score multiplier for the current tier (1.0x to 5.0x)
    float get_multiplier();

    // resets the streak when the player loses health
    void  on_health_lost();
}