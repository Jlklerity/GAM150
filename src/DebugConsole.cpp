/*!
@file           DebugConsole.cpp
@brief          Minecraft-style debug console overlay for the typing game.

All console state, input handling, command parsing, and rendering lives here.
game.cpp only calls Init / Update / Render / Exit / IsOpen.
*/

#pragma warning(disable: 26426)
#define NOMINMAX
#include "DebugConsole.h"
#include "AEEngine.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include "Enemy.h"
#include "PowerUp.h"
#include "Weather.h"
#include "Boss.h"

#include <string>
#include <vector>
#include <sstream>

// Internal state
namespace
{
    //  Game state references (set in init) 
    bool* s_pEnemyAreaVisible = nullptr;
    bool* s_pAutoSpawn = nullptr;
    bool* s_pDifficultyVisible = nullptr;
    Data::Health* s_pHealth = nullptr;
    std::vector<Data::Enemy>* s_pEnemyPool = nullptr;
    std::deque<int>* s_pFreeSlots = nullptr;
    Data::EnemyArea* s_pEnemyArea = nullptr;
    Data::WordBank* s_pWordBank = nullptr;

    //  Console open/close 
    bool s_isOpen = false;

    //  Input buffer 
    static char     s_inputBuf[64]{};
    static int      s_inputLen = 0;

    //  History 
    struct HistoryEntry
    {
        std::string text;
        bool        isError; // red if true, white if false
    };

    static std::vector<HistoryEntry> s_history;
    static const int MAX_HISTORY = 64;   // max stored entries
    static const int VISIBLE_LINES = 8;    // lines shown at once
    static int s_scrollOffset = 0;    // 0 = bottom (most recent)

    //  Cursor blink 
    static float s_cursorTimer = 0.0f;
    static bool  s_cursorVisible = true;

    //  Rendering 
    static AEGfxVertexList* s_pPanelMesh = nullptr; // reusable quad for background panels

    //  Layout constants (world space, 1600x900 ) 
    static const float SCREEN_W = 1600.0f;
    static const float SCREEN_H = 900.0f;
    static const float HALF_W = SCREEN_W * 0.5f;
    static const float HALF_H = SCREEN_H * 0.5f;

    static const float INPUT_BAR_H = 40.0f;
    static const float INPUT_BAR_Y = -HALF_H + INPUT_BAR_H * 0.5f + 4.0f; // near bottom
    static const float HISTORY_LINE_H = 36.0f;
    static const float HISTORY_PANEL_H = VISIBLE_LINES * HISTORY_LINE_H + 8.0f;
    static const float HISTORY_PANEL_Y = INPUT_BAR_Y + INPUT_BAR_H * 0.5f + HISTORY_PANEL_H * 0.5f + 2.0f;

    //  check if key held (independent from typing.cpp) 
    static bool s_prevKeys[256]{};

    bool KeyTriggered(int key)
    {
        if (key < 0 || key >= 256) return false;
        bool curr = (AEInputCheckCurr(static_cast<u8>(key)) != 0);
        bool triggered = curr && !s_prevKeys[key];
        s_prevKeys[key] = curr;
        return triggered;
    }

    // History helpers
    void PushHistory(const std::string& text, bool isError = false)
    {
        s_history.push_back({ text, isError });
        if ((int)s_history.size() > MAX_HISTORY)
            s_history.erase(s_history.begin()); // drop oldest
        s_scrollOffset = 0; // snap back to bottom on new entry
    }

