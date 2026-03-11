#include "Leaderboard.h"
#include "Enemy.h"
#include "GameStateManager.h"
#include "Game.h"
#include "MeshRenderer.h"
#include "MeshTemplates.h"
#include <iostream>
#include <vector>
#include <fstream>   
#include <algorithm> 
#include <AEEngine.h>




namespace 
{
	float scroll_offset = 0.0f; //scroll initial position
	int highest_score = 0;
	// NEW: How many lines to show on screen at once
	const int ENTRIES_PER_PAGE = 5;
	const int MAX_LEADERBOARD_SIZE = 50;
	float lb_scroll = 0.0f;
	const int LB_VISIBLE = 5;
	AEGfxVertexList* overlayMesh = nullptr;

	std::vector<Leaderboard_Data::ZenEntry> leaderboard_list;
	std::vector<Leaderboard_Data::TimedEntry> s_timedBoard;
}

namespace Leaderboard_System
{
	void Init() {
		if (overlayMesh != nullptr) {
			return; 
		}
		// Create a dedicated 1x1 white mesh just for the leaderboard overlay
		overlayMesh = Rect_Mesh(1.0f, 1.0f, 0xFFFFFFFF);

	}

	void Free() {
		if (overlayMesh) {
			AEGfxMeshFree(overlayMesh);
			overlayMesh = nullptr;
		}
	}

	int GetHighestScore() {
		Read_ZenLeaderboard();
		return highest_score;
	}


	static bool CompareZenScores(const Leaderboard_Data::ZenEntry& a, const Leaderboard_Data::ZenEntry& b) {
		return a.player_score > b.player_score;
	}

	static bool CompareTimedScores(const Leaderboard_Data::TimedEntry& a, const Leaderboard_Data::TimedEntry& b) {
		return a.player_score > b.player_score;
	}

	void Read_ZenLeaderboard() {

		leaderboard_list.clear(); //clean vector for new upcoming data from text file
		const char* leaderboard_filepath = "Assets/leaderboard.txt";
		std::ifstream file_stream(leaderboard_filepath);
		if (!file_stream.is_open()) {
			std::cout << "Unable to open zen leaderboard\n";
			return;
		}
		std::string player_name;
		int player_score;
		while (file_stream >> player_name >> player_score) { //store file data to vector for displaying or writing 
			Leaderboard_Data::ZenEntry newEntry{ player_name, player_score };
			if (player_score > highest_score) highest_score = player_score;
			leaderboard_list.push_back(newEntry);
			std::cout << newEntry.player_name << newEntry.player_score << "\n";
		}

		file_stream.close();
	}

	void Read_TimeLeaderboard() {

		s_timedBoard.clear();                                               // flush previous data before reading
		lb_scroll = 0.0f;
		const char* TIMED_LB_PATH = "Assets/leaderboard_timed.txt";
		std::ifstream f(TIMED_LB_PATH);
		if (!f.is_open()) return;                                           // file may not exist on first run, thats fine
		std::string n; int s;
		while (f >> n >> s)
			s_timedBoard.push_back({ n, s });                               // push each name-score pair into the vector
		f.close();
	}

	void Write_ZenLeaderboard(const std::string& player_name, const int& player_score) {

		Read_ZenLeaderboard();
		Leaderboard_Data::ZenEntry newEntry{ player_name, player_score };
		std::cout << "\n";

		leaderboard_list.push_back(newEntry);

		//sort players from highest to lowest score		
		std::sort(leaderboard_list.begin(), leaderboard_list.end(), CompareZenScores);

		const char* leaderboard_filepath = "Assets/leaderboard.txt";
		std::ofstream file_stream(leaderboard_filepath);

		if (!file_stream.is_open()) {
			std::cout << "Unable to open leaderboard\n";
			return;
		}

		for (const auto& entry : leaderboard_list) { //send new sorted leaderboard data to text file
			file_stream << entry.player_name << " " << entry.player_score << "\n";
		}
		file_stream.close();

	}

