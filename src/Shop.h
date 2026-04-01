#pragma once

/*!
@file           Shop.h
@author         Jeremy Lim
@brief          Handles the in-game shop logic, UI rendering, and player transactions.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
void display_score();
void shop_init();
void shop_update();
void shop_exit();
const char* shop_get_active_skin_path();

extern int g_shop_skin_id;
extern int g_shop_bg_id;