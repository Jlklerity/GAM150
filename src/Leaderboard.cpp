/*!
@file           Leaderboard.cpp
@author         Jeremy Lim
@brief          Manages reading, writing, and rendering the Zen and Timed mode leaderboards.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

#include "pch.h"
#include "Leaderboard.h"
#include <AEEngine.h>

/*!
@namespace      Anonymous
@brief          Private internal state for scroll offsets, highest scores, and cached leaderboard data structures.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace
{
    static float s_scroll_offset = 0.0f;
    static int   s_highest_zen_score = 0;
    static int   s_highest_time_score = 0;
    static int   s_highest_score = 0;

    constexpr int MAX_LEADERBOARD_SIZE = 50;
    constexpr int ENTRIES_PER_PAGE = 5;
    constexpr int LB_VISIBLE = 5;

    static float s_lb_scroll = 0.0f;

    static AEGfxVertexList* s_overlay_mesh = nullptr;

    static std::vector<LeaderboardData::ZenEntry>   s_leaderboard_list;
    static std::vector<LeaderboardData::TimedEntry> s_timed_board;
}

/*!
@namespace      LeaderboardSystem
@brief          System responsible for managing file I/O operations and UI overlays for the leaderboards.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace LeaderboardSystem
{

    /*!
    @function       init
    @brief          Initializes the leaderboard overlay mesh.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void init()
    {
        if (s_overlay_mesh)
        {
            AEGfxMeshFree(s_overlay_mesh);
            s_overlay_mesh = nullptr;
        }
        s_overlay_mesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
    }

    /*!
    @function       free
    @brief          Frees the overlay mesh and clears cached vector data to release memory.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void free()
    {
        if (s_overlay_mesh)
        {
            AEGfxMeshFree(s_overlay_mesh);
            s_overlay_mesh = nullptr;
        }
        s_leaderboard_list.clear();
        s_leaderboard_list.shrink_to_fit();
        s_timed_board.clear();
        s_timed_board.shrink_to_fit();
    }

    /*!
    @function       get_highest_score
    @brief          Reads both leaderboards and returns the highest score the player has achieved across both modes.
    @return         An integer representing the absolute highest score.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    int get_highest_score()
    {
        read_zen_leaderboard();
        read_time_leaderboard();
        s_highest_score = (s_highest_time_score > s_highest_zen_score) ? s_highest_time_score : s_highest_zen_score;
        return s_highest_score;
    }

    /*!
    @function       compare_zen_scores
    @brief          Comparator function used to sort Zen mode leaderboard entries in descending order.
    @param          a (const LeaderboardData::ZenEntry&) The first entry to compare.
    @param          b (const LeaderboardData::ZenEntry&) The second entry to compare.
    @return         True if the first score is greater than the second score.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    static bool compare_zen_scores(const LeaderboardData::ZenEntry& a, const LeaderboardData::ZenEntry& b)
    {
        return a.player_score > b.player_score;
    }

    /*!
    @function       compare_timed_scores
    @brief          Comparator function used to sort Timed mode leaderboard entries in descending order.
    @param          a (const LeaderboardData::TimedEntry&) The first entry to compare.
    @param          b (const LeaderboardData::TimedEntry&) The second entry to compare.
    @return         True if the first score is greater than the second score.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    static bool compare_timed_scores(const LeaderboardData::TimedEntry& a, const LeaderboardData::TimedEntry& b)
    {
        return a.player_score > b.player_score;
    }

    /*!
    @function       read_zen_leaderboard
    @brief          Reads the Zen mode leaderboard data from the local text file into the cached vector.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void read_zen_leaderboard()
    {
        s_leaderboard_list.clear();
        const char* leaderboard_filepath = "Assets/leaderboard.txt";
        std::ifstream file_stream(leaderboard_filepath);
        if (!file_stream.is_open())
        {
            std::cout << "Unable to open zen leaderboard\n";
            return;
        }

        std::string player_name;
        int player_score;
        while (file_stream >> player_name >> player_score)
        {
            LeaderboardData::ZenEntry new_entry{ player_name, player_score };
            if (player_score > s_highest_zen_score) s_highest_zen_score = player_score;
            s_leaderboard_list.push_back(new_entry);
            std::cout << new_entry.player_name << new_entry.player_score << "\n";
        }
        file_stream.close();
    }

    /*!
    @function       read_time_leaderboard
    @brief          Opens the local text file for Timed mode scores, reads the data, and populates the cached vector array.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void read_time_leaderboard()
    {
        s_timed_board.clear();
        s_lb_scroll = 0.0f;

        const char* TIMED_LB_PATH = "Assets/leaderboard_timed.txt";
        std::ifstream f(TIMED_LB_PATH);
        if (!f.is_open()) return;

        std::string n;
        int s;
        while (f >> n >> s)
        {
            if (s > s_highest_time_score) s_highest_time_score = s;
            s_timed_board.push_back({ n, s });
        }
        f.close();
    }

    /*!
    @function       write_zen_leaderboard
    @brief          Appends a new score to the Zen leaderboard, sorts it, and writes the updated list back to the local text file.
    @param          player_name (const std::string&) The name of the player.
    @param          player_score (const int&) The score achieved.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void write_zen_leaderboard(const std::string& player_name, const int& player_score)
    {
        read_zen_leaderboard();
        LeaderboardData::ZenEntry new_entry{ player_name, player_score };
        std::cout << "\n";

        s_leaderboard_list.push_back(new_entry);
        std::sort(s_leaderboard_list.begin(), s_leaderboard_list.end(), compare_zen_scores);

        const char* leaderboard_filepath = "Assets/leaderboard.txt";
        std::ofstream file_stream(leaderboard_filepath);
        if (!file_stream.is_open())
        {
            std::cout << "Unable to open leaderboard\n";
            return;
        }

        for (const auto& entry : s_leaderboard_list)
            file_stream << entry.player_name << " " << entry.player_score << "\n";

        file_stream.close();
    }

    /*!
    @function       write_time_leaderboard
    @brief          Appends a new score to the Timed leaderboard, sorts the vector, and rewrites the updated list back to the local text file.
    @param          player_name (const std::string&) The name of the player.
    @param          player_score (const int&) The score achieved by the player.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void write_time_leaderboard(const std::string& player_name, const int& player_score)
    {
        read_time_leaderboard();
        s_timed_board.push_back({ player_name, player_score });
        std::sort(s_timed_board.begin(), s_timed_board.end(), compare_timed_scores);

        const char* TIMED_LB_PATH = "Assets/leaderboard_timed.txt";
        std::ofstream f(TIMED_LB_PATH);
        if (!f.is_open()) return;

        for (const auto& e : s_timed_board)
            f << e.player_name << " " << e.player_score << "\n";

        f.close();
    }

    /*!
    @function       display_zen_leaderboard
    @brief          Renders the Zen leaderboard UI, handles scroll wheel input, and logic to exit the screen.
    @param          open_leaderboard (bool&) Reference to the boolean tracking if the leaderboard is open.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void display_zen_leaderboard(bool& open_leaderboard)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.85f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        mesh_renderer(s_overlay_mesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
        AEGfxSetTransparency(1.0f);

        if (Graphics::g_font_id < 0) return;

        // ---- Title ----
        const char* title = "ZEN MODE - LEADERBOARD";
        float tw, th;
        AEGfxGetPrintSize(Graphics::g_font_id, title, 0.7f, &tw, &th);
        AEGfxPrint(Graphics::g_font_id, title, -tw * 0.5f, 0.75f, 0.7f, 1.0f, 0.85f, 0.2f, 1.0f);

        // ---- Mouse wheel scrolling ----
        s32 wheel = 0;
        AEInputMouseWheelDelta(&wheel);
        int max_scroll = ((int)s_leaderboard_list.size() - LB_VISIBLE > 0)
            ? (int)s_leaderboard_list.size() - LB_VISIBLE : 0;
        if (wheel > 0) s_scroll_offset = (s_scroll_offset - 1.0f < 0.0f) ? 0.0f : s_scroll_offset - 1.0f;
        if (wheel < 0) s_scroll_offset = (s_scroll_offset + 1.0f > (float)max_scroll) ? (float)max_scroll : s_scroll_offset + 1.0f;

        constexpr float START_Y = 0.5f;
        constexpr float SPACING = 0.2f;

        for (int i = 0; i < LB_VISIBLE; ++i)
        {
            int idx = (int)s_scroll_offset + i;
            if (idx >= (int)s_leaderboard_list.size()) break;

            const auto& e = s_leaderboard_list[(size_t)idx];
            std::string line = std::to_string(idx + 1) + ".   " + e.player_name + "   " + std::to_string(e.player_score);
            float y = START_Y - (i * SPACING);

            if (idx == 0)
                AEGfxPrint(Graphics::g_font_id, line.c_str(), -0.3f, y, 0.9f, 1.0f, 1.0f, 0.0f, 1.0f); // yellow
            else
                AEGfxPrint(Graphics::g_font_id, line.c_str(), -0.3f, y, 0.9f, 1.0f, 1.0f, 1.0f, 1.0f); // white
        }

        // ---- Scroll indicators ----
        if (s_scroll_offset > 0)
            AEGfxPrint(Graphics::g_font_id, "^", 0.4f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f);
        if ((int)s_scroll_offset < max_scroll)
            AEGfxPrint(Graphics::g_font_id, "v", 0.4f, -0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f);

        // ---- Return prompt ----
        const char* prompt = "Press ESC or Click to Return";
        float pw, ph;
        AEGfxGetPrintSize(Graphics::g_font_id, prompt, 0.45f, &pw, &ph);
        AEGfxPrint(Graphics::g_font_id, prompt, -pw * 0.5f, -0.85f, 0.45f, 1.0f, 1.0f, 0.0f, 1.0f);

        if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            open_leaderboard = false;
        }
        else if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);

            float win_h = (float)AEGfxGetWindowHeight();
            float win_w = (float)AEGfxGetWindowWidth();
            float wx = (float)mx - (win_w * 0.5f);
            float wy = (win_h * 0.5f) - (float)my;

            constexpr float ARROW_W = 80.0f;
            constexpr float ARROW_H = 80.0f;
            constexpr float ARROW_Y = 0.0f;

            // Zen arrow is on the RIGHT side
            float arrow_x = (win_w * 0.5f) - 380.0f;

            if (!(wx >= arrow_x - ARROW_W * 1.5f && wx <= arrow_x + ARROW_W * 1.5f &&
                wy >= ARROW_Y - ARROW_H * 1.2f && wy <= ARROW_Y + ARROW_H * 1.2f))
            {
                open_leaderboard = false;
            }
        }
    }

    /*!
    @function       display_time_leaderboard
    @brief          Renders the Timed leaderboard UI, handles scroll wheel input for pagination, and processes exit logic.
    @param          open_leaderboard (bool&) Reference to the flag controlling the visibility of the leaderboard. Set to false to close.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void display_time_leaderboard(bool& open_leaderboard)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(0.85f);
        AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
        mesh_renderer(s_overlay_mesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
        AEGfxSetTransparency(1.0f);

        if (Graphics::g_font_id < 0) return;

        // ---- Title ----
        const char* title = "TIMED MODE - LEADERBOARD";
        float tw, th;
        AEGfxGetPrintSize(Graphics::g_font_id, title, 0.7f, &tw, &th);
        AEGfxPrint(Graphics::g_font_id, title, -tw * 0.5f, 0.75f, 0.7f, 1.0f, 0.85f, 0.2f, 1.0f);

        // ---- Mouse wheel scrolling ----
        s32 wheel = 0;
        AEInputMouseWheelDelta(&wheel);
        int max_scroll = ((int)s_timed_board.size() - LB_VISIBLE > 0)
            ? (int)s_timed_board.size() - LB_VISIBLE : 0;
        if (wheel > 0) s_lb_scroll = (s_lb_scroll - 1.0f < 0.0f) ? 0.0f : s_lb_scroll - 1.0f;
        if (wheel < 0) s_lb_scroll = (s_lb_scroll + 1.0f > (float)max_scroll) ? (float)max_scroll : s_lb_scroll + 1.0f;

        constexpr float START_Y = 0.5f;
        constexpr float SPACING = 0.2f;

        for (int i = 0; i < LB_VISIBLE; ++i)
        {
            int idx = (int)s_lb_scroll + i;
            if (idx >= (int)s_timed_board.size()) break;

            const auto& e = s_timed_board[(size_t)idx];
            std::string line = std::to_string(idx + 1) + ".   " + e.player_name + "   " + std::to_string(e.player_score);
            float y = START_Y - (i * SPACING);

            if (idx == 0)
                AEGfxPrint(Graphics::g_font_id, line.c_str(), -0.3f, y, 0.9f, 1.0f, 1.0f, 0.0f, 1.0f); // yellow
            else
                AEGfxPrint(Graphics::g_font_id, line.c_str(), -0.3f, y, 0.9f, 1.0f, 1.0f, 1.0f, 1.0f); // white
        }

        // ---- Scroll indicators ----
        if (s_lb_scroll > 0)
            AEGfxPrint(Graphics::g_font_id, "^", 0.4f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f);
        if ((int)s_lb_scroll < max_scroll)
            AEGfxPrint(Graphics::g_font_id, "v", 0.4f, -0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f);

        // ---- Return prompt ----
        const char* prompt = "Press ESC or Click to Return";
        float pw, ph;
        AEGfxGetPrintSize(Graphics::g_font_id, prompt, 0.45f, &pw, &ph);
        AEGfxPrint(Graphics::g_font_id, prompt, -pw * 0.5f, -0.85f, 0.45f, 1.0f, 1.0f, 0.0f, 1.0f);

        if (AEInputCheckTriggered(AEVK_ESCAPE))
        {
            open_leaderboard = false;
        }
        else if (AEInputCheckTriggered(AEVK_LBUTTON))
        {
            s32 mx, my;
            AEInputGetCursorPosition(&mx, &my);

            float win_h = (float)AEGfxGetWindowHeight();
            float win_w = (float)AEGfxGetWindowWidth();
            float wx = (float)mx - (win_w * 0.5f);
            float wy = (win_h * 0.5f) - (float)my;

            constexpr float ARROW_W = 80.0f;
            constexpr float ARROW_H = 80.0f;
            constexpr float ARROW_Y = 0.0f;

            // Time arrow is on the LEFT side
            float arrow_x = -(win_w * 0.5f) + 380.0f;

            if (!(wx >= arrow_x - ARROW_W * 1.5f && wx <= arrow_x + ARROW_W * 1.5f &&
                wy >= ARROW_Y - ARROW_H * 1.2f && wy <= ARROW_Y + ARROW_H * 1.2f))
            {
                open_leaderboard = false;
            }
        }
    }
}