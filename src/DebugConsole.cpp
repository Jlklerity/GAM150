#pragma warning(disable: 26426)
#define NOMINMAX
#include "pch.h"


// Internal state
namespace
{
    //  Game state references (set in init) 
    bool* s_p_enemy_area_visible = nullptr;
    bool* s_p_auto_spawn = nullptr;
    bool* s_p_difficulty_visible = nullptr;
    Data::Health* s_p_health = nullptr;
    std::vector<Data::Enemy>* s_p_enemy_pool = nullptr;
    std::deque<int>* s_p_free_slots = nullptr;
    Data::EnemyArea* s_p_enemy_area = nullptr;
    Data::WordBank* s_p_word_bank = nullptr;

    //  Console open/close 
    bool s_is_open = false;

    //  Input buffer 
    static char     s_input_buf[64]{};
    static int      s_input_len = 0;

    //  History 
    struct HistoryEntry
    {
        std::string text;
        bool        is_error; // red if true, white if false
    };

    static std::vector<HistoryEntry> s_history;
    static const int MAX_HISTORY = 64;   // max stored entries
    static const int VISIBLE_LINES = 8;    // lines shown at once
    static int s_scroll_offset = 0;    // 0 = bottom (most recent)

    //  Cursor blink 
    static float s_cursor_timer = 0.0f;
    static bool  s_cursor_visible = true;

    //  Rendering 
    static AEGfxVertexList* s_p_panel_mesh = nullptr; // reusable quad for background panels

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
    static bool s_prev_keys[256]{};

    // checks if a key was just pressed this frame (not held), param is the AE key code,
    // tracks previous frame state independently from typing.cpp so the console doesn't interfere with gameplay input
    bool key_triggered(int key)
    {
        if (key < 0 || key >= 256) return false;
        bool curr = (AEInputCheckCurr(static_cast<u8>(key)) != 0);
        bool triggered = curr && !s_prev_keys[key];
        s_prev_keys[key] = curr;
        return triggered;
    }

    // History helpers
    // adds a line to the command history, param is the text and whether it's an error (red) or normal (white),
    // drops the oldest entry if we exceed MAX_HISTORY, and snaps the scroll back to the bottom so the new entry is visible
    void push_history(const std::string& text, bool is_error = false)
    {
        s_history.push_back({ text, is_error });
        if ((int)s_history.size() > MAX_HISTORY)
            s_history.erase(s_history.begin()); // drop oldest
        s_scroll_offset = 0; // snap back to bottom on new entry
    }

