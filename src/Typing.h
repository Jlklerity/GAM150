#pragma once
#include "pch.h"

// loads the letter spritesheet and creates the single-tile quad for rendering;
// safe to call on restart - unloads any prior texture/mesh first
void typing_init();

// polls A-Z and BACKSPACE each frame, pushes/pops from the input buffer,
// advances the cursor blink timer, and counts down the wrong-word shake timer
void typing_update();

// renders the current input buffer at the bottom-centre of the screen with a
// blinking cursor; shakes the text on a wrong submission for s_shakeTextDuration
void typing_render();

// frees the letter quad mesh and unloads the spritesheet texture
void typing_exit();

// forward declare to avoid heavy includes if you want


namespace Data { struct Enemy; }
namespace Data { struct Score; }
namespace Data { struct Health; }

namespace UiSystem
{
    // renders the current score at top-left; white at night, black during the day
    void ui_draw_score_and_typed(const Data::Score& score);
}

// central per-frame typing loop - locks a target by prefix match, handles ENTER
// submission for normal/friendly/shielded enemies, triggers shake on wrong input,
// and always clears the buffer and target lock after submission
void typing_update_and_apply_to_enemies(std::vector<Data::Enemy>& enemy_pool, Data::Score& score, Data::Health& health);

// renders every active enemy's word above its sprite with a green typed prefix
// and a day/night-lerped untyped suffix; supports orbit word animation
void typing_draw_enemy_words(const std::vector<Data::Enemy>& enemy_pool);