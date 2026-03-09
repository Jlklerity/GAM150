#pragma once
#include <string>

std::string Main_Menu_GetPlayerName();
void ResetPlayerName(std::string& reset_name);
void Draw_Main_Menu(float textNdcOffset);
void Main_Menu_Input();
void Main_Menu_EnterName(float worldYOffset, float ndcYOffset, bool allowInput);
void Main_Menu_Init();
void Main_Menu_Update();
void Main_Menu_Exit();
