#pragma once
#include <string>

/*!
@file           MainMenu.cpp
@author         Jeremy Lim
@brief          Handles the main menu state, UI rendering, sub-state transitions, and name entry.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/


std::string main_menu_get_player_name();
void reset_player_name(std::string& reset_name);
void draw_main_menu(float text_ndc_offset);
void main_menu_input();
void main_menu_enter_name(float world_y_offset, float ndc_y_offset, bool allow_input);
void main_menu_init();
void main_menu_update();
void main_menu_exit();