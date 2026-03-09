/*!
@file           DebugConsole.cpp
@brief          Minecraft-style debug console overlay for the typing game.

All console state, input handling, command parsing, and rendering lives here.
game.cpp only calls Init / Update / Render / Exit / IsOpen.
*/

#define NOMINMAX  // min max taiji
#include "DebugConsole.h"
#include "AEEngine.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include "Enemy.h"
#include "PowerUp.h"
#include "Weather.h"

#include <string>
#include <vector>
#include <sstream>

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
namespace
{
    // ---- Game state references (set in Init) ----
    bool* s_pEnemyAreaVisible = nullptr;
    bool* s_pAutoSpawn = nullptr;
    bool* s_pDifficultyVisible = nullptr;
    Data::Health* s_pHealth = nullptr;
    std::vector<Data::Enemy>* s_pEnemyPool = nullptr;
    std::deque<int>* s_pFreeSlots = nullptr;
    Data::EnemyArea* s_pEnemyArea = nullptr;
    Data::WordBank* s_pWordBank = nullptr;

    // ---- Console open/close ----
    bool s_isOpen = false;

    // ---- Input buffer ----
    static char     s_inputBuf[64]{};
    static int      s_inputLen = 0;

    // ---- History ----
    struct HistoryEntry
    {
        std::string text;
        bool        isError; // red if true, white if false
    };

    static std::vector<HistoryEntry> s_history;
    static const int MAX_HISTORY = 64;   // max stored entries
    static const int VISIBLE_LINES = 8;    // lines shown at once
    static int s_scrollOffset = 0;    // 0 = bottom (most recent)

    // ---- Cursor blink ----
    static float s_cursorTimer = 0.0f;
    static bool  s_cursorVisible = true;

    // ---- Rendering ----
    static AEGfxVertexList* s_pPanelMesh = nullptr; // reusable quad for background panels

    // ---- Layout constants (world space, 1600x900 window) ----
    static const float SCREEN_W = 1600.0f;
    static const float SCREEN_H = 900.0f;
    static const float HALF_W = SCREEN_W * 0.5f;
    static const float HALF_H = SCREEN_H * 0.5f;

    static const float INPUT_BAR_H = 40.0f;
    static const float INPUT_BAR_Y = -HALF_H + INPUT_BAR_H * 0.5f + 4.0f; // near bottom
    static const float HISTORY_LINE_H = 36.0f;
    static const float HISTORY_PANEL_H = VISIBLE_LINES * HISTORY_LINE_H + 8.0f;
    static const float HISTORY_PANEL_Y = INPUT_BAR_Y + INPUT_BAR_H * 0.5f + HISTORY_PANEL_H * 0.5f + 2.0f;

    // ---- Key-repeat state (independent from Typing.cpp) ----
    static bool s_prevKeys[256]{};

    bool KeyTriggered(int key)
    {
        if (key < 0 || key >= 256) return false;                            // rej if oob
        bool curr = (AEInputCheckCurr(static_cast<u8>(key)) != 0);         // true if key is held down this frame
        bool triggered = curr && !s_prevKeys[key];                          // only true on the first frame the key is pressed
        s_prevKeys[key] = curr;                                             // save state for next frame comparison
        return triggered;
    }

    // ---------------------------------------------------------------------------
    // History helpers
    // ---------------------------------------------------------------------------
    void PushHistory(const std::string& text, bool isError = false)
    {
        s_history.push_back({ text, isError });                             // append new entry to history
        if ((int)s_history.size() > MAX_HISTORY)
            s_history.erase(s_history.begin());                             // drop oldest entry if over limit
        s_scrollOffset = 0;                                                 // snap back to bottom on new entry
    }