    // Command parsing helpers
    // splits a command string into tokens by whitespace, e.g. "spawn 5" -> ["spawn", "5"],
    // used by ExecuteCommand to separate the command name from its arguments
    std::vector<std::string> tokenise(const std::string& line)
    {
        std::vector<std::string> tokens;
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);
        return tokens;
    }

    // Case-insensitive compare
    // case-insensitive string compare, so "/Spawn" and "/spawn" both work,
    // returns true if both strings are the same length and all chars match when lowercased
    bool i_equals(const std::string& a, const std::string& b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
        return true;
    }

    // Command execution
    // parses and runs a command string entered by the player, param is the raw input including the leading '/',
    // echoes the command into history, strips the slash, tokenises, then dispatches to the matching handler,
    // pushes an error message to history if the command is unknown or used incorrectly
    void execute_command(const std::string& raw)
    {
        // Echo the typed command into history (white)
        push_history("> " + raw, false);

        // Strip leading '/' 
        std::string line = raw;
        if (!line.empty() && line[0] == '/') line = line.substr(1);

        auto tokens = tokenise(line);
        if (tokens.empty()) return;

        const std::string& cmd = tokens[0];

        // /spawn [3-8]   -- spawn an enemy of the given word length
        if (i_equals(cmd, "spawn"))
        {
            if (tokens.size() < 2)
            {
                push_history("Usage: /spawn [3-8] | friendly | shield | boss | minion", true);
                return;
            }

            // /spawn friendly
            if (i_equals(tokens[1], "friendly"))
            {
                int max_len = DifficultySystem::difficulty_get_unlocked_max_word_length();
                EnemySystem::enemy_spawn_friendly_from_pool(*s_p_enemy_pool, *s_p_free_slots, *s_p_enemy_area, *s_p_word_bank, 3, max_len, 1.15f);
                push_history("Spawned friendly enemy.", false);
                return;
            }

            // /spawn shield
            if (i_equals(tokens[1], "shield"))
            {
                int max_len = DifficultySystem::difficulty_get_unlocked_max_word_length();
                EnemySystem::enemy_spawn_shielded_from_pool(*s_p_enemy_pool, *s_p_free_slots, *s_p_enemy_area, *s_p_word_bank, 3, max_len, 1.05f);
                push_history("Spawned shielded enemy.", false);
                return;
            }

            // /spawn boss
            if (i_equals(tokens[1], "boss"))
            {
                BossSystem::boss_force_spawn();
                push_history("Spawned boss.", false);
                return;
            }

            // /spawn minion - spawn one boss minion (boss must be active)
            if (i_equals(tokens[1], "minion"))
            {
                if (!BossSystem::boss_is_active())
                {
                    push_history("Error: boss must be active to spawn a minion.", true);
                    return;
                }
                int len = Utility::utility_rand_int(BossConfig::BOSS_WORD_MIN_LEN, BossConfig::BOSS_WORD_MAX_LEN);
                EnemySystem::enemy_spawn_from_pool(*s_p_enemy_pool, *s_p_free_slots, *s_p_enemy_area, *s_p_word_bank, len);
                push_history("Spawned boss minion (word length " + std::to_string(len) + ").", false);
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
                push_history("Error: unknown spawn type. Use [3-8], friendly, shield, boss, minion.", true);
                return;
            }
            if (len < 3 || len > 8)
            {
                push_history("Error: word length must be between 3 and 8.", true);
                return;
            }
            EnemySystem::enemy_spawn_from_pool(*s_p_enemy_pool, *s_p_free_slots, *s_p_enemy_area, *s_p_word_bank, len);
            push_history("Spawned enemy with word length " + std::to_string(len) + ".", false);
            return;                                                         // must return or falls through to unknown command
        }

        // /kill boss - instantly kill the active boss
        if (i_equals(cmd, "kill"))
        {
            if (tokens.size() < 2)
            {
                push_history("Usage: /kill boss", true);
                return;
            }
            if (!BossSystem::boss_is_active())
            {
                push_history("Error: boss must be active to kill it.", true);
                return;
            }
            if (i_equals(tokens[1], "boss"))
            {
                BossSystem::boss_force_kill(*s_p_enemy_pool, *s_p_free_slots);
                push_history("Boss force-killed.", false);
            }
            else
            {
                push_history("Unknown kill target. Try: /kill boss", true);
            }
            return;
        }

        // /health add    -- add 10 hp
        if (i_equals(cmd, "health"))
        {
            if (tokens.size() < 2)
            {
                push_history("Usage: /health add", true);
                return;
            }
            if (i_equals(tokens[1], "add"))
            {
                HealthSystem::health_add(*s_p_health, 10.0f);
                push_history("Added 10 HP. Current: " + std::to_string((int)s_p_health->health_current), false);
            }
            else if (i_equals(tokens[1], "die")) {
                s_p_health->health_current = 0;
                push_history("Player killed.", false);
            }
            else
            {
                push_history("Unknown health subcommand. Try: /health add", true);
            }
            return;
        }


        // /difficulty up | down | show
        if (i_equals(cmd, "difficulty"))
        {
            if (tokens.size() < 2)
            {
                push_history("Usage: /difficulty [up|down|show]", true);
                return;
            }
            if (i_equals(tokens[1], "up"))
            {
                DifficultySystem::difficulty_add(1.0f);
                push_history("Difficulty increased to " +
                    std::to_string((int)DifficultySystem::difficulty_get()) + ".", false);
            }
            else if (i_equals(tokens[1], "down"))
            {
                DifficultySystem::difficulty_add(-1.0f);
                push_history("Difficulty decreased to " +
                    std::to_string((int)DifficultySystem::difficulty_get()) + ".", false);
            }
            else if (i_equals(tokens[1], "show"))
            {
                *s_p_difficulty_visible = !*s_p_difficulty_visible;
                push_history(std::string("Difficulty HUD: ") + (*s_p_difficulty_visible ? "ON" : "OFF"), false);
            }
            else
            {
                push_history("Unknown difficulty subcommand. Try: up | down | show", true);
            }
            return;
        }

        // /debugbox - toggle enemy area debug box
        if (i_equals(cmd, "debugbox"))
        {
            *s_p_enemy_area_visible = !*s_p_enemy_area_visible;
            push_history(std::string("Debug box: ") + (*s_p_enemy_area_visible ? "ON" : "OFF"), false);
            return;
        }

        // /autospawn - toggle automatic enemy spawning
        if (i_equals(cmd, "autospawn"))
        {
            *s_p_auto_spawn = !*s_p_auto_spawn;
            push_history(std::string("Auto-spawn: ") + (*s_p_auto_spawn ? "ON" : "OFF"), false);
            return;
        }

        // /help - list all commands
        if (i_equals(cmd, "help"))
        {
            push_history("/spawn [3-8]         - spawn enemy by word length", false);
            push_history("/spawn friendly      - spawn a friendly enemy", false);
            push_history("/spawn shield        - spawn a shielded enemy", false);
            push_history("/spawn boss          - force-spawn the boss", false);
            push_history("/spawn minion        - spawn a boss minion (boss required)", false);
            push_history("/kill boss           - instantly kill the active boss (boss required)", false);
            push_history("/trickroom           - force-trigger Trick Room (boss required)", false);
            push_history("/health add          - add 10 HP", false);
            push_history("/health die          - kills player", false);
            push_history("/difficulty up       - increase difficulty by 1", false);
            push_history("/difficulty down     - decrease difficulty by 1", false);
            push_history("/difficulty show     - toggle difficulty HUD", false);
            push_history("/debugbox            - toggle enemy area box", false);
            push_history("/autospawn           - toggle auto enemy spawning", false);
            push_history("/powerup lightning   - force-spawn Lightning drop", false); // azfar code start
            push_history("/powerup shield      - force-spawn Shield drop", false);
            push_history("/powerup slow        - force-spawn TimeSlow drop", false);
            push_history("/weather snow        - trigger blizzard event", false);
            push_history("/weather thunder     - trigger thunderstorm event", false);
            push_history("/weather rainbow     - trigger rainbow event", false);
            push_history("/weather ghost       - trigger ghost weather event", false);

            return;
        }

        // /powerup lightning | shield | slow | force spawn a power up drop
        if (i_equals(cmd, "powerup"))
        {
            if (tokens.size() < 2)
            {
                push_history("Usage: /powerup [lightning|shield|slow]", true);
                return;
            }
            if (i_equals(tokens[1], "lightning"))
            {
                PowerUpSystem::debug_spawn(PowerUpSystem::PowerType::LIGHTNING);
                push_history("Spawned Lightning power-up.", false);
            }
            else if (i_equals(tokens[1], "shield"))
            {
                PowerUpSystem::debug_spawn(PowerUpSystem::PowerType::SHIELD);
                push_history("Spawned Shield power-up.", false);
            }
            else if (i_equals(tokens[1], "slow"))
            {
                PowerUpSystem::debug_spawn(PowerUpSystem::PowerType::TIME_SLOW);
                push_history("Spawned TimeSlow power-up.", false);
            }
            else
            {
                push_history("Unknown power-up type. Use: lightning | shield | slow", true);
            }
            return;
        }

        // /weather snow | thunder | rainbow | ghost  -- force-trigger a weather event
        if (i_equals(cmd, "weather"))
        {
            if (tokens.size() < 2)
            {
                push_history("Usage: /weather [snow|thunder|rainbow|ghost]", true);
                return;
            }
            if (i_equals(tokens[1], "snow"))
            {
                weather_force_event(0, *s_p_enemy_pool);
                push_history("Triggered snow blizzard.", false);
            }
            else if (i_equals(tokens[1], "thunder"))
            {
                weather_force_event(1, *s_p_enemy_pool);
                push_history("Triggered thunderstorm.", false);
            }
            else if (i_equals(tokens[1], "rainbow"))
            {
                weather_force_event(2, *s_p_enemy_pool);
                push_history("Triggered rainbow.", false);
            }
            else if (i_equals(tokens[1], "ghost"))
            {
                weather_force_event(3, *s_p_enemy_pool);
                push_history("Triggered ghost weather.", false);
            }
            else
            {
                push_history("Unknown weather type. Use: snow | thunder | rainbow | ghost", true);
            }
            return;
        }


        // /trickroom - immediately trigger Trick Room (boss must be active)
        if (i_equals(cmd, "trickroom"))
        {
            if (!BossSystem::boss_is_active())
            {
                push_history("Error: boss must be active to trigger Trick Room.", true);
                return;
            }
            BossSystem::boss_force_trick_room();
            push_history("Trick Room triggered! Type all words in reverse for 20s.", false);
            return;
        }

        // Unknown command
        push_history("Unknown command: /" + cmd + "  (try /help)", true);
    }



    // Rendering helpers
    // ---------------------------------------------------------------------------

    // Draw a solid color quad in world space
    // draws a solid colour rectangle in world space, params are centre x/y, width, height, and rgba colour,
    // used for the input bar background, history panel background, and the green divider line
    void draw_panel(float x, float y, float w, float h, float r, float g, float b, float a)
    {
        if (!s_p_panel_mesh) return;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);
        AEGfxSetColorToMultiply(r, g, b, a);
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
        mesh_renderer(s_p_panel_mesh, x, y, w, h, 0.0f);
    }

    // Print text; nx/ny are already in NDC [-1,1]
    // prints text at the given NDC position with the given scale and rgb colour,
    // nx/ny must already be in NDC [-1, 1] range - use WorldXtoNDC/WorldYtoNDC to convert first
    void draw_text(const char* text, float nx, float ny, float scale,
        float r, float g, float b)
    {
        if (Graphics::g_font_id < 0) return;
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxPrint(Graphics::g_font_id, text, nx, ny, scale, r, g, b, 1.0f);
    }

    // Convert world Y to NDC
    // converts a world space Y coordinate to NDC [-1, 1] for use with AEGfxPrint
    float world_y_to_ndc(float wy) { return wy / HALF_H; }

    // converts a world space X coordinate to NDC [-1, 1] for use with AEGfxPrint
    float world_x_to_ndc(float wx) { return wx / HALF_W; }

} // anonymous namespace

