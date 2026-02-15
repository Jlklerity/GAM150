#pragma once
#include <string>
#include "Enemy.h"
namespace Leaderboard_Data
{
	struct Entry { 

		Entry(std::string playerName, int playerScore) 
			: player_name{ playerName }, player_score{ playerScore } {
		};

		std::pair<std::string, int> getPlayerData() const { return { player_name, player_score }; };

		std::string player_name; 
		int player_score;
	};
}

namespace Leaderboard_System
{
	//Stores leaderboard scores into txt file
	void Read_Leaderboard();

	void Write_Leaderboard(const std::string& player_name, const int& player_score);

	void Display_Leaderboard();
}

 