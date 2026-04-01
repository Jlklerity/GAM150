#pragma once
#include <vector>
#include "Enemy.h"   // EnemySystem::Enemy_Killed, Data::Enemy, Data::EnemyArea, EnemyConfig

/**
 * @file PowerUp.h
 * @author Azfar
 * @brief Power-up system interface for the typing game.
 *
 * This module handles:
 * - Up to 3 simultaneous power-up drops falling from the sky.
 * - Each drop occupies a numbered slot (1, 2, 3).
 * - Player presses the matching number key to activate the drop in that slot.
 * - Drops that reach the floor (enemy landing line) disappear without activating.
 * - Gameplay effects: Lightning, Shield, TimeSlow.
 *
 * Integration (game.cpp):
 * - Call PowerUpSystem::init() once in game_init().
 * - Call PowerUpSystem::update(dt, enemy_pool, enemy_area) every frame.
 * - Call PowerUpSystem::draw() in the render block.
 * - Call PowerUpSystem::free() once in game_exit().
 * - on_mouse_click() is a no-op stub kept for link compatibility.
 *
 * Assets expected (place in Assets/):
 *   Assets/powerup_lightning.png
 *   Assets/powerup_shield.png
 *   Assets/powerup_slow.png
 */

namespace Data { struct Enemy; }
namespace Data { struct EnemyArea; }

namespace PowerUpSystem
{
    /**
     * @enum PowerType
     * @brief Enumeration of available power-up effects.
     */
    enum class PowerType
    {
        LIGHTNING = 0, /**< Instantly kills all active enemies on screen. */
        SHIELD,        /**< Makes player immune to damage for a duration. */
        TIME_SLOW      /**< Slows the game for a duration by returning a time scale < 1.0f. */
    };

    /** @brief Initialises power-up resources. Call once in game_init(). */
    void init();

    /** @brief Releases resources. Call once in game_exit(). */
    void free();

    /**
     * @brief Updates power-up logic (timers, spawning, falling, key input).
     * @param dt           Delta time in seconds.
     * @param enemy_pool   Enemy pool used by Lightning activation.
     * @param enemy_area   Enemy play area - landing y used as the drop floor.
     * @param health       Player health
     */
    void update(float dt, std::vector<Data::Enemy>& enemy_pool,
        const Data::EnemyArea& enemy_area, Data::Health& health);

    /**
     * @brief No-op stub retained for link compatibility with game.cpp.
     *        Mouse clicks no longer activate power-ups.
     */
    void on_mouse_click(float mouse_x, float mouse_y, std::vector<Data::Enemy>& enemy_pool);

    /** @brief Draws all active drops (sprite + slot number + effect timers). */
    void draw();

    /** @brief Returns true while the Shield effect is active. */
    bool is_shield_active();

    /**
     * @brief Returns a time scale multiplier for TimeSlow.
     * @return 1.0f normally; SLOW_SCALE while slow is active.
     */
    float get_time_scale();

    /**
     * @brief Sets the random drop spawn interval.
     * @param min_seconds  Minimum seconds between spawns (clamped >= 0.2).
     * @param max_seconds  Maximum seconds between spawns (clamped >= min_seconds).
     */
    void set_drop_spawn_interval(float min_seconds, float max_seconds);

    /**
     * @brief Debug helper: force-spawns a drop of the given type into a free slot.
     *
     * Spawns at a fixed Y above the screen centre so it is immediately visible.
     * Uses the last known enemy area bounds; safe to call any time after init().
     * No-ops silently if all 3 slots are already occupied.
     *
     * @param type  The power-up type to force-spawn.
     */
    void debug_spawn(PowerType type);
}