// public stuff
namespace DebugConsole
{
    // sets up the console and stores references to all the game state the commands need to modify,
    // must be called once in game_init() before any Update/Render calls,
    // params are references to the spawning flags, health, enemy pool, free slots, enemy area, and word bank
    void init(
        bool& is_enemy_area_box_visible,
        bool& is_random_spawning_enabled,
        bool& is_difficulty_debug_visible,
        Data::Health& health,
        std::vector<Data::Enemy>& enemy_pool,
        std::deque<int>& free_slot_queue,
        Data::EnemyArea& enemy_area,
        Data::WordBank& word_bank)
    {
        s_p_enemy_area_visible = &is_enemy_area_box_visible;
        s_p_auto_spawn = &is_random_spawning_enabled;
        s_p_difficulty_visible = &is_difficulty_debug_visible;
        s_p_health = &health;
        s_p_enemy_pool = &enemy_pool;
        s_p_free_slots = &free_slot_queue;
        s_p_enemy_area = &enemy_area;
        s_p_word_bank = &word_bank;

        s_is_open = false;
        s_input_len = 0;
        s_input_buf[0] = '\0';
        s_scroll_offset = 0;
        s_history.clear();

        s_cursor_timer = 0.0f;
        s_cursor_visible = true;

        for (int i = 0; i < 256; ++i) s_prev_keys[i] = false;

        if (s_p_panel_mesh) { AEGfxMeshFree(s_p_panel_mesh); s_p_panel_mesh = nullptr; }
        s_p_panel_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    }

