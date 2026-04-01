#pragma once
#include <vector>

/*
@file       Weather.h
@brief      Weather system interface for the typing game.

            Implements random weather events that visually and mechanically
            affect gameplay. Currently implemented:

            - SNOWY:       Ambient snowflakes always fall during gameplay. After a
                           random interval a blizzard strikes, freezing the player.
                           Spam SPACEBAR to accumulate warmth and break free.

            - THUNDERSTORM: Dark overlay rolls in, then lightning bolts strike
                            periodically. Each bolt: 1s "BRACE!" warning then
                            white flash + input stun (0.5s) + random enemy lunges
                            at 3x speed. Storm lasts ~20s then clears.

            - RAINBOW:     Lasts 15 seconds. A rainbow sprite is drawn across the
                           screen (behind enemies). Power-ups spawn every 3 seconds
                           for the duration of the event.

            - GHOST:       5 ghosts fly across the screen in random directions.
                           Each ghost displays a digit (5-9). The player must type
                           the matching digit to destroy it. If any ghost survives
                           for 10 seconds, typing is locked for 3 seconds.
                           A centred "LOCKED" indicator with a countdown timer is
                           shown while the lock is active.

@integration (game.cpp):
            weather_init()              -- call once inside game_init()
            weather_update(dt, pool)    -- call inside the !isPaused && !isDead block
            weather_draw_background()    -- call BEFORE enemies are drawn (rainbow behind enemies)
            weather_draw()              -- call inside the render block, after world / powerup draws
            weather_exit()              -- call once inside game_exit()
            weather_is_player_frozen()    -- } gate BOTH on
            weather_is_player_stunned()   -- } gate BOTH on
            weather_is_typing_locked()    -- } typing_update_and_apply_to_enemies()

@author     (your name)
*/

namespace Data { struct Enemy; }

void weather_init();
void weather_update(float dt, std::vector<Data::Enemy>& enemy_pool);
void weather_draw_background(); // Call BEFORE enemies: draws rainbow behind them
void weather_draw();           // Call AFTER enemies: draws snow/thunder overlays + HUD
void weather_exit();
bool weather_is_player_frozen();   // true during full snow freeze
bool weather_is_player_stunned();  // true during brief thunder strike stun
bool weather_is_typing_locked();   // true during ghost-weather typing lock

/**
 * @brief Debug helper: immediately triggers a weather event of the given type.
 *        Any currently active event is cancelled first.
 * @param type       0 = Snow/Blizzard, 1 = Thunderstorm, 2 = Rainbow, 3 = Ghost
 * @param enemy_pool reference to enemy vector (needed to restore lunge speed)
 */
void weather_force_event(int type, std::vector<Data::Enemy>& enemy_pool);