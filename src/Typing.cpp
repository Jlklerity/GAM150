// File: Typing.cpp
// Author: Pengfei, Zi Yuan, Azfar
#include "pch.h"
#include "Typing.h"


// returns the display form of word - unchanged in normal mode, fully reversed
// during Trick Room so the player must type the last letter first;
// called by targeting and draw code to ensure both always agree on the same string
static std::string tr_display_word(const std::string& word)
{
    if (!BossSystem::boss_is_trick_room_active()) return word;   // normal mode: word unchanged
    std::string rev = word;
    std::reverse(rev.begin(), rev.end());                     // Trick Room: reverse every character
    return rev;
}
/*________________________________________________________________________________________________*/

namespace Utility
{
    // returns true if typed is a prefix of text - compares char by char and
    // rejects immediately if typed is longer than text; used to match partial
    // input against an enemy's display word during target locking
    bool static String_StartsWith(const std::string& text, const std::string& typed)
    {
        if (typed.size() > text.size()) return false;// reject if prefix is longer than text
        for (size_t i = 0; i < typed.size(); ++i)//loop over prefix letters
        {
            if (text[i] != typed[i]) return false; // rej if dun match                                                                       
        }
        return true;  //all match
    }


}

namespace EnemySystem
{

    // scans the enemy pool and returns the index of the best target for the current
    // typed prefix - a valid target must be active, not dying, and have a display word
    // that starts with typed; among all matches the enemy closest to the ground
    // (lowest enemy_position_y) wins, so the most dangerous enemy is always prioritised;
    // returns -1 if nothing typed or no matching enemy exists
    int static Enemy_FindBestTargetByPrefix(const std::vector<Data::Enemy>& pool, const std::string& typed)
    {
        if (typed.empty()) return -1; // no target if nothing typed

        int best_idx = -1; // init best target as -1 which is nth
        float best_y = 9999999.0f; //tracks how close the enemy is to landing, a smaller y = closer, init to a v large value so first valid enemy alwasy wins

        for (int i = 0; i < (int)pool.size(); ++i)  // loop through enemy pool
        {
            const auto& e = pool[(size_t)i]; // read enemy data
            if (!e.enemy_is_active) continue;  // skip inactive enemies
            if (e.enemy_is_dying) continue;    // skip dying enemies

            if (Utility::String_StartsWith(tr_display_word(e.enemy_word_text), typed)) // check typing match against display word (reversed during Trick Room)
            {
                if (e.enemy_position_y < best_y) // if enemy pos is smaller than besty (meaning its the closest to falling)
                {
                    best_y = e.enemy_position_y;     // store new best y
                    best_idx = i;                   // store new best index
                }
            }
        }
        return best_idx;   // return best index or -1
    }
}

namespace UiSystem
{
    // renders the current score at the top-left of the screen - draws white text
    // at night and black text during the day by querying DayCycle::is_night();
    // skips rendering entirely if the font has not been loaded (g_font_id < 0)
    void ui_draw_score_and_typed(const Data::Score& score)
    {
        if (Graphics::g_font_id < 0) return;     // skip if font missing

        char s[128];      // text buffer for score
        std::snprintf(s, sizeof(s), "SCORE: %d", score.score_value);    // format score line

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);  // enable blending for text


        if (DayCycle::is_night())
            AEGfxPrint(Graphics::g_font_id, s, -0.95f, 0.90f, 0.55f, 1.0f, 1.0f, 1.0f, 1.0f); // white at night
        else
            AEGfxPrint(Graphics::g_font_id, s, -0.95f, 0.90f, 0.55f, 0.0f, 0.0f, 0.0f, 1.0f); // draw score at top-left-ish
    }
}
// ======================================================






namespace
{
    // for debug
    static bool s_debugSubmitted = false; // check for submission
    static bool s_debugCorrect = false; // check if submission correct
    static std::string s_debugTyped; // str for whats s_typed
    static std::string s_debugTarget; // str for correct ans
    //


    static int s_targetIndex = -1;