    // Command parsing helpers
    std::vector<std::string> Tokenise(const std::string& line)
    {
        std::vector<std::string> tokens;
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);
        return tokens;
    }

    // Case-insensitive compare
    bool IEquals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
        return true;
    }

    // Command execution
    void ExecuteCommand(const std::string& raw)
    {
        // Echo the typed command into history (white)
        PushHistory("> " + raw, false);

        // Strip leading '/' 
        std::string line = raw;
        if (!line.empty() && line[0] == '/') line = line.substr(1);

        auto tokens = Tokenise(line);
        if (tokens.empty()) return;

        const std::string& cmd = tokens[0];

        // /spawn [3-8]   -- spawn an enemy of the given word length
        if (IEquals(cmd, "spawn"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /spawn [3-8] | friendly | shield | boss", true);
                return;
            }

            // /spawn friendly
            if (IEquals(tokens[1], "friendly"))
            {
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();
                EnemySystem::Enemy_SpawnFriendlyFromPool(*s_pEnemyPool, *s_pFreeSlots, *s_pEnemyArea, *s_pWordBank, 3, max_len, 1.15f);
                PushHistory("Spawned friendly enemy.", false);
                return;
            }

            // /spawn shield
            if (IEquals(tokens[1], "shield"))
            {
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();
                EnemySystem::Enemy_SpawnShieldedFromPool(*s_pEnemyPool, *s_pFreeSlots, *s_pEnemyArea, *s_pWordBank, 3, max_len, 1.05f);
                PushHistory("Spawned shielded enemy.", false);
                return;
            }

            // /spawn boss
            if (IEquals(tokens[1], "boss"))
            {
                BossSystem::Boss_ForceSpawn();
                PushHistory("Spawned boss.", false);
                return;
            }

            // /spawn [3-8]
            int len = 0;
            try
            {
                len = std::stoi(tokens[1]);
            }
            catch (...)
            {
                PushHistory("Error: unknown spawn type. Use [3-8], friendly, shield, boss.", true);
                return;
            }
            if (len < 3 || len > 8)
            {
                PushHistory("Error: word length must be between 3 and 8.", true);
                return;
            }
            EnemySystem::Enemy_SpawnFromPool(*s_pEnemyPool, *s_pFreeSlots, *s_pEnemyArea, *s_pWordBank, len);
            PushHistory("Spawned enemy with word length " + std::to_string(len) + ".", false);
            return;                                                         // must return or falls through to unknown command
        }

        // /kill boss - instantly kill the active boss
        if (IEquals(cmd, "kill"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /kill boss", true);
                return;
            }

            if (IEquals(tokens[1], "boss"))
            {
                BossSystem::Boss_ForceKill(*s_pEnemyPool, *s_pFreeSlots);
                PushHistory("Boss force-killed.", false);
            }
            else
            {
                PushHistory("Unknown kill target. Try: /kill boss", true);
            }
            return;
        }

        // /health add    -- add 10 hp
        if (IEquals(cmd, "health"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /health add", true);
                return;
            }
            if (IEquals(tokens[1], "add"))
            {
                HealthSystem::Health_Add(*s_pHealth, 10.0f);
                PushHistory("Added 10 HP. Current: " + std::to_string((int)s_pHealth->health_current), false);
            }
            else if (IEquals(tokens[1], "die")) {
                s_pHealth->health_current = 0;
                PushHistory("Player killed.", false);
            }
            else
            {
                PushHistory("Unknown health subcommand. Try: /health add", true);
            }
            return;
        }


        // /difficulty up | down | show
        if (IEquals(cmd, "difficulty"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /difficulty [up|down|show]", true);
                return;
            }
            if (IEquals(tokens[1], "up"))
            {
                DifficultySystem::Difficulty_Add(1.0f);
                PushHistory("Difficulty increased to " +
                    std::to_string((int)DifficultySystem::Difficulty_Get()) + ".", false);
            }
            else if (IEquals(tokens[1], "down"))
            {
                DifficultySystem::Difficulty_Add(-1.0f);
                PushHistory("Difficulty decreased to " +
                    std::to_string((int)DifficultySystem::Difficulty_Get()) + ".", false);
            }
            else if (IEquals(tokens[1], "show"))
            {
                *s_pDifficultyVisible = !*s_pDifficultyVisible;
                PushHistory(std::string("Difficulty HUD: ") + (*s_pDifficultyVisible ? "ON" : "OFF"), false);
            }
            else
            {
                PushHistory("Unknown difficulty subcommand. Try: up | down | show", true);
            }
            return;
        }

        // /debugbox - toggle enemy area debug box
        if (IEquals(cmd, "debugbox"))
        {
            *s_pEnemyAreaVisible = !*s_pEnemyAreaVisible;
            PushHistory(std::string("Debug box: ") + (*s_pEnemyAreaVisible ? "ON" : "OFF"), false);
            return;
        }

        // /autospawn - toggle automatic enemy spawning
        if (IEquals(cmd, "autospawn"))
        {
            *s_pAutoSpawn = !*s_pAutoSpawn;
            PushHistory(std::string("Auto-spawn: ") + (*s_pAutoSpawn ? "ON" : "OFF"), false);
            return;
        }

        // /help - list all commands
        if (IEquals(cmd, "help"))
        {
            PushHistory("/spawn [3-8]         - spawn enemy by word length", false);
            PushHistory("/spawn friendly      - spawn a friendly enemy", false);
            PushHistory("/spawn shield        - spawn a shielded enemy", false);
            PushHistory("/spawn boss          - force-spawn the boss", false);
            PushHistory("/kill boss           - instantly kill the active boss", false);
            PushHistory("/health add          - add 10 HP", false);
            PushHistory("/health die          - kills player", false);
            PushHistory("/difficulty up       - increase difficulty by 1", false);
            PushHistory("/difficulty down     - decrease difficulty by 1", false);
            PushHistory("/difficulty show     - toggle difficulty HUD", false);
            PushHistory("/debugbox            - toggle enemy area box", false);
            PushHistory("/autospawn           - toggle auto enemy spawning", false);
            PushHistory("/powerup lightning   - force-spawn Lightning drop", false); // azfar code start
            PushHistory("/powerup shield      - force-spawn Shield drop", false);
            PushHistory("/powerup slow        - force-spawn TimeSlow drop", false);
            PushHistory("/weather snow        - trigger blizzard event", false);
            PushHistory("/weather thunder     - trigger thunderstorm event", false);
            PushHistory("/weather rainbow     - trigger rainbow event", false);
            PushHistory("/weather ghost       - trigger ghost weather event", false);

            return;
        }

        // /powerup lightning | shield | slow | force spawn a power up drop
        if (IEquals(cmd, "powerup"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /powerup [lightning|shield|slow]", true);
                return;
            }
            if (IEquals(tokens[1], "lightning"))
            {
                PowerUpSystem::DebugSpawn(PowerUpSystem::PowerType::Lightning);
                PushHistory("Spawned Lightning power-up.", false);
            }
            else if (IEquals(tokens[1], "shield"))
            {
                PowerUpSystem::DebugSpawn(PowerUpSystem::PowerType::Shield);
                PushHistory("Spawned Shield power-up.", false);
            }
            else if (IEquals(tokens[1], "slow"))
            {
                PowerUpSystem::DebugSpawn(PowerUpSystem::PowerType::TimeSlow);
                PushHistory("Spawned TimeSlow power-up.", false);
            }
            else
            {
                PushHistory("Unknown power-up type. Use: lightning | shield | slow", true);
            }
            return;
        }

        // /weather snow | thunder | rainbow | ghost  -- force-trigger a weather event
        if (IEquals(cmd, "weather"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /weather [snow|thunder|rainbow|ghost]", true);
                return;
            }
            if (IEquals(tokens[1], "snow"))
            {
                Weather_ForceEvent(0, *s_pEnemyPool);
                PushHistory("Triggered snow blizzard.", false);
            }
            else if (IEquals(tokens[1], "thunder"))
            {
                Weather_ForceEvent(1, *s_pEnemyPool);
                PushHistory("Triggered thunderstorm.", false);
            }
            else if (IEquals(tokens[1], "rainbow"))
            {
                Weather_ForceEvent(2, *s_pEnemyPool);
                PushHistory("Triggered rainbow.", false);
            }
            else if (IEquals(tokens[1], "ghost"))
            {
                Weather_ForceEvent(3, *s_pEnemyPool);
                PushHistory("Triggered ghost weather.", false);
            }
            else
            {
                PushHistory("Unknown weather type. Use: snow | thunder | rainbow | ghost", true);
            }
            return;
        }


        // Unknown command
        PushHistory("Unknown command: /" + cmd + "  (try /help)", true);
    }



    // Rendering helpers
    // ---------------------------------------------------------------------------

    // Draw a solid color quad in world space
    void DrawPanel(float x, float y, float w, float h, float r, float g, float b, float a)
    {
        if (!s_pPanelMesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, a);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        Mesh_Renderer(s_pPanelMesh, x, y, w, h, 0.0f);
    }

    // Print text; nx/ny are already in NDC [-1,1]
    void DrawText(const char* text, float nx, float ny, float scale,
        float r, float g, float b)
    {
        if (Graphics::font_id < 0) return;
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::font_id, text, nx, ny, scale, r, g, b, 1.0f);
    }

    // Convert world Y to NDC
    float WorldYtoNDC(float wy) { return wy / HALF_H; }
    float WorldXtoNDC(float wx) { return wx / HALF_W; }

} // anonymous namespace

