#pragma once
#include <string>
#include "Enemy.h"
namespace Leaderboard_Data
{
	struct ZenEntry { 

		ZenEntry(std::string playerName, int playerScore) 
			: player_name{ playerName }, player_score{ playerScore } {
		};

		std::string player_name; 
		int player_score;
	};
	struct TimedEntry
	{
		std::string player_name;
		int player_score;
	};
}

namespace Leaderboard_System
{
	void Init();
	void Free();
	int GetHighestScore();
	void Display_ZenLeaderboard(bool& openLeaderboard);
	void Display_TimeLeaderboard(bool& openLeaderboard);

	//Stores leaderboard scores into txt file
	void Read_ZenLeaderboard();
	void Read_TimeLeaderboard();

	void Write_ZenLeaderboard(const std::string& player_name, const int& player_score);
	void Write_TimeLeaderboard(const std::string& player_name, const int& player_score);

}

 