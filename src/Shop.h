#pragma once

void display_score();
void shop_init();
void shop_update();
void shop_exit();
const char* shop_get_active_skin_path();

extern int g_shop_skin_id;
extern int g_shop_bg_id;