// public stuff
namespace DebugConsole
{
    void Init(
        bool& is_enemy_area_box_visible,
        bool& is_random_spawning_enabled,
        bool& is_difficulty_debug_visible,
        Data::Health& health,
        std::vector<Data::Enemy>& enemy_pool,
        std::deque<int>& free_slot_queue,
        Data::EnemyArea& enemy_area,
        Data::WordBank& word_bank)
    {
        s_pEnemyAreaVisible = &is_enemy_area_box_visible;
        s_pAutoSpawn = &is_random_spawning_enabled;
        s_pDifficultyVisible = &is_difficulty_debug_visible;
        s_pHealth = &health;
        s_pEnemyPool = &enemy_pool;
        s_pFreeSlots = &free_slot_queue;
        s_pEnemyArea = &enemy_area;
        s_pWordBank = &word_bank;

        s_isOpen = false;
        s_inputLen = 0;
        s_inputBuf[0] = '\0';
        s_scrollOffset = 0;
        s_history.clear();

        s_cursorTimer = 0.0f;
        s_cursorVisible = true;

        for (int i = 0; i < 256; ++i) s_prevKeys[i] = false;

        if (s_pPanelMesh) { AEGfxMeshFree(s_pPanelMesh); s_pPanelMesh = nullptr; }
        s_pPanelMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    }

