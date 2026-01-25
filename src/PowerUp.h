#pragma once
#include <vector>

/**
 * @file PowerUp.h
 * @author Azfar
 * @brief Power-up system interface for the typing game.
 *
 * This module handles:
 * - A bottom-left power-up button with a 30 second cooldown.
 * - Random falling power-up drops that can be clicked to activate effects.
 * - Gameplay effects (Lightning, Shield, TimeSlow).
 *
 * Integration (typical):
 * - Call PowerUpSystem::Init() once in Game_Init().
 * - Call PowerUpSystem::Update(dt, enemy_pool) every frame.
 * - Convert mouse pixel coords to world coords, then call PowerUpSystem::OnMouseClick().
 * - Call PowerUpSystem::Draw() after drawing the world (draw last so UI appears on top).
 * - Call PowerUpSystem::Free() once in Game_Exit().
 */

namespace Data { struct Enemy; }

namespace PowerUpSystem
{
    /**
     * @enum PowerType
     * @brief Enumeration of available power-up effects.
     */
    enum class PowerType
    {
        Lightning = 0, /**< Instantly kills all active enemies on screen. */
        Shield,        /**< Makes player immune to damage for a duration (requires health integration later). */
        TimeSlow       /**< Slows the game for a duration by returning a time scale < 1.0f. */
    };

    /**
     * @brief Initializes power-up system resources.
     *
     * Creates the placeholder quad mesh for rendering drops/buttons (color mode) and
     * loads the font used to render labels.
     *
     * Call once at game initialization.
     */
    void Init();

    /**
     * @brief Frees power-up system resources.
     *
     * Releases mesh and font resources created in Init().
     * Call once at game shutdown.
     */
    void Free();

    /**
     * @brief Updates power-up logic (cooldowns, timers, spawning, drop movement).
     *
     * @param dt Delta time in seconds (if global time slow is applied in game.cpp, pass scaled dt).
     * @param enemy_pool Reference to the enemy pool. Used when activating certain effects.
     */
    void Update(float dt, std::vector<Data::Enemy>& enemy_pool);

    /**
     * @brief Handles a mouse click to activate the UI button or a falling drop.
     *
     * @param mouseX Mouse X in world coordinates (origin at center, range approx [-800, +800]).
     * @param mouseY Mouse Y in world coordinates (origin at center, range approx [-450, +450]).
     * @param enemy_pool Reference to the enemy pool used by activation logic.
     */
    void OnMouseClick(float mouseX, float mouseY, std::vector<Data::Enemy>& enemy_pool);

    /**
     * @brief Draws the power-up UI button and the falling drop (if active).
     *
     * Draw this AFTER drawing enemies and other world elements so it appears on top (UI layer).
     */
    void Draw();

    /**
     * @brief Returns whether Shield is currently active.
     *
     * Note: Actual "immunity to damage" requires health system integration.
     * For now, this is an effect-state query for future use.
     *
     * @return true if shield timer > 0, false otherwise.
     */
    bool IsShieldActive();

    /**
     * @brief Returns a time scale multiplier for TimeSlow.
     *
     * When TimeSlow is active, this returns a value < 1.0f (e.g. 0.4f).
     * Integration:
     * - Multiply your game dt by this value to slow everything.
     *
     * @return 1.0f if no slow is active; otherwise a smaller time scale.
     */
    float GetTimeScale();

    /**
     * @brief Sets the random drop spawn interval for testing/balancing.
     *
     * Drops will spawn after a randomized delay between [minSeconds, maxSeconds].
     *
     * @param minSeconds Minimum time between spawns (clamped to >= 0.2s).
     * @param maxSeconds Maximum time between spawns (clamped to >= minSeconds).
     */
    void SetDropSpawnInterval(float minSeconds, float maxSeconds);
}