    //cursor
    static float s_cursorTimer = 0.0f;
    static bool  s_cursorVisible = true;

    //shake anim
    static float s_shakeStrength = 4.0f;  // pixels of offset
    static std::string s_shakeText;
    static float s_shakeTextTime = 0.0f;
    static const float s_shakeTextDuration = 0.15f;


    AEGfxTexture* s_lettersTex = nullptr; // texture for letters spritesheet
    AEGfxVertexList* s_letterQuad = nullptr; //texture for single letter 

    const int COLS{ 8 }; //sheet grid cols
    const int ROWS{ 4 }; //sheet grid rows
    const float UV_W{ 1.0f / COLS }; //uv width per letter
    const float UV_H{ 1.0f / ROWS }; //uv height per letter

    //const char* target{ "BANANA" }; // target word, change to enemys one later
    char s_typed[32]{}; // word s_typed enters this array
    int s_typedLen{}; // length of s_typed word


    bool s_prevKey[256]{}; // stores last frames key state, basically makes sure u cannot hold the key down, is an array cuz one for every key


    // maps a lowercase letter to its uppercase equivalent; passes non-alpha chars through unchanged
    static char ToUpper(char c)
    {
        if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
        return c;
    }

    // returns true only on the first frame a key transitions from up to down -
    // maintains a persistent 512-entry bool array (maxKey) so holding a key
    // never repeats; prevents the player from spamming a single letter by holding
    static bool key_pressed_once(int key)
    {
        static bool max_key[512] = {}; // array of 512 bools js to cover all of alpha engine keycode idk dun ask me headache to get rid of one warning

        if (key < 0 || key >= 512) return false; // rej if oob


        bool curr = (AEInputCheckCurr(static_cast<u8>(key)) != 0); // basically curr == true means key held down, false = not held down
        bool once = curr && !max_key[key]; // checks if its pressed once
        max_key[key] = curr; // save current key state to be used in next frame again
        return once;
    }



    // converts a letter character to its 0-based index into the spritesheet (A=0 � Z=25);
    // returns -1 for any non-alpha character so the draw loop can safely skip it
    static int char_to_index(char c)
    {
        c = (char)toupper((unsigned char)c);;// convert to upper
        if (c < 'A' || c > 'Z') return -1; //if not A-Z, return -1 so draw loop cannot draw 
        return (c - 'A'); // return the index of letters for drawing of sprite
    }

    // uppercases c and appends it to s_typed if it is a letter (A-Z) and the
    // buffer has room (max 31 chars + null terminator); always null-terminates
    static void push_char(char c)
    {
        c = ToUpper(c); //conver to upper
        if (c < 'A' || c > 'Z') return; // reject nonletter

        if (s_typedLen < (int)sizeof(s_typed) - 1) // max typable chars is 31 cuz null terminator so this caps our typing at 31 chars
        {
            s_typed[s_typedLen++] = c; // adds char into array and increment length for next one
            s_typed[s_typedLen] = '\0';// end w null terminator

            std::cout << "Typed: " << s_typed << std::endl; //print what we s_typed so far
        }
    }

    // removes the last character from s_typed by decrementing s_typedLen and
    // writing a null terminator - does nothing if the buffer is already empty
    static void pop_char()
    {
        if (s_typedLen > 0) // check if got anyt to pop
        {
            s_typed[--s_typedLen] = '\0'; // decrement length and set new end to null terminator
            std::cout << "Typed: " << s_typed << std::endl; // print 
        }
    }



    // resets s_typedLen to 0 and null-terminates s_typed, effectively emptying the input buffer
    static void clear_typed()
    {
        s_typedLen = 0;
        s_typed[0] = '\0'; //set first char to null terminator
    }

