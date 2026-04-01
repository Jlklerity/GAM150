#pragma once
#include <string>
#include "Enemy.h"

namespace LeaderboardData
{
    struct ZenEntry
    {
        ZenEntry(std::string player_name, int player_score)
            : player_name{ player_name }, player_score{ player_score } {
        }

        std::string player_name;
        int         player_score;
    };

    struct TimedEntry
    {
        std::string player_name;
        int         player_score;
    };
}

namespace LeaderboardSystem
{
    void init();
    void free();
    int  get_highest_score();
    void display_zen_leaderboard(bool& open_leaderboard);
    void display_time_leaderboard(bool& open_leaderboard);

    void read_zen_leaderboard();
    void read_time_leaderboard();

    void write_zen_leaderboard(const std::string& player_name, const int& player_score);
    void write_time_leaderboard(const std::string& player_name, const int& player_score);
}