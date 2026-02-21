#include "Leaderboard.h"
#include "Enemy.h"
#include "GameStateManager.h"
#include "Game.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include <iostream>
#include <vector>
#include <fstream>   // Required for file I/O
#include <algorithm> // Required for std::sort
#include "AEEngine.h"




namespace 
{
	float scroll_offset = 0.0f; //scroll initial position

	// NEW: How many lines to show on screen at once
	const int ENTRIES_PER_PAGE = 5;
	const int MAX_LEADERBOARD_SIZE = 50;
	AEGfxVertexList* overlayMesh = nullptr;
}

namespace Leaderboard_System 
{
	void Init() {
		// Create a dedicated 1x1 white mesh just for the leaderboard overlay
		overlayMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);
	}

	void Free() {
		if (overlayMesh) {
			AEGfxMeshFree(overlayMesh);
			overlayMesh = nullptr;
		}
	}

	std::vector<Leaderboard_Data::Entry> leaderboard_list;

	static bool CompareScores(const Leaderboard_Data::Entry& a, const Leaderboard_Data::Entry& b) {
		return a.player_score > b.player_score;
	}

	void Read_Leaderboard() {

		leaderboard_list.clear(); //clean vector for new upcoming data from text file
		const char* leaderboard_filepath = "Assets/leaderboard.txt";                                                                             
		std::ifstream file_stream(leaderboard_filepath);                                                                                
		if (!file_stream.is_open()) {
			std::cout << "Unable to open leaderboard\n";
			return;
		}
		std::string player_name;
		int player_score;
		while (file_stream >> player_name >> player_score) { //store file data to vector for displaying or writing 
			Leaderboard_Data::Entry newEntry{ player_name, player_score };
			leaderboard_list.push_back(newEntry);
			std::cout << newEntry.player_name << newEntry.player_score << "\n";
		}
		file_stream.close();
	}

	void Write_Leaderboard(const std::string& player_name, const int& player_score) {
		
		Read_Leaderboard();
		Leaderboard_Data::Entry newEntry{ player_name, player_score };
		std::cout << "\n";
		
		leaderboard_list.push_back(newEntry);
		 
		//sort players from highest to lowest score		
		std::sort(leaderboard_list.begin(), leaderboard_list.end(), CompareScores);
		const char* leaderboard_filepath = "Assets/leaderboard.txt";
		std::ofstream file_stream(leaderboard_filepath);

		if (!file_stream.is_open()) {
			std::cout << "Unable to open leaderboard\n";
			return;
		}

		for (const auto& entry : leaderboard_list) { //send new sorted leaderboard data to text file
			file_stream << entry.player_name << " " <<entry.player_score << "\n";
		}
		file_stream.close();

	}

	void Display_Leaderboard(bool& openLeaderboard) {
		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(0.85f);
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);

		Mesh_Renderer(overlayMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);

		AEGfxSetTransparency(1.0f); // Reset transparency for text

		// 3. Draw a prompt so they know how to leave
		if (Graphics::font_id >= 0) {
			const char* promptText = "Press ESC or Click to Return";
			float pw, ph;
			AEGfxGetPrintSize(Graphics::font_id, promptText, 0.5f, &pw, &ph);
			AEGfxPrint(Graphics::font_id, promptText, -pw * 0.5f, -0.8f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f);
		}

		// 4. Return to the Game Over buttons if they click or hit Escape
		if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON)) {
			openLeaderboard = false;
		}
		if (Graphics::font_id < 0) return;

		//MOUSE WHEEL settings
		s32 wheel_delta = 0;
		AEInputMouseWheelDelta(&wheel_delta); // Get scroll amount

		if (wheel_delta != 0) {
			if (wheel_delta > 0) scroll_offset--;
			else scroll_offset++;
		}

		u64 max_offset = leaderboard_list.size() - ENTRIES_PER_PAGE;
		scroll_offset = Utility::Utility_Clamp(scroll_offset, 0, (float)max_offset);


		//RENDER LEADERBOARD
		float startY = 150.0f;
		float spacing = 50.0f;

		for (int i = 0; i < ENTRIES_PER_PAGE; ++i) { //display leaderboard starting from highest to lowest score

			int data_index = (int)scroll_offset + i;
			if (data_index >= leaderboard_list.size()) break;

			const auto& entry = leaderboard_list[data_index];

			std::string rank = std::to_string(data_index + 1) + ".   ";
			std::string player_name = entry.player_name;
			std::string score = std::to_string(entry.player_score);

			std::string player_stats = rank + player_name + "   " + score;

			float y = startY - (i * spacing);
			AEGfxPrint(Graphics::font_id, player_stats.c_str(), -0.3f, y / 450.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		//Scroll indicator
		if (scroll_offset > 0) {
			AEGfxPrint(Graphics::font_id, "^", 0.4f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		if (scroll_offset < max_offset) {
			AEGfxPrint(Graphics::font_id, "v", 0.4f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	
}

 