    // ---------------------------------------------------------------------------
    // Command parsing helpers
    // ---------------------------------------------------------------------------
    std::vector<std::string> Tokenise(const std::string& line)
    {
        std::vector<std::string> tokens;
        std::istringstream ss(line);                                        // wrap line in a stream so we can extract word by word
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);                           // push each whitespace-separated token into the vector
        return tokens;
    }

    // case-insensitive compare, used so commands work regardless of caps
    bool IEquals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size()) return false;                            // diff lengths = cant be equal
        for (size_t i = 0; i < a.size(); ++i)
            if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false; // compare lowercase versions char by char
        return true;
    }

    // ---------------------------------------------------------------------------
    // Command execution
    // ---------------------------------------------------------------------------
    void ExecuteCommand(const std::string& raw)
    {
        PushHistory("> " + raw, false);                                     // echo the typed command into history in white

        std::string line = raw;
        if (!line.empty() && line[0] == '/') line = line.substr(1);        // strip leading '/' if present before tokenising

        auto tokens = Tokenise(line);                                       // split into command + arguments
        if (tokens.empty()) return;                                         // nth to do if empty after stripping

        const std::string& cmd = tokens[0];                                 // first token is the command name

        // ------------------------------------------------------------------
        // /spawn [3-8]   -- spawn an enemy of the given word length
        // ------------------------------------------------------------------
        if (IEquals(cmd, "spawn"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /spawn [3-8] | friendly | shield", true);
                return;
            }

            // /spawn friendly
            if (IEquals(tokens[1], "friendly"))
            {
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();                                          // get current max word length based on difficulty
                EnemySystem::Enemy_SpawnFriendlyFromPool(*s_pEnemyPool, *s_pFreeSlots, *s_pEnemyArea, *s_pWordBank, 3, max_len, 1.15f);
                PushHistory("Spawned friendly enemy.", false);
                return;
            }

            // /spawn shield
            if (IEquals(tokens[1], "shield"))
            {
                int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();                                          // get current max word length based on difficulty
                EnemySystem::Enemy_SpawnShieldedFromPool(*s_pEnemyPool, *s_pFreeSlots, *s_pEnemyArea, *s_pWordBank, 3, max_len, 1.05f);
                PushHistory("Spawned shielded enemy.", false);
                return;
            }

            // /spawn [3-8]
            int len = 0;
            try
            {
                len = std::stoi(tokens[1]);                                 // try to parse the length arg as int
            }
            catch (...)
            {
                PushHistory("Error: unknown spawn type. Use [3-8], friendly, or shield.", true); // not a number or known keyword
                return;
            }
            if (len < 3 || len > 8)
            {
                PushHistory("Error: word length must be between 3 and 8.", true);               // valid int but out of supported range
                return;
            }
        }

        // ------------------------------------------------------------------
        // /health add    -- add 10 hp
        // ------------------------------------------------------------------
        if (IEquals(cmd, "health"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /health add", true);
                return;
            }
            if (IEquals(tokens[1], "add"))
            {
                HealthSystem::Health_Add(*s_pHealth, 10.0f);                                    // add 10 hp via the health system
                PushHistory("Added 10 HP. Current: " + std::to_string((int)s_pHealth->health_current), false); // confirm with new value
            }
            else
            {
                PushHistory("Unknown health subcommand. Try: /health add", true);
            }
            return;
        }

        // ------------------------------------------------------------------
        // /difficulty up | down | show
        // ------------------------------------------------------------------
        if (IEquals(cmd, "difficulty"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /difficulty [up|down|show]", true);
                return;
            }
            if (IEquals(tokens[1], "up"))
            {
                DifficultySystem::Difficulty_Add(1.0f);                     // increase difficulty by 1 level
                PushHistory("Difficulty increased to " +
                    std::to_string((int)DifficultySystem::Difficulty_Get()) + ".", false); // confirm new level
            }
            else if (IEquals(tokens[1], "down"))
            {
                DifficultySystem::Difficulty_Add(-1.0f);                    // decrease difficulty by 1 level
                PushHistory("Difficulty decreased to " +
                    std::to_string((int)DifficultySystem::Difficulty_Get()) + ".", false); // confirm new level
            }
            else if (IEquals(tokens[1], "show"))
            {
                *s_pDifficultyVisible = !*s_pDifficultyVisible;             // toggle the difficulty HUD overlay
                PushHistory(std::string("Difficulty HUD: ") + (*s_pDifficultyVisible ? "ON" : "OFF"), false);
            }
            else
            {
                PushHistory("Unknown difficulty subcommand. Try: up | down | show", true);
            }
            return;
        }

        // ------------------------------------------------------------------
        // /debugbox      -- toggle enemy area debug box
        // ------------------------------------------------------------------
        if (IEquals(cmd, "debugbox"))
        {
            *s_pEnemyAreaVisible = !*s_pEnemyAreaVisible;                   // toggle the enemy area bounding box overlay
            PushHistory(std::string("Debug box: ") + (*s_pEnemyAreaVisible ? "ON" : "OFF"), false);
            return;
        }

        // ------------------------------------------------------------------
        // /autospawn     -- toggle automatic enemy spawning
        // ------------------------------------------------------------------
        if (IEquals(cmd, "autospawn"))
        {
            *s_pAutoSpawn = !*s_pAutoSpawn;                                 // toggle random enemy spawning on/off
            PushHistory(std::string("Auto-spawn: ") + (*s_pAutoSpawn ? "ON" : "OFF"), false);
            return;
        }

        // ------------------------------------------------------------------
        // /help          -- list all commands
        // ------------------------------------------------------------------
        if (IEquals(cmd, "help"))
        {
            PushHistory("/spawn [3-8]         - spawn enemy by word length", false);
            PushHistory("/spawn friendly      - spawn a friendly enemy", false);
            PushHistory("/spawn shield        - spawn a shielded enemy", false);
            PushHistory("/health add          - add 10 HP", false);
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
            return;
        }

        // ------------------------------------------------------------------
        // /powerup lightning | shield | slow | force spawn a power up drop
        // ------------------------------------------------------------------
        if (IEquals(cmd, "powerup"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /powerup [lightning|shield|slow]", true);
                return;
            }
            if (IEquals(tokens[1], "lightning"))
            {
                PowerUpSystem::DebugSpawn(PowerUpSystem::PowerType::Lightning); // force spawn a lightning drop
                PushHistory("Spawned Lightning power-up.", false);
            }
            else if (IEquals(tokens[1], "shield"))
            {
                PowerUpSystem::DebugSpawn(PowerUpSystem::PowerType::Shield);    // force spawn a shield drop
                PushHistory("Spawned Shield power-up.", false);
            }
            else if (IEquals(tokens[1], "slow"))
            {
                PowerUpSystem::DebugSpawn(PowerUpSystem::PowerType::TimeSlow);  // force spawn a time slow drop
                PushHistory("Spawned TimeSlow power-up.", false);
            }
            else
            {
                PushHistory("Unknown power-up type. Use: lightning | shield | slow", true);
            }
            return;
        }

        // ------------------------------------------------------------------
        // /weather snow | thunder   -- force-trigger a weather event
        // ------------------------------------------------------------------
        if (IEquals(cmd, "weather"))
        {
            if (tokens.size() < 2)
            {
                PushHistory("Usage: /weather [snow|thunder]", true);
                return;
            }
            if (IEquals(tokens[1], "snow"))
            {
                Weather_ForceEvent(0);                                      // 0 = snow/blizzard event
                PushHistory("Triggered snow blizzard.", false);
            }
            else if (IEquals(tokens[1], "thunder"))
            {
                Weather_ForceEvent(1);                                      // 1 = thunderstorm event
                PushHistory("Triggered thunderstorm.", false);
            }
            else
            {
                PushHistory("Unknown weather type. Use: snow | thunder", true);
            }
            return;
        }

        // ------------------------------------------------------------------
        // Unknown command
        // ------------------------------------------------------------------
        PushHistory("Unknown command: /" + cmd + "  (try /help)", true);
    }

    // ---------------------------------------------------------------------------
    // Rendering helpers
    // ---------------------------------------------------------------------------

    // Draw a solid-color quad in world space
    void DrawPanel(float x, float y, float w, float h, float r, float g, float b, float a)
    {
        if (!s_pPanelMesh) return;                                          // skip if mesh not initialised

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);                               // solid color render mode
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                // enable alpha blending for transparency
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, a);                               // apply panel color and alpha
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        Mesh_Renderer(s_pPanelMesh, x, y, w, h, 0.0f);                     // draw the quad at given world position and size
    }

    // nx/ny are already in NDC [-1,1], no conversion needed
    void DrawText(const char* text, float nx, float ny, float scale,
        float r, float g, float b)
    {
        if (Graphics::font_id < 0) return;                                  // skip if font not loaded
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::font_id, text, nx, ny, scale, r, g, b, 1.0f); // draw text at ndc position with given color
    }

    float WorldYtoNDC(float wy) { return wy / HALF_H; }                    // convert world y to NDC range -1..1
    float WorldXtoNDC(float wx) { return wx / HALF_W; }                    // convert world x to NDC range -1..1

} // anonymous namespace

