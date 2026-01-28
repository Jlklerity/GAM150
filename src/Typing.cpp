#include "AEEngine.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include <cstring>
#include "Typing.h"
#include <iostream>
#include "Enemy.h"




namespace Utility
{
    //  check if what we type matches enemy, params are (string we are checking (enemy text), typed string we want to match to the front)
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

    // basically chooses which enemy should be targeted based on what the player has typed so far,and how close the enemy is to the ground, params are (pool of all enemies in the game, what we typed so far)
    // this function also targets the most dangerous enemy IF 2 words have the same text, unlikely but jic
    //return value >= 0 is the index of chosen enemy, if return -1 means no valid target
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

            if (Utility::String_StartsWith(e.enemy_word_text, typed)) // check typing match
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
    void Ui_DrawScoreAndTyped(const Data::Score& score)
    {
        if (Graphics::font_id < 0) return;     // skip if font missing

        char s[128];      // text buffer for score
        std::snprintf(s, sizeof(s), "SCORE: %d", score.score_value);    // format score line

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);  // enable blending for text
        AEGfxPrint(Graphics::font_id, s, -0.95f, 0.90f, 0.55f, 1, 1, 1, 1); // draw score at top-left-ish

    }
}
// ======================================================






namespace
{

    // for debug
    static bool s_debugSubmitted = false; // check for submission
    static bool s_debugCorrect = false; // check if submission correct
    static std::string s_debugTyped; // str for whats typed
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


    AEGfxTexture* lettersTex = nullptr; // texture for letters spritesheet
    AEGfxVertexList* letterQuad = nullptr; //texture for single letter 

    const int COLS{ 8 }; //sheet grid cols
    const int ROWS{ 4 }; //sheet grid rows
    const float UV_W{ 1.0f / COLS }; //uv width per letter
    const float UV_H{ 1.0f / ROWS }; //uv height per letter

    //const char* target{ "BANANA" }; // target word, change to enemys one later
    char typed[32]{}; // word typed enters this array
    int typedLen{}; // length of typed word


    bool prevKey[256]{}; // stores last frames key state, basically makes sure u cannot hold the key down, is an array cuz one for every key


    // converts lowercase to uppercase
    static char ToUpper(char c)
    {
        if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
        return c;
    }

    // make sure key is pressed once per frame so player cannot hold n spam one letter
    static bool KeyPressedOnce(int key)
    {
        static bool maxKey[512] = {}; // array of 512 bools js to cover all of alpha engine keycode idk dun ask me headache to get rid of one warning

        if (key < 0 || key >= 512) return false; // rej if oob

      
        bool curr = (AEInputCheckCurr(static_cast<u8>(key)) != 0); // basically curr == true means key held down, false = not held down
        bool once = curr && !maxKey[key]; // checks if its pressed once
        maxKey[key] = curr; // save current key state to be used in next frame again
        return once;
    }



    //convert char to index for sprite sheet
    static int CharToIndex(char c)
    {
        c = (char)toupper((unsigned char)c);;// convert to upper
        if (c < 'A' || c > 'Z') return -1; //if not A-Z, return -1 so draw loop cannot draw 
        return (c - 'A'); // return the index of letters for drawing of sprite
    }

    // push char to typed array as str (typedLen = last char in array)
    static void PushChar(char c)
    {
        c = ToUpper(c); //conver to upper
        if (c < 'A' || c > 'Z') return; // reject nonletter

        if (typedLen < (int)sizeof(typed) - 1) // max typable chars is 31 cuz null terminator so this caps our typing at 31 chars
        {
            typed[typedLen++] = c; // adds char into array and increment length for next one
            typed[typedLen] = '\0';// end w null terminator

            std::cout << "Typed: " << typed << std::endl; //print what we typed so far
        }
    }

    //remove last char from typed array (basically backspace) (typedLen = last char in array)
    static void PopChar()
    {
        if (typedLen > 0) // check if got anyt to pop
        {
            typed[--typedLen] = '\0'; // decrement length and set new end to null terminator
            std::cout << "Typed: " << typed << std::endl; // print 
        }
    }



    //clear everyth typed
    static void ClearTyped()
    {
        typedLen = 0;
        typed[0] = '\0'; //set first char to null terminator
    }

    //draws word, params are (word, xpos of first letter, ypos of first letter, w of each letter quad, h of <-, spacing between letters )
    static void DrawWord(const char* word, float x, float y, float w, float h, float spacing)
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f); //visible jic

        for (int i = 0; word[i] != '\0'; ++i) //loop thru chars in word
        {
            int idx = CharToIndex(word[i]); //convert to index
            if (idx < 0) continue; // reject non letter

            int col = idx % COLS; // calc col n row in sheet 
            int row = idx / COLS;

            float u = col * UV_W;
            float v = row * UV_H;

            AEGfxTextureSet(lettersTex, u, v);

            float px = x + i * (w + spacing); // calc next letter pos
            Mesh_Renderer(letterQuad, px, y, w, h, 0.0f);
        }
    }

    // calculate start x pos to center word n call DrawWord params: (word, xpos of first letter, ypos of first letter, w of each letter quad, h of <-, spacing between letters )
    static void DrawWordCentered(const char* word, float centerX, float y,
        float w, float h, float spacing)
    {
        int len = (int)strlen(word); // count chars not including nullt
        if (len <= 0) return; // nth to draw do nth

        float totalWidth = (len * w) + ((len - 1) * spacing); // calc total width of word
        float startX = centerX - totalWidth * 0.5f;// calc start x pos to center word

        DrawWord(word, startX, y, w, h, spacing);// draw
    }


    static void DrawCursorAfterWordCentered(const char* word, float centerX, float y, float w, float h, float spacing) {
        int len = (int)strlen(word);

        // total width of letters + spacing
        float totalWidth = (len * w) + ((len > 0) ? (len - 1) * spacing : 0.0f);

        // start x that DrawWordCentered uses
        float startX = centerX - totalWidth * 0.5f;

        // cursor x position = after last character
        float cursorX = startX + len * (w + spacing);

        // Draw a thin rectangle as cursor
        // You'll need a simple "white quad" mesh or use AEGfx primitives.
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);

        // make cursor thin; height ~ 70% of letter height
        float cw = 8.0f;
        float ch = h * 0.7f;

        AEGfxSetRenderMode(AE_GFX_RM_COLOR);
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
        AEGfxSetTransparency(1.0f);


        AEGfxSetBlendColor(1.0f, 1.0f, 1.0f, 1.0f);


        Mesh_Renderer(letterQuad /*or a plain quad*/, cursorX, y, cw, ch, 0.0f);
    }

    const char* Typing_GetText()
    {
        return typed; //return typed word
    }

    void Typing_Clear()
    {
        ClearTyped(); //clear 
    }



}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////namespace end

