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

@integration (game.cpp):
            Weather_Init()              -- call once inside Game_Init()
            Weather_Update(dt, pool)    -- call inside the !isPaused && !isDead block
            Weather_DrawBackground()    -- call BEFORE enemies are drawn (rainbow behind enemies)
            Weather_Draw()              -- call inside the render block, after world / powerup draws
            Weather_Exit()              -- call once inside Game_Exit()
            Weather_IsPlayerFrozen()    -- } gate BOTH on
            Weather_IsPlayerStunned()   -- } Typing_UpdateAndApplyToEnemies()

@author     (your name)
*/

namespace Data { struct Enemy; }

void Weather_Init();
void Weather_Update(float dt, std::vector<Data::Enemy>& enemy_pool);
void Weather_DrawBackground(); // Call BEFORE enemies: draws rainbow behind them
void Weather_Draw();           // Call AFTER enemies: draws snow/thunder overlays + HUD
void Weather_Exit();
bool Weather_IsPlayerFrozen();   // true during full snow freeze
bool Weather_IsPlayerStunned();  // true during brief thunder strike stun

/**
 * @brief Debug helper: immediately triggers a weather event of the given type.
 *        Any currently active event is cancelled first.
 * @param type       0 = Snow/Blizzard, 1 = Thunderstorm, 2 = Rainbow
 * @param enemy_pool reference to enemy vector (needed to restore lunge speed)
 */
void Weather_ForceEvent(int type, std::vector<Data::Enemy>& enemy_pool);