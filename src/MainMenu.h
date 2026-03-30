#pragma once
#include <string>

std::string Main_Menu_GetPlayerName();
void ResetPlayerName(std::string& reset_name);
void Draw_Main_Menu(float text_ndc_offset);
void Main_Menu_Input();
void Main_Menu_EnterName(float world_y_offset, float ndc_y_offset, bool allow_input);
void Main_Menu_Init();
void Main_Menu_Update();
void Main_Menu_Exit();