#pragma once
#include <string>
#include "Enemy.h"
namespace Leaderboard_Data
{
	struct Entry { 

		Entry(std::string playerName, int playerScore) 
			: player_name{ playerName }, player_score{ playerScore } {
		};

		std::string player_name; 
		int player_score;
	};
}

namespace Leaderboard_System
{
	void Init();
	void Free();
	int GetHighestScore();
	void Display_Leaderboard(bool& openLeaderboard);

	//Stores leaderboard scores into txt file
	void Read_Leaderboard();

	void Write_Leaderboard(const std::string& player_name, const int& player_score);
}

 