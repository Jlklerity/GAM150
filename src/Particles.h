#pragma once
#include "AEEngine.h"

// creates the shared quad mesh used by all particles and loads sprite sheet textures, call once in game_init()
void particles_init();

// frees the shared quad mesh and sprite textures, call once in game_exit()
void particles_free();

// advances all active particles, lines, sprite effects, shake and flash each frame,
// param dt is delta time in seconds - should be time-scaled if TimeSlow is active
void particles_update(float dt);

// renders all active particles, impact lines, sprite effects and the flash overlay,
// call after drawing enemies and before drawing UI overlays
void particles_draw();

// spawns the full damage combo at the given world position: dust particles, impact lines,
// screen flash and camera shake - call when an enemy lands and deals damage
void particles_spawn_damage_impact(float x, float y);

// spawns a coloured burst at the given world position when the player kills an enemy,
// colour is picked by word_length (3-8) to match the enemy sprite colour coding,
// no shake or flash since this is a reward event
void particles_spawn_kill_impact(float x, float y, int word_length);

// starts a screen shake with the given duration in seconds and max pixel intensity,
// overwrites any ongoing shake - safe to call at any time
void particles_trigger_shake(float duration, float intensity);

// plays the shield break animation centred on the enemy, call when a shield is broken
void particles_spawn_shield_break(float x, float y);

// plays the friendly heal animation centred on the enemy, call when a friendly enemy is typed
void particles_spawn_friendly_heal(float x, float y);