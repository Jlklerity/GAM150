#pragma once
/*!
@file       LaserBeam.h
@brief      Player laser beam — animated travel, runtime colour & shape.

A beam fires from the player sprite to a killed enemy. Each beam:
  1. TRAVELS from origin to target over TRAVEL_TIME seconds.
  2. FADES out over FADE_TIME seconds after arriving.

Shape modes
-----------
  LASER_SHAPE_SOLID   (0)  — single stretched rectangle (default)
  LASER_SHAPE_DASHED  (1)  — rectangular segments with gaps
  LASER_SHAPE_ZIGZAG  (2)  — alternating angled segments
  LASER_SHAPE_DOTS    (3)  — small squares stamped at intervals
  LASER_SHAPE_TEXTURE (4)  — stretches Assets/laser_custom.png across the beam
                             (falls back to SOLID if file not found)

Integration in game.cpp
-----------------------
  Game_Init()   -> LaserBeam_Init();                          // loads custom tex if present
                   LaserBeam_SetOrigin(spriteX + 75.f, spriteY);
                   LaserBeam_SetOuterColor(gLaserOuterR, gLaserOuterG, gLaserOuterB);
                   LaserBeam_SetCoreColor (gLaserCoreR,  gLaserCoreG,  gLaserCoreB);
                   LaserBeam_SetShape(gLaserShape);
  Game_Update() -> LaserBeam_Update(dt);
  render loop   -> LaserBeam_Draw();
  Game_Exit()   -> LaserBeam_Free();

Integration in Typing.cpp
--------------------------
  LaserBeam_Fire(target.enemy_position_x, target.enemy_position_y);
*/

// ----------------------------------------------------------------
// Shape constants  (pass to LaserBeam_SetShape)
// ----------------------------------------------------------------
static const int LASER_SHAPE_SOLID = 0;
static const int LASER_SHAPE_DASHED = 1;
static const int LASER_SHAPE_ZIGZAG = 2;
static const int LASER_SHAPE_DOTS = 3;
static const int LASER_SHAPE_TEXTURE = 4;
static const int LASER_SHAPE_COUNT = 5;

static const char* const LASER_SHAPE_NAMES[5] = {
    "SOLID", "DASHED", "ZIGZAG", "DOTS", "CUSTOM"
};

// ----------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------
void LaserBeam_Init();   // also auto-tries to load Assets/laser_custom.png
void LaserBeam_Free();

// ----------------------------------------------------------------
// Configuration  (safe to call at any time)
// ----------------------------------------------------------------
void LaserBeam_SetOrigin(float x, float y);
void LaserBeam_SetOuterColor(float r, float g, float b);
void LaserBeam_SetCoreColor(float r, float g, float b);
void LaserBeam_GetOuterColor(float& r, float& g, float& b);
void LaserBeam_GetCoreColor(float& r, float& g, float& b);

/** Sets the active shape mode.  Use LASER_SHAPE_* constants. */
void LaserBeam_SetShape(int shape);
int  LaserBeam_GetShape();

// ----------------------------------------------------------------
// Per-frame
// ----------------------------------------------------------------
void LaserBeam_Update(float dt);
void LaserBeam_Draw();

// ----------------------------------------------------------------
// Event
// ----------------------------------------------------------------
void LaserBeam_Fire(float toX, float toY);