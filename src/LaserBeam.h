#pragma once
/*!
@file       LaserBeam.h
@brief      Player laser beam - animated sprite sheet travel + explosion on impact.

Each beam fires from the player origin to a killed enemy:
  1. TRAVELS over TRAVEL_TIME seconds, animating the laser spritesheet along the beam.
  2. On arrival, spawns an EXPLOSION animation at the enemy position.

Spritesheets expected in Assets/
  laser.png      - horizontal strip, LASER_FRAME_COUNT frames, 1 row
  explosion.png  - horizontal strip, EXPLOSION_FRAME_COUNT frames, 1 row

Integration in game.cpp
-----------------------
  game_init()   -> laser_beam_init();
                   laser_beam_set_origin(spriteX + 75.f, spriteY);
  game_update() -> laser_beam_update(dt);
  render loop   -> laser_beam_draw();
  game_exit()   -> laser_beam_free();

Integration in Typing.cpp
--------------------------
  laser_beam_fire(target.enemy_position_x, target.enemy_position_y);
*/

// ----------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------
void laser_beam_init();
void laser_beam_free();

// ----------------------------------------------------------------
// Configuration  (safe to call at any time)
// ----------------------------------------------------------------
void laser_beam_set_origin(float x, float y);

// ----------------------------------------------------------------
// Per-frame
// ----------------------------------------------------------------
void laser_beam_update(float dt);
void laser_beam_draw();

// ----------------------------------------------------------------
// Event
// ----------------------------------------------------------------
void laser_beam_fire(float to_x, float to_y);