#include "AEEngine.h"
#include "MeshTemplates.h"
#include "MeshRenderer.h"
#include <cstring>
#include "Typing.h"
#include <iostream>

namespace
{
	AEGfxTexture* lettersTex = nullptr; // texture for letters spritesheet
    AEGfxVertexList* letterQuad = nullptr; //texture for single letter 

    const int COLS{ 8 }; //sheet grid cols
    const int ROWS {4}; //sheet grid rows
    const float UV_W{1.0f / COLS }; //uv width per letter
	const float UV_H {1.0f / ROWS}; //uv height per letter

    const char* target{ "BANANA" }; // target word, change to enemys one later
    char typed[32]{}; // word typed enters this array
    int typedLen{}; // length of typed word

    
	bool prevKey[256]{}; // stores last frame’s key state, basically makes sure u cannot hold the key down, is an array cuz one for every key
}

// converts lowercase to uppercase
static char ToUpper(char c)
{
    if (c >= 'a' && c <= 'z') return (char)(c - 'a' + 'A');
    return c;
}

// make sure key is pressed once per frame so player cannot hold n spam one letter
static bool KeyPressedOnce(int key)
{
	bool curr = AEInputCheckCurr(key) != 0; //  AEInputCheckCurr(key) returns 0 if key not held, else nonzero, !=0 basically makes it true/false (curr == true if key held) else false
	bool once = curr && !prevKey[key];// once == true only if 1.curr is true (key held) and 2. prevKey[key] is false (key was not held last frame)
	prevKey[key] = curr;// store current state for next frame
    return once;
}

//convert char to index for sprite sheet
static int CharToIndex(char c)
{
	c = ToUpper(c);// convert to upper
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

// check if what we typed match target
static bool SubmitCorrect()
{
    int i = 0; //index
    while (target[i] && typed[i]) //while both strs still have chars
    {
		if (ToUpper(target[i]) != ToUpper(typed[i])) return false; // if mismatch, return false
		++i;// next char
    }
    return (target[i] == '\0' && typed[i] == '\0'); // check if both strs end tgt
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




// load letter spritesheet, create quad for single tile, clear typed array n reset keyboard hist
void Typing_Init()
{
    lettersTex = AEGfxTextureLoad("Assets/lettersheet-Sheet.png"); // load
    AE_ASSERT_MESG(lettersTex, "Failed to load letters spritesheet!"); 

	const float LETTER_UV_W = 1.0f / 8.0f; // uv size per tile
    const float LETTER_UV_H = 1.0f / 4.0f;

	letterQuad = Sprite_Mesh(LETTER_UV_W, LETTER_UV_H); // create quad

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

    // backspace
    if (KeyPressedOnce(AEVK_BACK))
    {
        PopChar();
    }

    // submit
    if (KeyPressedOnce(AEVK_RETURN))
    {
        std::cout << "SUBMIT: " << typed << std::endl;
        if (SubmitCorrect())
        {
            std::cout << "Correct (" << typed << ")\n";
        }
        else {
            std::cout << " wrong u Typed: " << typed << " Target: " << target << "\n";
        }

        ClearTyped(); 
    }
}


//draw whatever is currently in typed
void Typing_Render()
{
    DrawWordCentered(typed, 0.0f, -350.0f, 60.0f, 60.0f, 8.0f);


    static bool printed = false; // debug text
    if (!printed)
    {
        std::cout << "Typing_Render called" << std::endl;
        printed = true;
    }
}

void Typing_Exit()
{
    if (letterQuad) { AEGfxMeshFree(letterQuad); letterQuad = nullptr; } //freeee
    if (lettersTex) { AEGfxTextureUnload(lettersTex); lettersTex = nullptr; }// freeeeeee
}
