#pragma once

void DisplayScore();
void Shop_Init();
void Shop_Update();
void Shop_Exit();
const char* Shop_GetActiveSkinPath();

extern int g_shop_skin_id;
extern int g_shop_bg_id;