// ---------------------------------------------------------------------------
// public stuff
// ---------------------------------------------------------------------------
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
        s_pEnemyAreaVisible = &is_enemy_area_box_visible;                   // store ref to debug box toggle
        s_pAutoSpawn = &is_random_spawning_enabled;                         // store ref to auto spawn toggle
        s_pDifficultyVisible = &is_difficulty_debug_visible;                // store ref to difficulty hud toggle
        s_pHealth = &health;                                                // store ref to player health
        s_pEnemyPool = &enemy_pool;                                         // store ref to enemy pool for spawn commands
        s_pFreeSlots = &free_slot_queue;                                    // store ref to free slot queue for spawn commands
        s_pEnemyArea = &enemy_area;                                         // store ref to enemy area for spawn commands
        s_pWordBank = &word_bank;                                           // store ref to word bank for spawn commands

        s_isOpen = false;                                                   // console starts closed
        s_inputLen = 0;
        s_inputBuf[0] = '\0';
        s_scrollOffset = 0;
        s_history.clear();                                                  // clear any leftover history from a previous session

        s_cursorTimer = 0.0f;
        s_cursorVisible = true;

        for (int i = 0; i < 256; ++i) s_prevKeys[i] = false;              // clear key state so no phantom inputs on first frame

        s_pPanelMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);                 // build a 1x1 unit quad, reused for all background panels
    }

    void Update()
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();

        // ---- Toggle console open with '/' key (AEVK_SLASH = 0xBF on Windows) ----
        if (KeyTriggered(AEVK_SLASH) && !s_isOpen)
        {
            s_isOpen = true;
            s_inputLen = 0;
            s_inputBuf[0] = '\0';
            s_cursorTimer = 0.0f;
            s_cursorVisible = true;
            s_inputBuf[s_inputLen++] = '/';                                 // pre-fill '/' so player sees it immediately
            s_inputBuf[s_inputLen] = '\0';
            *s_pDifficultyVisible = true;                                   // auto-show difficulty HUD when console opens
            return;                                                         // consume the '/' keypress so it doesn't get typed
        }

        if (!s_isOpen) return;                                              // skip all input handling if console is closed

        // ---- Cursor blink ----
        s_cursorTimer += dt;
        if (s_cursorTimer >= 0.5f)
        {
            s_cursorTimer = 0.0f;                                           // reset timer
            s_cursorVisible = !s_cursorVisible;                             // toggle cursor visibility every 0.5s
        }

        // ---- Scroll history with mouse wheel ----
        {
            s32 wheel = 0;
            AEInputMouseWheelDelta(&wheel);
            int maxScroll = ((int)s_history.size() - VISIBLE_LINES > 0) ? (int)s_history.size() - VISIBLE_LINES : 0; // max scroll = entries that don't fit on screen
            if (wheel > 0) s_scrollOffset = (s_scrollOffset + 1 < maxScroll) ? s_scrollOffset + 1 : maxScroll; // scroll up (older entries)
            if (wheel < 0) s_scrollOffset = (s_scrollOffset - 1 > 0) ? s_scrollOffset - 1 : 0;                 // scroll down (newer entries)
        }

        // ---- Escape closes without submitting ----
        if (KeyTriggered(AEVK_ESCAPE))
        {
            s_isOpen = false;
            s_inputLen = 0;
            s_inputBuf[0] = '\0';
            return;
        }

        // ---- Enter: execute command if input has content, close if empty ----
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
            s_inputLen = 1;                                                 // reset input buffer back to just '/'
            s_inputBuf[0] = '/';
            s_inputBuf[1] = '\0';
            s_scrollOffset = 0;                                             // snap history to bottom after command
            return;
        }

        // ---- Backspace: remove last char but never delete the leading '/' ----
        if (KeyTriggered(AEVK_BACK))
        {
            if (s_inputLen > 1)
                s_inputBuf[--s_inputLen] = '\0';                           // decrement length and null terminate
            return;
        }

        // ---- Letter, digit, and space input ----
        if (s_inputLen < (int)sizeof(s_inputBuf) - 2)                      // leave room for null terminator
        {
            for (int k = AEVK_A; k <= AEVK_Z; ++k)
            {
                if (KeyTriggered(k))
                {
                    char c = (char)('a' + (k - AEVK_A));                   // convert keycode to lowercase char
                    s_inputBuf[s_inputLen++] = c;
                    s_inputBuf[s_inputLen] = '\0';
                }
            }

            // AEVK_0 .. AEVK_9 = 0x30 .. 0x39
            for (int k = AEVK_0; k <= AEVK_9; ++k)
            {
                if (KeyTriggered(k))
                {
                    char c = (char)('0' + (k - AEVK_0));                   // convert keycode to digit char
                    s_inputBuf[s_inputLen++] = c;
                    s_inputBuf[s_inputLen] = '\0';
                }
            }

            if (KeyTriggered(AEVK_SPACE))
            {
                s_inputBuf[s_inputLen++] = ' ';                            // append space for multi-word commands
                s_inputBuf[s_inputLen] = '\0';
            }
        }
    }

    void Render()
    {
        bool hasHistory = !s_history.empty();                               // only draw history panel if there are entries

        // ---- Draw history panel background ----
        if (s_isOpen && hasHistory)
        {
            float panelY = s_isOpen
                ? HISTORY_PANEL_Y
                : -HALF_H + INPUT_BAR_H + HISTORY_PANEL_H * 0.5f + 4.0f;  // position above input bar

            DrawPanel(0.0f, panelY, SCREEN_W, HISTORY_PANEL_H, 0.0f, 0.0f, 0.0f, 0.65f); // semi-transparent black background

            int totalEntries = (int)s_history.size();
            int bottomIdx = totalEntries - 1 - s_scrollOffset;             // newest visible entry index
            int topIdx = bottomIdx - (VISIBLE_LINES - 1);                  // oldest visible entry index

            float textScale = 0.38f;
            float lineStartY = panelY + (HISTORY_PANEL_H * 0.5f) - 4.0f;  // start drawing from top of panel

            for (int line = 0; line < VISIBLE_LINES; ++line)               // draw each visible history line
            {
                int entryIdx = topIdx + line;
                if (entryIdx < 0 || entryIdx >= totalEntries) continue;    // skip out-of-range indices

                const HistoryEntry& entry = s_history[(size_t)entryIdx];

                float worldY = lineStartY - (line * HISTORY_LINE_H) - HISTORY_LINE_H * 0.5f; // calc y pos for this line
                float ny = WorldYtoNDC(worldY);
                float nx = WorldXtoNDC(-HALF_W + 12.0f);                   // left margin

                float r = entry.isError ? 1.0f : 1.0f;
                float g = entry.isError ? 0.2f : 1.0f;                     // red for errors, white for normal
                float b = entry.isError ? 0.2f : 1.0f;

                DrawText(entry.text.c_str(), nx, ny, textScale, r, g, b);
            }

            // ---- Scroll indicators ----
            int maxScroll = (totalEntries - VISIBLE_LINES > 0) ? totalEntries - VISIBLE_LINES : 0;
            if (s_scrollOffset < maxScroll)                                 // more entries above current view
            {
                float ny = WorldYtoNDC(panelY + HISTORY_PANEL_H * 0.5f - 2.0f);
                DrawText("^", WorldXtoNDC(HALF_W - 30.0f), ny, 0.35f, 0.8f, 0.8f, 0.8f);
            }
            if (s_scrollOffset > 0)                                         // more entries below current view
            {
                float ny = WorldYtoNDC(panelY - HISTORY_PANEL_H * 0.5f + 2.0f);
                DrawText("v", WorldXtoNDC(HALF_W - 30.0f), ny, 0.35f, 0.8f, 0.8f, 0.8f);
            }
        }

        if (!s_isOpen) return;                                              // skip input bar if console is closed

        DrawPanel(0.0f, INPUT_BAR_Y, SCREEN_W, INPUT_BAR_H, 0.08f, 0.08f, 0.08f, 0.92f);                  // dark background bar
        DrawPanel(0.0f, INPUT_BAR_Y + INPUT_BAR_H * 0.5f + 1.0f, SCREEN_W, 2.0f, 0.4f, 0.8f, 0.4f, 1.0f); // thin green accent line above bar

        if (Graphics::font_id >= 0)
        {
            std::string display = std::string(s_inputBuf);
            if (s_cursorVisible) display += "_";                            // append blinking cursor char

            float textScale = 0.42f;
            float nx = WorldXtoNDC(-HALF_W + 12.0f);                       // left margin
            float ny = WorldYtoNDC(INPUT_BAR_Y - 6.0f);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::font_id, display.c_str(), nx, ny, textScale, 0.6f, 1.0f, 0.6f, 1.0f); // draw input text in green
        }
    }

    void Exit()
    {
        if (s_pPanelMesh)
        {
            AEGfxMeshFree(s_pPanelMesh);                                    // free the panel quad mesh
            s_pPanelMesh = nullptr;
        }
        s_history.clear();                                                  // clear history so it doesnt persist across sessions
        s_isOpen = false;
        s_inputLen = 0;
    }

    bool IsOpen()
    {
        return s_isOpen;                                                    // used by game.cpp to freeze gameplay while console is open
    }

} // namespace DebugConsole