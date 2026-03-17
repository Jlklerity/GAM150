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
  Game_Init()   -> LaserBeam_Init();
                   LaserBeam_SetOrigin(spriteX + 75.f, spriteY);
  Game_Update() -> LaserBeam_Update(dt);
  render loop   -> LaserBeam_Draw();
  Game_Exit()   -> LaserBeam_Free();

Integration in Typing.cpp
--------------------------
  LaserBeam_Fire(target.enemy_position_x, target.enemy_position_y);
*/

// ----------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------
void LaserBeam_Init();
void LaserBeam_Free();

// ----------------------------------------------------------------
// Configuration  (safe to call at any time)
// ----------------------------------------------------------------
void LaserBeam_SetOrigin(float x, float y);

// ----------------------------------------------------------------
// Per-frame
// ----------------------------------------------------------------
void LaserBeam_Update(float dt);
void LaserBeam_Draw();

// ----------------------------------------------------------------
// Event
// ----------------------------------------------------------------
void LaserBeam_Fire(float toX, float toY);