    // renders word left-to-right starting at (x, y) - samples each letter's UV tile
    // from the spritesheet using CharToIndex, advances the x cursor by (w + spacing)
    // per character, and skips any character that has no valid spritesheet index
    static void draw_word(const char* word, float x, float y, float w, float h, float spacing)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f); //visible jic

        for (int i = 0; word[i] != '\0'; ++i) //loop thru chars in word
        {
            int idx = char_to_index(word[i]); //convert to index
            if (idx < 0) continue; // reject non letter

            int col = idx % COLS; // calc col n row in sheet 
            int row = idx / COLS;

            float u = col * UV_W;
            float v = row * UV_H;

            AEGfxTextureSet(s_lettersTex, u, v);

            float px = x + i * (w + spacing); // calc next letter pos
            mesh_renderer(s_letterQuad, px, y, w, h, 0.0f);
        }
    }

    // computes the total pixel width of word (letters + inter-letter spacing) then
    // shifts the start x so the word is horizontally centred on centerX, then delegates to DrawWord
    static void draw_word_centered(const char* word, float centerX, float y,
        float w, float h, float spacing)
    {
        int len = (int)strlen(word); // count chars not including nullt
        if (len <= 0) return; // nth to draw do nth

        float total_width = (len * w) + ((len - 1) * spacing); // calc total width of word
        float start_x = centerX - total_width * 0.5f;// calc start x pos to center word

        draw_word(word, start_x, y, w, h, spacing);// draw
    }


    // draws a thin blinking cursor rectangle immediately after the last letter of word,
    // using the same centering math as DrawWordCentered so the cursor always aligns
    // with the typed input display at the bottom of the screen
    static void DrawCursorAfterWordCentered(const char* word, float centerX, float y, float w, float h, float spacing) {
        int len = (int)strlen(word);

        // total width of letters + spacing
        float total_width = (len * w) + ((len > 0) ? (len - 1) * spacing : 0.0f);

        // start x that DrawWordCentered uses
        float start_x = centerX - total_width * 0.5f;

        // cursor x position = after last character
        float cursor_x = start_x + len * (w + spacing);

        // Draw a thin rectangle as cursor
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // make cursor thin
        float cw = 8.0f;
        float ch = h * 0.7f;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);


        AEGfxSetBlendColor(1.0f, 1.0f, 1.0f, 1.0f);


        mesh_renderer(s_letterQuad, cursor_x, y, cw, ch, 0.0f);
    }

    // returns a read-only pointer to the current contents of s_typed
    const char* typing_get_text()
    {
        return s_typed; //return s_typed word
    }

    // clears the input buffer - thin public wrapper around ClearTyped for external callers
    void typing_clear()
    {
        clear_typed(); //clear 
    }



}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////namespace end

// loads the letter spritesheet and creates the single-tile quad for rendering;
// unloads any previously loaded texture/mesh first so it is safe to call on restart;
// clears the input buffer and resets s_prevKey so no held keys bleed in from a prior state
void typing_init()
{
    if (s_lettersTex) { AEGfxTextureUnload(s_lettersTex); s_lettersTex = nullptr; }
    if (s_letterQuad) { AEGfxMeshFree(s_letterQuad);      s_letterQuad = nullptr; }

    s_lettersTex = AEGfxTextureLoad("Assets/lettersheet-Sheet.png");
    AE_ASSERT_MESG(s_lettersTex, "Failed to load letters spritesheet!");

    s_letterQuad = Sprite_Mesh(UV_W, UV_H);

    clear_typed();
    for (int i = 0; i < 256; ++i) s_prevKey[i] = false;
}


