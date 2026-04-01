#pragma once

namespace SysSettings
{
    void init();
    bool display_settings(); // Returns false when the player clicks the "Close" button
    void cleanup();
}