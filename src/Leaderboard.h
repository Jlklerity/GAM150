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
    void Init();
    void Free();
    int  GetHighestScore();
    void DisplayZenLeaderboard(bool& open_leaderboard);
    void DisplayTimeLeaderboard(bool& open_leaderboard);

    void ReadZenLeaderboard();
    void ReadTimeLeaderboard();

    void WriteZenLeaderboard(const std::string& player_name, const int& player_score);
    void WriteTimeLeaderboard(const std::string& player_name, const int& player_score);
}