// polls all letter keys (A-Z) and BACKSPACE each frame via KeyPressedOnce to
// prevent hold-spam, pushing or popping from s_typed accordingly and playing a
// typing sound effect on each keystroke; also advances the cursor blink timer
// and counts down s_shakeTextTime, clearing the shake word when it expires
void typing_update()
{
    for (int k = AEVK_A; k <= AEVK_Z; ++k) // for all letters
    {
        if (key_pressed_once(k)) // if key pressed (not held)
        {
            char c = (char)('A' + (k - AEVK_A));// convert key code to actual char
            push_char(c); // push to s_typed array
            SoundManager::play_typing_sfx();
        }
    }

    if (key_pressed_once(AEVK_BACK)) {
        pop_char(); // backspace
        SoundManager::play_typing_sfx();
    }

    float dt = (float)AEFrameRateControllerGetFrameTime();
    s_cursorTimer += dt; // add dt for cursor timer

    if (s_cursorTimer >= 0.5f) // blink every 0.5s
    {
        s_cursorTimer = 0.0f; // reset
        s_cursorVisible = !s_cursorVisible;
    }

    // update shake text timer
    if (s_shakeTextTime > 0.0f) // check if shake animation is active
    {
        s_shakeTextTime -= dt; //countdown remaining shake duration
        if (s_shakeTextTime <= 0.0f) // check if shake fin
        {
            s_shakeTextTime = 0.0f; // reset shake itmer
            s_shakeText.clear();   // remove shaken word after duration
        }
    }

}


// renders the player's current input at the bottom-centre of the screen - if a
// wrong-word shake is active, draws s_shakeText with a random pixel offset each
// frame for the shake duration; otherwise draws s_typed normally; appends a
// blinking cursor rect after the last letter; in _DEBUG builds also prints
// submit results (correct/wrong word and target) to stdout once per submission
void typing_render()
{
    float dx = 0.0f, dy = 0.0f;

    if (s_shakeTextTime > 0.0f && !s_shakeText.empty()) // if shake timer is active and theres text to shake
    {
        dx = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * s_shakeStrength; // (float)rand() / RAND_MAX generates random float 0 to 1, *2 stretches the range, -1 recenters the range around zero
        dy = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * s_shakeStrength;

        draw_word_centered(s_shakeText.c_str(), 0.0f + dx, -350.0f + dy, 60.0f, 60.0f, 8.0f);
    }
    else
    {
        draw_word_centered(s_typed, 0.0f, -350.0f, 60.0f, 60.0f, 8.0f);
    }


    if (s_cursorVisible)
    {
        DrawCursorAfterWordCentered(s_typed, 0.0f + dx, -350.0f + dy, 60.0f, 60.0f, 8.0f);
    }
#ifdef _DEBUG                                                   // debug chunk
    static bool printed = false;
    if (!printed)
    {
        std::cout << "Typing_Render called" << std::endl;
        printed = true;
    }

    if (s_debugSubmitted)
    {
        std::cout << "SUBMIT: " << s_debugTyped << std::endl;

        if (s_debugCorrect)
        {
            std::cout << "RESULT: CORRECT" << std::endl;
            std::cout << "TARGET WORD: " << s_debugTarget << std::endl;
        }
        else
        {
            std::cout << "RESULT: WRONG" << std::endl;
            if (!s_debugTarget.empty())
                std::cout << "TARGET WORD: " << s_debugTarget << std::endl;
            else
                std::cout << "TARGET WORD: <none>" << std::endl;
        }

        std::cout << "------------------------" << std::endl;

        s_debugSubmitted = false; // print once per submit
    }
#endif
}

// frees the letter quad mesh and unloads the spritesheet texture, nulling both
// pointers so re-entry via Typing_Init is safe
void typing_exit()
{
    if (s_letterQuad) { AEGfxMeshFree(s_letterQuad); s_letterQuad = nullptr; } //freeee
    if (s_lettersTex) { AEGfxTextureUnload(s_lettersTex); s_lettersTex = nullptr; }// freeeeeee
}