    // handles the '/' key to open the console, and all keyboard input while it's open,
    // call this every frame BEFORE the gameplay freeze check so the console can still respond when gameplay is paused,
    // handles backspace, enter (submit or close), escape (close), letter/digit/space keys, and mouse wheel scrolling
    void update()
    {
        float dt = (float)AEFrameRateControllerGetFrameTime();

        //  Toggle console open / 
        // AEVK_SLASH = 0xBF 
        if (key_triggered(AEVK_SLASH) && !s_is_open) //if console not open + / presed
        {
            s_is_open = true;
            s_input_len = 0;
            s_input_buf[0] = '\0';
            s_cursor_timer = 0.0f;
            s_cursor_visible = true;
            s_input_buf[s_input_len++] = '/';                                 // pre-fill / 
            s_input_buf[s_input_len] = '\0';
            *s_p_difficulty_visible = true;                                   // auto-show difficulty HUD when console opens
            return;                                                         // consume the /
        }

        if (!s_is_open) return; // if console not open returb

        //  Cursor blink 
        s_cursor_timer += dt;
        if (s_cursor_timer >= 0.5f)
        {
            s_cursor_timer = 0.0f;
            s_cursor_visible = !s_cursor_visible;
        }

        //  Scroll history with mouse wheel 
        {
            s32 wheel = 0;
            AEInputMouseWheelDelta(&wheel);
            int max_scroll = ((int)s_history.size() - VISIBLE_LINES > 0) ? (int)s_history.size() - VISIBLE_LINES : 0;
            if (wheel > 0) s_scroll_offset = (s_scroll_offset + 1 < max_scroll) ? s_scroll_offset + 1 : max_scroll; // scroll up (older)
            if (wheel < 0) s_scroll_offset = (s_scroll_offset - 1 > 0) ? s_scroll_offset - 1 : 0;                 // scroll down (newer)
        }

        //  Escape closes without submitting 
        if (key_triggered(AEVK_ESCAPE))
        {
            s_is_open = false;
            s_input_len = 0;
            s_input_buf[0] = '\0';
            *s_p_difficulty_visible = false;                                  // hide difficulty HUD when console closes
            return;
        }

        //  Enter submits 
        if (key_triggered(AEVK_RETURN))
        {
            std::string cmd(s_input_buf);
            while (!cmd.empty() && cmd.back() == ' ') cmd.pop_back();      // trim trailing spaces

            if (cmd.empty() || cmd == "/")
            {
                s_is_open = false;                                           // empty enter = close console
                s_input_len = 0;
                s_input_buf[0] = '\0';
                s_scroll_offset = 0;
                *s_p_difficulty_visible = false;                              // hide difficulty HUD when console closes
                return;
            }

            execute_command(cmd);                                            // non-empty = run command and stay open
            s_input_len = 1;                                                 // reset buffer back to just '/'
            s_input_buf[0] = '/';
            s_input_buf[1] = '\0';
            s_scroll_offset = 0;                                             // snap history to bottom after command
            return;
        }

        //  Backspace 
        if (key_triggered(AEVK_BACK))
        {
            // Don't let player delete the leading '/'
            if (s_input_len > 1)
            {
                s_input_buf[--s_input_len] = '\0';
            }
            return;
        }

        //  Letter keys A-Z 
        if (s_input_len < (int)sizeof(s_input_buf) - 2) // leave room for null
        {
            for (int k = AEVK_A; k <= AEVK_Z; ++k)
            {
                if (key_triggered(k))
                {
                    char c = (char)('a' + (k - AEVK_A)); // lowercase
                    s_input_buf[s_input_len++] = c;
                    s_input_buf[s_input_len] = '\0';
                }
            }

            //  Digit keys 0-9 
            // Alpha Engine AEVK_0 - AEVK_9 = 0x30 - 0x39
            for (int k = AEVK_0; k <= AEVK_9; ++k)
            {
                if (key_triggered(k))
                {
                    char c = (char)('0' + (k - AEVK_0));
                    s_input_buf[s_input_len++] = c;
                    s_input_buf[s_input_len] = '\0';
                }
            }

            //  Space 
            if (key_triggered(AEVK_SPACE))
            {
                s_input_buf[s_input_len++] = ' ';
                s_input_buf[s_input_len] = '\0';
            }
        }
    }

