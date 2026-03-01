/*!
@file           Particles.h
@brief          Self-contained particle system for impact effects.

Two effects are provided:
  - Particles_SpawnDamageImpact : full combo (dust + flash + screen shake + impact lines)
                                  called when an enemy lands and deals damage.
  - Particles_SpawnKillImpact   : color burst splat (no shake)
                                  called when a typed/killed enemy hits the floor.

Integration in game.cpp (nothing else needs to change):
    Game_Init()   -> Particles_Init()
    Game_Update() -> Particles_Update(dt)   [inside gameplay block]
    Game_Exit()   -> Particles_Free()
    render block  -> Particles_Draw()       [after enemies, before UI]

    Inside the  was_active && !e.enemy_is_active  loop:
        if (e.enemy_did_damage == false before update, i.e. natural land)
            Particles_SpawnDamageImpact(e.enemy_position_x, e.enemy_position_y);
        else (typed kill landing)
            Particles_SpawnKillImpact(e.enemy_position_x, e.enemy_position_y, e.enemy_word_length);
*/

#pragma once
#include "AEEngine.h"

/*
@function   Particles_Init
@brief      Creates the single shared quad mesh used by all particles.
            Call once in Game_Init().
*/
void Particles_Init();

/*
@function   Particles_Free
@brief      Frees the shared quad mesh.
            Call once in Game_Exit().
*/
void Particles_Free();

/*
@function   Particles_Update
@brief      Advances all active particles, impact lines, shake and flash timers.
            Call every frame inside the gameplay (non-paused) block.
@param      dt  Delta time in seconds (already time-scaled from game.cpp).
*/
void Particles_Update(float dt);

/*
@function   Particles_Draw
@brief      Renders all active particles, impact lines and flash overlay.
            Call after drawing enemies, before drawing UI overlays.
*/
void Particles_Draw();

/*
@function   Particles_SpawnDamageImpact
@brief      Spawns the full combo effect at the given world position:
              - Grey/white dust particles with gravity
              - Short radiating impact lines
              - Brief white screen flash
              - Screen shake proportional to landing speed
@param      x   World X of the landing position.
@param      y   World Y of the landing position.
*/
void Particles_SpawnDamageImpact(float x, float y);

/*
@function   Particles_SpawnKillImpact
@brief      Spawns a colored burst splat at the given world position.
            Color is chosen by word_length (matches enemy sprite color coding).
            No screen shake - this is a positive/reward event.
@param      x               World X of the landing position.
@param      y               World Y of the landing position.
@param      word_length     Enemy word length (3-8) used to pick burst color.
*/
void Particles_SpawnKillImpact(float x, float y, int word_length);