	void Write_TimeLeaderboard(const std::string& player_name, const int& player_score) {

		Read_TimeLeaderboard();                                                     // load existing entries first so we dont overwrite them
		s_timedBoard.push_back({ player_name, player_score });                            // add the new entry
		std::sort(s_timedBoard.begin(), s_timedBoard.end(), CompareTimedScores); // sort highest to lowest
		const char* TIMED_LB_PATH = "Assets/leaderboard_timed.txt";
		std::ofstream f(TIMED_LB_PATH);
		if (!f.is_open()) return;                                           // cant write if file fails to open
		for (const auto& e : s_timedBoard)
			f << e.player_name << " " << e.player_score << "\n";                         // write each entry as "name score" on its own line
		f.close();

	}

	void Display_ZenLeaderboard(bool& openLeaderboard) {

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

	void Display_TimeLeaderboard(bool& openLeaderboard) {

		AEGfxSetRenderMode(AE_GFX_RM_COLOR);
		AEGfxSetBlendMode(AE_GFX_BM_BLEND);
		AEGfxSetTransparency(0.85f);                                        // semi-transparent black background
		AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 1.0f);
		Mesh_Renderer(overlayMesh, 0.0f, 0.0f, 1600.0f, 900.0f, 0.0f);
		AEGfxSetTransparency(1.0f);                                         // reset transparency for text

		if (Graphics::font_id < 0) return;

		// ---- Title ----
		const char* title = "TIMED MODE - LEADERBOARD";
		float tw, th;
		AEGfxGetPrintSize(Graphics::font_id, title, 0.7f, &tw, &th);
		AEGfxPrint(Graphics::font_id, title, -tw * 0.5f, 0.75f, 0.7f, 1.0f, 0.85f, 0.2f, 1.0f); // centered, gold color

		// ---- Mouse wheel scrolling ----
		s32 wheel = 0;
		AEInputMouseWheelDelta(&wheel);
		int maxScroll = ((int)s_timedBoard.size() - LB_VISIBLE > 0) ? (int)s_timedBoard.size() - LB_VISIBLE : 0; // max scroll = entries that dont fit
		if (wheel > 0) lb_scroll = (lb_scroll - 1.0f < 0.0f) ? 0.0f : lb_scroll - 1.0f;          // scroll up toward older entries
		if (wheel < 0) lb_scroll = (lb_scroll + 1.0f > (float)maxScroll) ? (float)maxScroll : lb_scroll + 1.0f; // scroll down toward newer

		float startY = 0.5f;
		float spacing = 0.2f;

		for (int i = 0; i < LB_VISIBLE; ++i)
		{
			int idx = (int)lb_scroll + i;
			if (idx >= (int)s_timedBoard.size()) break;                     // stop if past end of list

			const auto& e = s_timedBoard[(size_t)idx];
			std::string line = std::to_string(idx + 1) + ".   " + e.player_name + "   " + std::to_string(e.player_score); // format as "rank. name score"
			float y = startY - (i * spacing);
			AEGfxPrint(Graphics::font_id, line.c_str(), -0.3f, y, 0.9f, 1.0f, 1.0f, 1.0f, 1.0f);
		}

		// ---- Scroll indicators ----
		if (lb_scroll > 0)
			AEGfxPrint(Graphics::font_id, "^", 0.4f, 0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f);        // more entries above
		if ((int)lb_scroll < maxScroll)
			AEGfxPrint(Graphics::font_id, "v", 0.4f, -0.6f, 0.8f, 0.8f, 0.8f, 0.8f, 1.0f);       // more entries below

		// ---- Return prompt ----
		const char* prompt = "Press ESC or Click to Return";
		float pw, ph;
		AEGfxGetPrintSize(Graphics::font_id, prompt, 0.45f, &pw, &ph);
		AEGfxPrint(Graphics::font_id, prompt, -pw * 0.5f, -0.85f, 0.45f, 1.0f, 1.0f, 0.0f, 1.0f); // yellow

		if (AEInputCheckTriggered(AEVK_ESCAPE) || AEInputCheckTriggered(AEVK_LBUTTON))
			openLeaderboard = false;                                        // close leaderboard, return to timeout screen
	}
}

 