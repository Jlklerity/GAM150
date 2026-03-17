/*!
@file           UpgradeSystem.h
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#pragma once

#include "Enemy.h" 

/*
@namespace      UpgradeConfig
@brief          Centralized tuning values for the upgrade card screen.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace UpgradeConfig
{
    // Card layout
    extern const float UPGRADE_CARD_W;                                                                                      // card width in world pixels at base scale
    extern const float UPGRADE_CARD_H;                                                                                      // card height in world pixels at base scale
    extern const float UPGRADE_CARD_SPACING;                                                                                // horizontal gap between adjacent cards
    extern const float UPGRADE_CARD_CENTER_Y;                                                                               // vertical center of all three cards in world space

    // Hover scale
    extern const float UPGRADE_CARD_HOVER_SCALE;                                                                            // multiplier applied to card size when the cursor is over it (e.g. 1.3)

    // Upgrade potency
    extern const float UPGRADE_HP_BONUS;                                                                                    // extra max HP granted by the health card
    extern const float UPGRADE_DURATION_BONUS;                                                                              // multiplier stacked per duration card pick (e.g. 1.3 = 30% longer per pick)
    extern const float UPGRADE_SPAWN_REDUCTION;                                                                             // seconds subtracted from power-up spawn interval per pick

    // Overlay
    extern const float UPGRADE_OVERLAY_ALPHA;                                                                               // opacity of the dim overlay behind the cards (0 = invisible, 1 = solid)
} // UpgradeConfig namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      UpgradeSystem
@brief          Manages the post-boss upgrade card screen: loading assets, animating cards,
                handling hover and selection, applying the chosen upgrade, and cleaning up.

Call Upgrade_Init once at game start and Upgrade_Free once at game exit. When the boss dies,
call Upgrade_TriggerScreen. While Upgrade_IsScreenOpen returns true, skip normal gameplay
updates and call Upgrade_Update then Upgrade_Draw each frame instead.

PowerUpSystem should query Upgrade_GetPowerUpDurationMultiplier and
Upgrade_GetPowerUpSpawnIntervalReduction to respect accumulated upgrade bonuses.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace UpgradeSystem
{
    /*
    @function       Upgrade_Init
    @brief          Loads all three card textures, builds meshes, and resets all state and bonuses.

    Loads Assets/MaxHP.png, Assets/IncreasePowerDuration.png, and Assets/IncreasePowerSpawn.png,
    each as a single full-image texture (uv_w = 1.0, uv_h = 1.0). Builds a shared sprite quad
    mesh and a unit rectangle mesh for the dim overlay. Calls Upgrade_Free first so repeated calls
    on restart never leak resources.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Init();

    /*
    @function       Upgrade_Free
    @brief          Unloads all three card textures and frees the card and overlay meshes.

    Null-checked before every free so safe to call even if Init never ran. Pointers are set to
    nullptr after freeing to prevent double-free on repeated calls.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Free();

    /*
    @function       Upgrade_TriggerScreen
    @brief          Opens the upgrade card screen after the boss is defeated.

    Marks the screen as open, resets the animation timer and frame, and shuffles which upgrade
    type occupies which display slot. Does nothing if the screen is already open.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_TriggerScreen();

    /*
    @function       Upgrade_IsScreenOpen
    @brief          Returns true while the upgrade card screen is visible and blocking gameplay.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Upgrade_IsScreenOpen();

    /*
    @function       Upgrade_Update
    @params         health (Data::Health&), dt (float)
    @brief          Updates hover state and applies a clicked upgrade.

    Reads the cursor position, determines which card is hovered, and on left-click applies the
    upgrade and closes the screen. The health reference is only written for the max HP card.
    Does nothing when the screen is closed.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Update(Data::Health& health, float dt);

    /*
    @function       Upgrade_Draw
    @brief          Renders the dim overlay and the three upgrade cards.

    Each card uses its own texture as a single full image. The hovered card is drawn at
    UPGRADE_CARD_HOVER_SCALE times its normal size and on top of the other cards.
    Does nothing when the screen is not open.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Draw();

    /*
    @function       Upgrade_GetPowerUpDurationMultiplier
    @brief          Returns the cumulative power-up duration multiplier from picked duration cards.
    Returns 1.0 if the player has never picked the duration card.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Upgrade_GetPowerUpDurationMultiplier();

    /*
    @function       Upgrade_GetPowerUpSpawnIntervalReduction
    @brief          Returns the total seconds to subtract from the power-up spawn interval.
    Returns 0.0 if the player has never picked the spawn-rate card.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Upgrade_GetPowerUpSpawnIntervalReduction();

    /*
    @function       Upgrade_Reset
    @brief          Resets all accumulated upgrade bonuses to defaults. Called by Upgrade_Init.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Reset();

} // UpgradeSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/