// renders every active, non-dying enemy's word above its sprite using AEGfxPrint -
// the targeted enemy's word is split at the typed prefix: already-typed letters
// draw green, remaining letters draw black (day) or white (night) via DayCycle lerp;
// all non-targeted enemies draw fully in the day/night untyped colour;
// letters with the orbit effect enabled are smoothly blended from their baseline
// position outward to full orbit radius so the word visibly circles the sprite
void typing_draw_enemy_words(const std::vector<Data::Enemy>& enemy_pool)
{
    if (Graphics::g_font_id < 0) return;                                                                                      // skip if font missing

    // Lerp untyped word color from black (day) to white (night)
    float nightA = DayCycle::get_night_alpha();
    float untypedR = nightA;
    float untypedG = nightA;
    float untypedB = nightA;

    for (int i = 0; i < (int)enemy_pool.size(); ++i)                                                                        // loop through all enemies by index so we can compare against s_targetIndex
    {
        const Data::Enemy& e = enemy_pool[(size_t)i];                                                                       // read enemy data

        if (!e.enemy_is_active) continue;                                                                                   // skip inactive enemies
        if (e.enemy_word_text.empty()) continue;                                                                            // skip if no text
        if (e.enemy_is_dying) continue;                                                                                     // skip during death animation so word disappears when enemy is killed

        // Build the display word: reversed during Trick Room so the player types last-letter-first
        std::string display_word = tr_display_word(e.enemy_word_text);                                                       // normal or reversed depending on Trick Room state

        float ndc_x = 0.0f, ndc_y = 0.0f;
        Utility::convert_world_to_ndc(e.enemy_position_x, e.enemy_position_y, ndc_x, ndc_y);                                  // convert enemy world pos to NDC for text placement

        float text_w = 0.0f, text_h = 0.0f;
        AEGfxGetPrintSize(Graphics::g_font_id, display_word.c_str(), EnemyConfig::TEXT_SCALE, &text_w, &text_h);              // measure full display word size for centering

        float half_win_h = AEGfxGetWinMaxY();
        float label_offset_ndc = (half_win_h > 0.0f) ? (EnemyConfig::WORD_LABEL_OFFSET_PX / half_win_h) : 0.0f;

        float draw_x = ndc_x - (text_w * 0.5f);                                                                             // center text horizontally
        float draw_y = ndc_y + label_offset_ndc;                                                                            // position text above enemy

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // enable blending for text

        int word_len = (int)display_word.size();                                                                            // number of letters in the display word
        float cursor_x = draw_x;                                                                                            // tracks baseline x position as we step through letters

        if (i == s_targetIndex && s_typedLen > 0)                                                                             // this enemy is targeted and player has s_typed something
        {
            int split = (s_typedLen < word_len) ? s_typedLen : word_len;                                                        // clamp split index to word length

            for (int li = 0; li < word_len; ++li)                                                                           // loop through every letter
            {
                char single[2] = { display_word[(size_t)li], '\0' };                                                        // isolate this letter from the display word (reversed during Trick Room)
                float lw = 0.0f, lh = 0.0f;
                AEGfxGetPrintSize(Graphics::g_font_id, single, EnemyConfig::TEXT_SCALE, &lw, &lh);                            // measure letter width

                float final_x = cursor_x;                                                                                   // default to baseline x
                float final_y = draw_y;                                                                                     // default to baseline y

                if (e.enemy_has_orbit_effect && e.enemy_orbit_radius > 0.001f && !e.enemy_has_landed)                       // only apply orbit if assigned, radius is meaningful, and enemy is still falling
                {
                    float angle = e.enemy_orbit_timer * EnemyConfig::ORBIT_SPEED                                            // orbit angle advances over time
                        + (float)li * (6.2831853f / (float)word_len);                                                       // each letter evenly spaced around the circle

                    float orbit_x = ndc_x + cosf(angle) * e.enemy_orbit_radius;                                             // orbit x around enemy center
                    float orbit_y = ndc_y + sinf(angle) * e.enemy_orbit_radius;                                             // orbit y around enemy center

                    float t = e.enemy_orbit_radius / EnemyConfig::ORBIT_RADIUS_MAX_NDC;                                     // blend factor 0 at baseline, 1 at full orbit
                    final_x = cursor_x + (orbit_x - cursor_x) * t;                                                          // blend baseline to orbit x
                    final_y = draw_y + (orbit_y - draw_y) * t;                                                              // blend baseline to orbit y
                }

                if (li < split)                                                                                             // s_typed portion, draw green
                    AEGfxPrint(Graphics::g_font_id, single, final_x, final_y, EnemyConfig::TEXT_SCALE, 0.1f, 0.65f, 0.1f, 1);
                else                                                                                                        // untyped portion, lerp black->white at night
                    AEGfxPrint(Graphics::g_font_id, single, final_x, final_y, EnemyConfig::TEXT_SCALE, untypedR, untypedG, untypedB, 1);

                cursor_x += lw;                                                                                             // advance baseline cursor
            }
        }
        else                                                                                                                // not targeted, draw all letters in black
        {
            for (int li = 0; li < word_len; ++li)                                                                           // loop through every letter
            {
                char single[2] = { display_word[(size_t)li], '\0' };                                                        // isolate this letter from the display word (reversed during Trick Room)
                float lw = 0.0f, lh = 0.0f;
                AEGfxGetPrintSize(Graphics::g_font_id, single, EnemyConfig::TEXT_SCALE, &lw, &lh);                            // measure letter width

                float final_x = cursor_x;                                                                                   // default to baseline x
                float final_y = draw_y;                                                                                     // default to baseline y

                if (e.enemy_has_orbit_effect && e.enemy_orbit_radius > 0.001f && !e.enemy_has_landed)                       // only apply orbit if assigned, radius is meaningful, and enemy is still falling
                {
                    float angle = e.enemy_orbit_timer * EnemyConfig::ORBIT_SPEED                                            // orbit angle advances over time
                        + (float)li * (6.2831853f / (float)word_len);                                                       // each letter evenly spaced around the circle

                    float orbit_x = ndc_x + cosf(angle) * e.enemy_orbit_radius;                                             // orbit x around enemy center
                    float orbit_y = ndc_y + sinf(angle) * e.enemy_orbit_radius;                                             // orbit y around enemy center

                    float t = e.enemy_orbit_radius / EnemyConfig::ORBIT_RADIUS_MAX_NDC;                                     // blend factor 0 at baseline, 1 at full orbit
                    final_x = cursor_x + (orbit_x - cursor_x) * t;                                                          // blend baseline to orbit x
                    final_y = draw_y + (orbit_y - draw_y) * t;                                                              // blend baseline to orbit y
                }

                AEGfxPrint(Graphics::g_font_id, single, final_x, final_y, EnemyConfig::TEXT_SCALE, untypedR, untypedG, untypedB, 1);               // draw letter lerped black->white at night
                cursor_x += lw;                                                                                             // advance baseline cursor
            }
        }
    }
}









