#pragma once
#include <vector>

void Typing_Init();
void Typing_Update();
void Typing_Render();
void Typing_Exit();

// forward declare to avoid heavy includes if you want


namespace Data { struct Enemy; }
namespace Data { struct Score; }

void Typing_UpdateAndApplyToEnemies(std::vector<Data::Enemy>& enemy_pool, Data::Score& score);

// (optional but useful)
const char* Typing_GetText();
void Typing_Clear();