    // draws the console UI on top of everything else, call this at the end of the render block,
    // draws the scrollable history panel when the console is open, and the input bar with blinking cursor,
    // also shows up/down scroll indicators when there are more history entries than visible lines
    void render()
    {
        // History panel is always visible if there are entries (even when console is closed)
        // Input bar only visible when open
        bool has_history = !s_history.empty();

        //  Draw history panel background 
        if (s_is_open && has_history)
        {
            float panel_y = s_is_open ? HISTORY_PANEL_Y : -HALF_H + INPUT_BAR_H + HISTORY_PANEL_H * 0.5f + 4.0f;

            draw_panel(0.0f, panel_y, SCREEN_W, HISTORY_PANEL_H, 0.0f, 0.0f, 0.0f, 0.65f); // history panel background

            //  Draw history lines 
            // s_scroll_offset=0 means show the newest VISIBLE_LINES entries
            int total_entries = (int)s_history.size();
            // The bottom of the visible window in history index terms:
            int bottom_idx = total_entries - 1 - s_scroll_offset;         // newest visible
            int top_idx = bottom_idx - (VISIBLE_LINES - 1);           // oldest visible

            float text_scale = 0.38f;
            float line_start_y = panel_y + (HISTORY_PANEL_H * 0.5f) - 4.0f; // top of panel

            // loop through each visible line slot in the history panel
            for (int line = 0; line < VISIBLE_LINES; ++line)
            {
                // calculate which history entry maps to this row
                int entry_idx = top_idx + line;

                // skip if out of bounds (<8)
                if (entry_idx < 0 || entry_idx >= total_entries) continue;

                // grab the history entry at this index
                const HistoryEntry& entry = s_history[(size_t)entry_idx];

                // calculate world Y position for this row, gg downward each line
                float world_y = line_start_y - (line * HISTORY_LINE_H) - HISTORY_LINE_H * 0.5f;

                // convert to NDC for print
                float ny = world_y_to_ndc(world_y);


                float nx = world_x_to_ndc(-HALF_W + 12.0f);

                // pick red for errors, white for normal output
                float r = entry.is_error ? 1.0f : 1.0f;
                float g = entry.is_error ? 0.2f : 1.0f;
                float b = entry.is_error ? 0.2f : 1.0f;

                // draw the text at the calculated position and color
                draw_text(entry.text.c_str(), nx, ny, text_scale, r, g, b);
            }

            //  Scroll indicators 
            int max_scroll = (total_entries - VISIBLE_LINES > 0) ? total_entries - VISIBLE_LINES : 0;
            if (s_scroll_offset < max_scroll) // can scroll up (older entries above)
            {
                float ny = world_y_to_ndc(panel_y + HISTORY_PANEL_H * 0.5f - 2.0f);
                draw_text("^", world_x_to_ndc(HALF_W - 30.0f), ny, 0.35f, 0.8f, 0.8f, 0.8f);
            }
            if (s_scroll_offset > 0) // can scroll down (newer entries below)
            {
                float ny = world_y_to_ndc(panel_y - HISTORY_PANEL_H * 0.5f + 2.0f);
                draw_text("v", world_x_to_ndc(HALF_W - 30.0f), ny, 0.35f, 0.8f, 0.8f, 0.8f);
            }
        }

        //  Draw input bar (only when open) 
        if (!s_is_open) return;

        // Background bar
        draw_panel(0.0f, INPUT_BAR_Y, SCREEN_W, INPUT_BAR_H, 0.08f, 0.08f, 0.08f, 0.92f);

        // Thin line above the bar
        draw_panel(0.0f, INPUT_BAR_Y + INPUT_BAR_H * 0.5f + 1.0f, SCREEN_W, 2.0f, 0.4f, 0.8f, 0.4f, 1.0f);

        // Input text + cursor
        if (Graphics::g_font_id >= 0)
        {
            std::string display = std::string(s_input_buf);
            if (s_cursor_visible) display += "_";

            float text_scale = 0.42f;
            float nx = world_x_to_ndc(-HALF_W + 12.0f);
            float ny = world_y_to_ndc(INPUT_BAR_Y - 6.0f);

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);
            AEGfxPrint(Graphics::g_font_id, display.c_str(), nx, ny, text_scale, 0.6f, 1.0f, 0.6f, 1.0f);
        }
    }

    // frees the panel mesh and clears history, call this in game_exit() to avoid memory leaks
    void cleanup()
    {
        if (s_p_panel_mesh)
        {
            AEGfxMeshFree(s_p_panel_mesh);
            s_p_panel_mesh = nullptr;
        }
        s_history.clear();
        s_is_open = false;
        s_input_len = 0;
    }

    // returns true while the console is open, use this in game.cpp to freeze all gameplay logic
    bool is_open()
    {
        return s_is_open;
    }

} // namespace DebugConsole