// central typing game loop called every frame - drives Typing_Update to collect
// input, then uses Enemy_FindBestTargetByPrefix to lock onto the closest matching
// enemy; clears the lock if the typed buffer no longer matches the target's display
// word; on ENTER checks for an exact match and dispatches the correct kill path:
// FRIENDLY enemies trigger a heal + laser + streak update, SHIELDED enemies with
// an intact shield call Enemy_BreakShield and spawn a shield-break particle,
// all other enemies are killed normally with a laser + score + streak update;
// a wrong submission stores the typed text as a shake animation and resets the
// streak; always clears s_typed and the target lock after any submission
void typing_update_and_apply_to_enemies(std::vector<Data::Enemy>& enemy_pool, Data::Score& score, Data::Health& health) {


    typing_update(); //call typing update logic, basicallly pushing whatever is s_typed to the buffer
    bool submitted = key_pressed_once(AEVK_RETURN);    // check for enter

    std::string typed_buffer = s_typed; // convert from char array to std::string

    if (typed_buffer.empty()) //if nothing s_typed, unlock target and return
    {
        s_targetIndex = -1;
        for (int i = 0; i < (int)enemy_pool.size(); ++i)
            enemy_pool[(size_t)i].enemy_is_targeted = false; // no target, clear all targeted flags
        return;
    }


    if (s_targetIndex >= 0) // means have locked enemy target
        //if index out of range OR enemy no longer active OR enemy_is_dying OR the s_typed letters no longer match the start of the display word, target lock clear (s_targetIndex =-1)
        if (s_targetIndex >= (int)enemy_pool.size() || !enemy_pool[(size_t)s_targetIndex].enemy_is_active || enemy_pool[(size_t)s_targetIndex].enemy_is_dying || !Utility::String_StartsWith(tr_display_word(enemy_pool[(size_t)s_targetIndex].enemy_word_text), typed_buffer)) s_targetIndex = -1;

    // if no target locked try to find one
    if (s_targetIndex < 0) s_targetIndex = EnemySystem::Enemy_FindBestTargetByPrefix(enemy_pool, typed_buffer);

    for (int i = 0; i < (int)enemy_pool.size(); ++i)
        enemy_pool[(size_t)i].enemy_is_targeted = (i == s_targetIndex);  // mark only the current target as targeted

    // if player pressed enter, always clear (and only kill if locked + exact match)
    if (submitted)
    {
        //debug chunk
        s_debugSubmitted = true;
        s_debugTyped = typed_buffer;
        s_debugCorrect = false;
        s_debugTarget.clear();
        //

        //if (s_targetIndex < 0)  s_targetIndex = EnemySystem::Enemy_FindBestTargetByPrefix(enemy_pool, typed_buffer);
        bool correct = false; //for shake

        if (s_targetIndex >= 0) // if have target
        {
            Data::Enemy& target = enemy_pool[(size_t)s_targetIndex];
            s_debugTarget = target.enemy_word_text; //store its word for debug

            //if enemy valid AND s_typed word matches including the reversed word during Trick Room)
            if (target.enemy_is_active && !target.enemy_is_dying && tr_display_word(target.enemy_word_text) == typed_buffer)
            {
                // -------------------------
                // FRIENDLY NPC: heal 10 hp
                // -------------------------
                if (target.enemy_type == Data::EnemyType::FRIENDLY)
                {
                    laser_beam_fire(target.enemy_position_x, target.enemy_position_y); // fire laser to enemy
                    EnemySystem::enemy_killed(target, health);                                                  // kill friendly
                    particles_spawn_friendly_heal(target.enemy_position_x, target.enemy_position_y);             // play heal sprite effect
                    score.score_value += (int)(target.enemy_word_length * 10 * StreakSystem::get_multiplier());
                    if (health.health_current < 0) health.health_current = 0;                                   // clamp
                    StreakSystem::on_word_correct_at(target.enemy_position_x, target.enemy_position_y);
                }
                // -------------------------
                // SHIELDED NPC: first word breaks shield, second word kills
                // -------------------------
                else if (target.enemy_type == Data::EnemyType::SHIELDED && target.enemy_shield_intact)
                {
                    EnemySystem::enemy_break_shield(target);                                                     // row 0 to row 1, swap to word2
                    particles_spawn_shield_break(target.enemy_position_x, target.enemy_position_y);               // play shield break sprite effect
                    score.score_value += (int)(target.enemy_word_length * 10 * StreakSystem::get_multiplier());

                    SoundManager::play_enemy_death_sfx();
                }
                // -------------------------
                // NORMAL enemy OR shielded after shield broken: kill + gain score
                // -------------------------
                else
                {
                    laser_beam_fire(target.enemy_position_x, target.enemy_position_y); // fire laser to enemy
                    EnemySystem::enemy_killed(target, health);                                                  // kill
                    score.score_value += (int)(target.enemy_word_length * 10 * StreakSystem::get_multiplier());
                    StreakSystem::on_word_correct_at(target.enemy_position_x, target.enemy_position_y);

                    SoundManager::play_enemy_death_sfx();
                }

                s_debugCorrect = true;
                correct = true; // no shake
            }
        }

        if (!correct) //if not correct
        {
            s_shakeText = typed_buffer;           // store what is s_typed
            s_shakeTextTime = s_shakeTextDuration;
            StreakSystem::on_streak_reset();
        }



        clear_typed();  //clear
        s_targetIndex = -1; //no target lock
        return;
    }


    // No target locked yet -> allow free typing (dont clear)
    if (s_targetIndex < 0)
    {
        return;
    }

}