// load letter spritesheet, create quad for single tile, clear typed array n reset keyboard hist
void Typing_Init()
{
    lettersTex = AEGfxTextureLoad("Assets/lettersheet-Sheet.png"); // load
    AE_ASSERT_MESG(lettersTex, "Failed to load letters spritesheet!");

    letterQuad = Sprite_Mesh(UV_W, UV_H); // create quad

    ClearTyped(); // clear

    for (int i = 0; i < 256; ++i) prevKey[i] = false; // clear prevKey so can type immediately
}


// checks if any letters is pressed this fram, if yes apped to typed array. also checks for backspace = remove last char n submit = treats typed as a submitted word then checks if correct, then clears it
void Typing_Update()
{
    for (int k = AEVK_A; k <= AEVK_Z; ++k) // for all letters
    {
        if (KeyPressedOnce(k)) // if key pressed (not held)
        {
            char c = (char)('A' + (k - AEVK_A));// convert key code to actual char
            PushChar(c); // push to typed array
        }
    }

    if (KeyPressedOnce(AEVK_BACK)) PopChar(); // backspace


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


//draw whatever is currently in typed, no input logic
void Typing_Render()
{
    float dx = 0.0f, dy = 0.0f;

    if (s_shakeTextTime > 0.0f && !s_shakeText.empty()) // if shake timer is active and theres text to shake
    {
        dx = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * s_shakeStrength; // (float)rand() / RAND_MAX generates random float 0 to 1, *2 stretches the range, -1 recenters the range around zero
        dy = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * s_shakeStrength;

        DrawWordCentered(s_shakeText.c_str(), 0.0f + dx, -350.0f + dy, 60.0f, 60.0f, 8.0f);
    }
    else
    {
        DrawWordCentered(typed, 0.0f, -350.0f, 60.0f, 60.0f, 8.0f);
    }


    if (s_cursorVisible)
    {
        DrawCursorAfterWordCentered(typed, 0.0f + dx, -350.0f + dy, 60.0f, 60.0f, 8.0f);
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

void Typing_Exit()
{
    if (letterQuad) { AEGfxMeshFree(letterQuad); letterQuad = nullptr; } //freeee
    if (lettersTex) { AEGfxTextureUnload(lettersTex); lettersTex = nullptr; }// freeeeeee
}











// connects typing and the enemies, params are (enemy_pool, score), enemy pool to find n kill enemy, score so can modify score
void Typing_UpdateAndApplyToEnemies(std::vector<Data::Enemy>& enemy_pool, Data::Score& score) {
    Typing_Update(); //call typing update logic, basicallly pushing whatever is typed to the buffer
    bool submitted = KeyPressedOnce(AEVK_RETURN);    // check for enter

    std::string typed_buffer = typed; // convert from char array to std::string

    if (typed_buffer.empty()) //if nothing typed, unlock target and return
    {
        s_targetIndex = -1;
        return;
    }


    if (s_targetIndex >= 0) // means have locked enemy target
        //if index out of range OR enemy no longer active OR enemy_is_dying OR the typed letters no longer match the start of the target word, target lock clear (s_targetIndex =-1)
        if (s_targetIndex >= (int)enemy_pool.size() || !enemy_pool[(size_t)s_targetIndex].enemy_is_active || enemy_pool[(size_t)s_targetIndex].enemy_is_dying || !Utility::String_StartsWith(enemy_pool[(size_t)s_targetIndex].enemy_word_text, typed_buffer)) s_targetIndex = -1;

    // if no target locked try to find one
    if (s_targetIndex < 0) s_targetIndex = EnemySystem::Enemy_FindBestTargetByPrefix(enemy_pool, typed_buffer);

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

            //if enemy valid AND typed word matches
            if (target.enemy_is_active && !target.enemy_is_dying && target.enemy_word_text == typed_buffer)
            {
                EnemySystem::Enemy_Killed(target); //kill the enemy
                score.score_value += target.enemy_word_length * 10; // increase score based on word length (length x 10)
                s_debugCorrect = true;
                correct = true; //no shake
            }
        }

        if (!correct) //if not correct
        {
            s_shakeText = typed_buffer;           // store what is typed
            s_shakeTextTime = s_shakeTextDuration;
        }



        ClearTyped();  //clear
        s_targetIndex = -1; //no target lock
        return;
    }


    // No target locked yet -> allow free typing (dont clear)
    if (s_targetIndex < 0)
    {
        return;
    }

}
