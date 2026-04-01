#pragma once
/*!
@file           Settings.h
@author         Jeremy Lim
@brief          Handles the UI and logic for the in-game audio settings menu, allowing players to adjust various volume levels.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

namespace SysSettings
{
    void init();
    bool display_settings(); // Returns false when the player clicks the "Close" button
    void cleanup();
}