    void Update()
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();

        //  Toggle console open / 
        // AEVK_SLASH = 0xBF 
        if (KeyTriggered(AEVK_SLASH) && !s_isOpen) //if console not open + / presed
        {
            s_isOpen = true;
            s_inputLen = 0;
            s_inputBuf[0] = '\0';
            s_cursorTimer = 0.0f;
            s_cursorVisible = true;
            s_inputBuf[s_inputLen++] = '/';                                 // pre-fill / 
            s_inputBuf[s_inputLen] = '\0';
            *s_pDifficultyVisible = true;                                   // auto-show difficulty HUD when console opens
            return;                                                         // consume the /
        }

        if (!s_isOpen) return; // if console not open returb

        //  Cursor blink 
        s_cursorTimer += dt;
        if (s_cursorTimer >= 0.5f)
        {
            s_cursorTimer = 0.0f;
            s_cursorVisible = !s_cursorVisible;
        }

        //  Scroll history with mouse wheel 
        {
            s32 wheel = 0;
            AEInputMouseWheelDelta(&wheel);
            int maxScroll = ((int)s_history.size() - VISIBLE_LINES > 0) ? (int)s_history.size() - VISIBLE_LINES : 0;
            if (wheel > 0) s_scrollOffset = (s_scrollOffset + 1 < maxScroll) ? s_scrollOffset + 1 : maxScroll; // scroll up (older)
            if (wheel < 0) s_scrollOffset = (s_scrollOffset - 1 > 0) ? s_scrollOffset - 1 : 0;                 // scroll down (newer)
        }

        //  Escape closes without submitting 
        if (KeyTriggered(AEVK_ESCAPE))
        {
            s_isOpen = false;
            s_inputLen = 0;
            s_inputBuf[0] = '\0';
            *s_pDifficultyVisible = false;                                  // hide difficulty HUD when console closes
            return;
        }

        //  Enter submits 
        if (KeyTriggered(AEVK_RETURN))
        {
            std::string cmd(s_inputBuf);
            while (!cmd.empty() && cmd.back() == ' ') cmd.pop_back();      // trim trailing spaces

            if (cmd.empty() || cmd == "/")
            {
                s_isOpen = false;                                           // empty enter = close console
                s_inputLen = 0;
                s_inputBuf[0] = '\0';
                s_scrollOffset = 0;
                *s_pDifficultyVisible = false;                              // hide difficulty HUD when console closes
                return;
            }

            ExecuteCommand(cmd);                                            // non-empty = run command and stay open
            s_inputLen = 1;                                                 // reset buffer back to just '/'
            s_inputBuf[0] = '/';
            s_inputBuf[1] = '\0';
            s_scrollOffset = 0;                                             // snap history to bottom after command
            return;
        }

        //  Backspace 
        if (KeyTriggered(AEVK_BACK))
        {
            // Don't let player delete the leading '/'
            if (s_inputLen > 1)
            {
                s_inputBuf[--s_inputLen] = '\0';
            }
            return;
        }

