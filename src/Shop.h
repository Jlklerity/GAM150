#pragma once

void DisplayScore();
void Shop_Init();
void Shop_Update();
void Shop_Exit();
const char* Shop_GetActiveSkinPath();

extern int gShopSkinID; // 0 = Default, 1 = Alt
extern int gShopBgID;   // 0 = Default, 1 = Alt