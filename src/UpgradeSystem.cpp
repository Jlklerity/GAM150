/*!
@file           UpgradeSystem.cpp
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#include "pch.h"
#include "UpgradeSystem.h"


/*
@namespace      UpgradeConfig
@brief          Tuning constants for the upgrade card screen.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace UpgradeConfig
{
    const float UPGRADE_CARD_W = 300.0f;                                                                                    // card width in world pixels at base scale
    const float UPGRADE_CARD_H = 400.0f;                                                                                    // card height in world pixels at base scale
    const float UPGRADE_CARD_SPACING = 60.0f;                                                                               // horizontal gap between adjacent cards
    const float UPGRADE_CARD_CENTER_Y = 80.0f;                                                                              // vertical center of all three cards in world space

    const float UPGRADE_CARD_HOVER_SCALE = 1.3f;                                                                            // scale factor applied to the card when hovered

    const float UPGRADE_HP_BONUS = 10.0f;                                                                                   // extra max HP granted by the health card (+10)
    const float UPGRADE_DURATION_BONUS = 1.3f;                                                                              // duration multiplier stacked per pick
    const float UPGRADE_SPAWN_REDUCTION = 2.0f;                                                                             // seconds subtracted from power-up spawn interval per pick

    const float UPGRADE_OVERLAY_ALPHA = 0.65f;                                                                              // opacity of the dim overlay behind the cards
} // UpgradeConfig namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      Anonymous
@brief          Private state used only inside UpgradeSystem.cpp.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace
{
    /*
    @enum           UpgradeType
    @brief          Identifies which gameplay bonus a card grants when selected.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    enum class UpgradeType
    {
        MAX_HP = 0,                                                                                                         // increase the player maximum health
        POWERUP_DURATION = 1,                                                                                               // extend the active duration of power-ups
        POWERUP_SPAWN_RATE = 2,                                                                                             // reduce the time between power-up spawns
        COUNT = 3
    };

    static const int  CARD_COUNT = 3;                                                                                       // always exactly three cards on screen

    // Each card has its own single-image texture (full UV: uv_w = 1.0, uv_h = 1.0)
    // Index matches the UpgradeType enum value:
    //   0 = MAX_HP           -> Assets/MaxHP.png
    //   1 = POWERUP_DURATION -> Assets/IncreasePowerDuration.png
    //   2 = POWERUP_SPAWN    -> Assets/IncreasePowerSpawn.png

    static const char* const CARD_TEXTURE_PATHS[CARD_COUNT] =
    {
        "Assets/MaxHP.png",                                                                                                 // UpgradeType::MAX_HP (index 0)
        "Assets/IncreasePowerDuration.png",                                                                                 // UpgradeType::POWERUP_DURATION (index 1)
        "Assets/IncreasePowerSpawn.png"                                                                                     // UpgradeType::POWERUP_SPAWN_RATE (index 2)
    };

    // Assets: one texture per upgrade type, shared quad mesh, shared overlay mesh
    AEGfxTexture* s_card_textures[CARD_COUNT] = { nullptr, nullptr, nullptr };                                                // one texture per card type
    AEGfxVertexList* s_card_mesh = nullptr;                                                                                   // quad covering one animation frame, shared by all cards
    AEGfxVertexList* s_overlay_mesh = nullptr;                                                                                // unit rect for the dim overlay

    // Screen state
    bool             s_upgrade_screen_open = false;
    bool             s_card_is_hovered[CARD_COUNT] = { false, false, false };

    // Display order: s_slot_upgrade_type[i] is the UpgradeType shown in slot i (shuffled on trigger)
    UpgradeType      s_slot_upgrade_type[CARD_COUNT] =
    {
        UpgradeType::MAX_HP,
        UpgradeType::POWERUP_DURATION,
        UpgradeType::POWERUP_SPAWN_RATE
    };

    // Accumulated upgrade bonuses (persist across boss fights within one session)
    float            s_accumulated_duration_multiplier = 1.0f;
    float            s_accumulated_spawn_reduction = 0.0f;

    /*
    @function       Slot_CenterX
    @params         slot_index (int)
    @brief          Returns the world X center for a given card slot.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Slot_CenterX(int slot_index)
    {
        float total_w = (float)CARD_COUNT * UpgradeConfig::UPGRADE_CARD_W
            + (float)(CARD_COUNT - 1) * UpgradeConfig::UPGRADE_CARD_SPACING;                                                // total width of the three-card row
        float left_edge = -(total_w * 0.5f) + (UpgradeConfig::UPGRADE_CARD_W * 0.5f);                                       // center x of the leftmost card
        return left_edge + (float)slot_index * (UpgradeConfig::UPGRADE_CARD_W + UpgradeConfig::UPGRADE_CARD_SPACING);       // offset to the requested slot
    }

    /*
    @function       IsMouseOverCard
    @params         slot_index (int), world_mouse_x (float), world_mouse_y (float)
    @brief          Returns true if the mouse is inside the card's AABB using the base (unscaled) size.

    The hit-test always uses the base card size so the clickable area does not change when the
    card is scaled up by the hover effect.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool IsMouseOverCard(int slot_index, float world_mouse_x, float world_mouse_y)
    {
        float cx = Slot_CenterX(slot_index);
        float cy = UpgradeConfig::UPGRADE_CARD_CENTER_Y;
        float hw = UpgradeConfig::UPGRADE_CARD_W * 0.5f;
        float hh = UpgradeConfig::UPGRADE_CARD_H * 0.5f;

        return (world_mouse_x >= cx - hw && world_mouse_x <= cx + hw &&
            world_mouse_y >= cy - hh && world_mouse_y <= cy + hh);
    }

    /*
    @function       ApplyUpgrade
    @params         type (UpgradeType), health (Data::Health&)
    @brief          Immediately applies the effect of the chosen upgrade.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void ApplyUpgrade(UpgradeType type, Data::Health& health)
    {
        switch (type)
        {
        case UpgradeType::MAX_HP:
        {
            health.health_max += UpgradeConfig::UPGRADE_HP_BONUS;                                                           // raise the health ceiling
            health.health_current += UpgradeConfig::UPGRADE_HP_BONUS;                                                       // grant the bonus HP immediately
            if (health.health_current > health.health_max)                                                                  // guard against floating-point overshoot
                health.health_current = health.health_max;
            break;
        }
        case UpgradeType::POWERUP_DURATION:
        {
            s_accumulated_duration_multiplier *= UpgradeConfig::UPGRADE_DURATION_BONUS;                                       // stack the multiplier
            break;
        }
        case UpgradeType::POWERUP_SPAWN_RATE:
        {
            s_accumulated_spawn_reduction += UpgradeConfig::UPGRADE_SPAWN_REDUCTION;                                          // accumulate the reduction
            break;
        }
        default: break;
        }
    }

} // anonymous namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      UpgradeSystem
@brief          Public lifecycle functions for the upgrade card screen.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace UpgradeSystem
{
    /*
    @function       Upgrade_Init
    @brief          Loads all three card textures and builds meshes. Resets all state and bonuses.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Init()
    {
        Upgrade_Free();                                                                                                     // free any previous resources before reloading
        Upgrade_Reset();                                                                                                    // reset bonuses so they do not carry over between sessions

        // Load one 2-column, 1-row sprite sheet per upgrade type
        for (int i = 0; i < CARD_COUNT; ++i)
        {
            s_card_textures[i] = AEGfxTextureLoad(CARD_TEXTURE_PATHS[i]);                                                     // load this card's sprite sheet
            AE_ASSERT_MESG(s_card_textures[i], CARD_TEXTURE_PATHS[i]);                                                        // crash with the path name if the file was not found
        }

        // Build the shared card quad: full UV (uv_w = 1.0, uv_h = 1.0) for single-image textures
        s_card_mesh = Graphics::Mesh_BuildSpriteQuad(1.0f, 1.0f, 0xFFFFFFFF);

        // Build the full-screen overlay rectangle (scaled at draw time)
        s_overlay_mesh = Graphics::Mesh_BuildUnitRect(0xFFFFFFFF);

        s_upgrade_screen_open = false;
        for (int i = 0; i < CARD_COUNT; ++i)
            s_card_is_hovered[i] = false;
    }

    /*
    @function       Upgrade_Free
    @brief          Unloads all three card textures and frees meshes. Safe to call if Init never ran.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Free()
    {
        for (int i = 0; i < CARD_COUNT; ++i)
        {
            if (s_card_textures[i])
            {
                AEGfxTextureUnload(s_card_textures[i]);                                                                       // unload this card's texture from VRAM
                s_card_textures[i] = nullptr;                                                                                 // clear pointer to prevent double-free
            }
        }

        if (s_card_mesh)
        {
            AEGfxMeshFree(s_card_mesh);                                                                                       // free the shared card vertex list
            s_card_mesh = nullptr;
        }

        if (s_overlay_mesh)
        {
            AEGfxMeshFree(s_overlay_mesh);                                                                                    // free the overlay vertex list
            s_overlay_mesh = nullptr;
        }
    }

    /*
    @function       Upgrade_TriggerScreen
    @brief          Opens the upgrade screen, resets animation, and shuffles card slots.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_TriggerScreen()
    {
        if (s_upgrade_screen_open) return;                                                                                    // already open, do nothing

        // Fisher-Yates shuffle of the three upgrade types across display slots
        s_slot_upgrade_type[0] = UpgradeType::MAX_HP;
        s_slot_upgrade_type[1] = UpgradeType::POWERUP_DURATION;
        s_slot_upgrade_type[2] = UpgradeType::POWERUP_SPAWN_RATE;

        for (int i = CARD_COUNT - 1; i > 0; --i)
        {
            int j = Utility::Utility_RandInt(0, i);
            UpgradeType tmp = s_slot_upgrade_type[i];
            s_slot_upgrade_type[i] = s_slot_upgrade_type[j];
            s_slot_upgrade_type[j] = tmp;
        }

        for (int i = 0; i < CARD_COUNT; ++i)
            s_card_is_hovered[i] = false;                                                                                     // clear stale hover state from the previous frame

        s_upgrade_screen_open = true;
    }

    /*
    @function       Upgrade_IsScreenOpen
    @brief          Returns true while the upgrade screen is blocking normal gameplay.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Upgrade_IsScreenOpen()
    {
        return s_upgrade_screen_open;
    }

    /*
    @function       Upgrade_Update
    @params         health (Data::Health&)
    @brief          Advances animation, updates hover state, and applies a clicked upgrade.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Update(Data::Health& health)
    {
        if (!s_upgrade_screen_open) return;

        // Convert cursor pixel position to world space
        // Window is 1600x900 with world origin at center: wx = mx - 800, wy = 450 - my
        s32 mx = 0, my = 0;
        AEInputGetCursorPosition(&mx, &my);
        float wx = (float)mx - 800.0f;
        float wy = 450.0f - (float)my;

        // Update hover flag for each slot
        for (int i = 0; i < CARD_COUNT; ++i)
            s_card_is_hovered[i] = IsMouseOverCard(i, wx, wy);

        // Check for a left-click to select a card
        if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            for (int i = 0; i < CARD_COUNT; ++i)
            {
                if (s_card_is_hovered[i])
                {
                    ApplyUpgrade(s_slot_upgrade_type[i], health);                                                             // apply the chosen upgrade immediately
                    s_upgrade_screen_open = false;                                                                            // close the screen
                    return;
                }
            }
        }
    }

    /*
    @function       Upgrade_Draw
    @brief          Draws the dim overlay and three cards. Hovered card draws at 1.3x.

    Each slot binds its own texture based on which UpgradeType is assigned to it. Textures are
    single full images with no animation. Idle cards are drawn first; the hovered card is drawn
    last so it appears on top when enlarged.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Draw()
    {
        if (!s_upgrade_screen_open) return;
        if (!s_card_mesh || !s_overlay_mesh) return;                                                                            // mesh missing, skip to avoid null dereference

        // --- Dim overlay ---
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(UpgradeConfig::UPGRADE_OVERLAY_ALPHA);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);

        Graphics::Draw_DrawMeshAt(s_overlay_mesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

        // --- Cards (2-pass: idle first, hovered on top) ---
        for (int pass = 0; pass < 2; ++pass)
        {
            for (int i = 0; i < CARD_COUNT; ++i)
            {
                bool hovered = s_card_is_hovered[i];

                if (pass == 0 && hovered) continue;                                                                         // skip hovered card in idle pass
                if (pass == 1 && !hovered) continue;                                                                        // skip idle cards in hovered pass

                // Look up which upgrade type is in this slot to select the correct texture
                int type_idx = static_cast<int>(s_slot_upgrade_type[i]);
                if (type_idx < 0 || type_idx >= CARD_COUNT) continue;                                                       // guard against invalid enum cast
                if (!s_card_textures[type_idx]) continue;                                                                     // skip if this card's texture failed to load

                Graphics::Graphics_SetSpriteState();

                // Bind this card's own texture at UV origin (full single image)
                AEGfxTextureSet(s_card_textures[type_idx], 0.0f, 0.0f);

                float cx = Slot_CenterX(i);
                float cy = UpgradeConfig::UPGRADE_CARD_CENTER_Y;
                float scale = hovered ? UpgradeConfig::UPGRADE_CARD_HOVER_SCALE : 1.0f;                                     // 1.3x when hovered, 1.0x otherwise
                float draw_w = UpgradeConfig::UPGRADE_CARD_W * scale;
                float draw_h = UpgradeConfig::UPGRADE_CARD_H * scale;

                Graphics::Draw_DrawMeshAt(s_card_mesh, cx, cy, draw_w, draw_h, 0.0f);
            }
        }

    }

    float Upgrade_GetPowerUpDurationMultiplier() { return s_accumulated_duration_multiplier; }
    float Upgrade_GetPowerUpSpawnIntervalReduction() { return s_accumulated_spawn_reduction; }

    /*
    @function       Upgrade_Reset
    @brief          Clears all accumulated bonuses and resets screen/animation state.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Upgrade_Reset()
    {
        s_accumulated_duration_multiplier = 1.0f;
        s_accumulated_spawn_reduction = 0.0f;
        s_upgrade_screen_open = false;
        for (int i = 0; i < CARD_COUNT; ++i)
            s_card_is_hovered[i] = false;
    }

} // UpgradeSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/