        //  Letter keys A-Z 
        if (s_inputLen < (int)sizeof(s_inputBuf) - 2) // leave room for null
        {
            for (int k = AEVK_A; k <= AEVK_Z; ++k)
            {
                if (KeyTriggered(k))
                {
                    char c = (char)('a' + (k - AEVK_A)); // lowercase
                    s_inputBuf[s_inputLen++] = c;
                    s_inputBuf[s_inputLen] = '\0';
                }
            }

            //  Digit keys 0-9 
            // Alpha Engine AEVK_0 - AEVK_9 = 0x30 - 0x39
            for (int k = AEVK_0; k <= AEVK_9; ++k)
            {
                if (KeyTriggered(k))
                {
                    char c = (char)('0' + (k - AEVK_0));
                    s_inputBuf[s_inputLen++] = c;
                    s_inputBuf[s_inputLen] = '\0';
                }
            }

            //  Space 
            if (KeyTriggered(AEVK_SPACE))
            {
                s_inputBuf[s_inputLen++] = ' ';
                s_inputBuf[s_inputLen] = '\0';
            }
        }
    }

    void Render()
    {
        // History panel is always visible if there are entries (even when console is closed)
        // Input bar only visible when open
        bool hasHistory = !s_history.empty();

        //  Draw history panel background 
        if (s_isOpen && hasHistory)
        {
            float panelY = s_isOpen ? HISTORY_PANEL_Y : -HALF_H + INPUT_BAR_H + HISTORY_PANEL_H * 0.5f + 4.0f;

            DrawPanel(0.0f, panelY, SCREEN_W, HISTORY_PANEL_H, 0.0f, 0.0f, 0.0f, 0.65f); // history panel background

            //  Draw history lines 
            // s_scrollOffset=0 means show the newest VISIBLE_LINES entries
            int totalEntries = (int)s_history.size();
            // The bottom of the visible window in history index terms:
            int bottomIdx = totalEntries - 1 - s_scrollOffset;         // newest visible
            int topIdx = bottomIdx - (VISIBLE_LINES - 1);           // oldest visible

            float textScale = 0.38f;
            float lineStartY = panelY + (HISTORY_PANEL_H * 0.5f) - 4.0f; // top of panel

            // loop through each visible line slot in the history panel
            for (int line = 0; line < VISIBLE_LINES; ++line)
            {
                // calculate which history entry maps to this row
                int entryIdx = topIdx + line;

                // skip if out of bounds (<8)
                if (entryIdx < 0 || entryIdx >= totalEntries) continue;

                // grab the history entry at this index
                const HistoryEntry& entry = s_history[(size_t)entryIdx];

                // calculate world Y position for this row, gg downward each line
                float worldY = lineStartY - (line * HISTORY_LINE_H) - HISTORY_LINE_H * 0.5f;

                // convert to NDC for print
                float ny = WorldYtoNDC(worldY);


                float nx = WorldXtoNDC(-HALF_W + 12.0f);

                // pick red for errors, white for normal output
                float r = entry.isError ? 1.0f : 1.0f;
                float g = entry.isError ? 0.2f : 1.0f;
                float b = entry.isError ? 0.2f : 1.0f;

                // draw the text at the calculated position and color
                DrawText(entry.text.c_str(), nx, ny, textScale, r, g, b);
            }

            //  Scroll indicators 
            int maxScroll = (totalEntries - VISIBLE_LINES > 0) ? totalEntries - VISIBLE_LINES : 0;
            if (s_scrollOffset < maxScroll) // can scroll up (older entries above)
            {
                float ny = WorldYtoNDC(panelY + HISTORY_PANEL_H * 0.5f - 2.0f);
                DrawText("^", WorldXtoNDC(HALF_W - 30.0f), ny, 0.35f, 0.8f, 0.8f, 0.8f);
            }
            if (s_scrollOffset > 0) // can scroll down (newer entries below)
            {
                float ny = WorldYtoNDC(panelY - HISTORY_PANEL_H * 0.5f + 2.0f);
                DrawText("v", WorldXtoNDC(HALF_W - 30.0f), ny, 0.35f, 0.8f, 0.8f, 0.8f);
            }
        }

        //  Draw input bar (only when open) 
        if (!s_isOpen) return;

        // Background bar
        DrawPanel(0.0f, INPUT_BAR_Y, SCREEN_W, INPUT_BAR_H, 0.08f, 0.08f, 0.08f, 0.92f);

        // Thin line above the bar
        DrawPanel(0.0f, INPUT_BAR_Y + INPUT_BAR_H * 0.5f + 1.0f, SCREEN_W, 2.0f, 0.4f, 0.8f, 0.4f, 1.0f);

        // Input text + cursor
        if (Graphics::font_id >= 0)
        {
            std::string display = std::string(s_inputBuf);
            if (s_cursorVisible) display += "_";

            float textScale = 0.42f;
            float nx = WorldXtoNDC(-HALF_W + 12.0f);
            float ny = WorldYtoNDC(INPUT_BAR_Y - 6.0f);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, display.c_str(), nx, ny, textScale, 0.6f, 1.0f, 0.6f, 1.0f);
        }
    }

    void Exit()
    {
        if (s_pPanelMesh)
        {
            AEGfxMeshFree(s_pPanelMesh);
            s_pPanelMesh = nullptr;
        }
        s_history.clear();
        s_isOpen = false;
        s_inputLen = 0;
    }

    bool IsOpen()
    {
        return s_isOpen;
    }

} // namespace DebugConsole