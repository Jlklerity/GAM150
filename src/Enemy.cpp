/*!
@file           Enemy.cpp
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#include "pch.h"
#include "Enemy.h"

/*
@namespace      EnemyConfig
@brief          Central location for all enemy-related tuning values and gameplay constants.

This namespace groups the configuration constants used by the enemy, difficulty, spawning, and lane
systems. Keeping these values in one place makes it easy to tweak pacing, spacing, and visuals without
touching the core gameplay code.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace EnemyConfig
{
    // Enemy pool sizing and safety limits
    const int   MAX_ENEMIES = 40;                                                                                           // max enemies stored in the pool
    const int   MAX_SPAWN_TRIES = 160;                                                                                      // max attempts to find a safe spawn spot
    const int   MAX_WORD_TRIES = 160;                                                                                       // max attempts to find a unique word

    // Visual and text sizing
    const float ENEMY_SIZE_PX = 150.0f;                                                                                     // enemy sprite size
    const float TEXT_SCALE = 0.6f;                                                                                          // enemy word text scale
    const float WORD_LABEL_OFFSET_PX = 80.0f;                                                                               // word drawn y axis offset

    // Difficulty tuning
    const float DIFFICULTY_RAMP_PER_SECOND = 0.1f;                                                                          // difficulty gain per second
    const float DIFFICULTY_MAX = 30.0f;                                                                                     // cap for difficulty value

    // Random spawn tuning
    const float RANDOM_SPAWN_INTERVAL_START_SECONDS = 2.5f;                                                                 // starting spawn interval
    const float RANDOM_SPAWN_INTERVAL_MIN_SECONDS = 0.85f;                                                                  // minimum spawn interval
    const float RANDOM_SPAWN_INTERVAL_DECREASE_PER_LEVEL = 0.020f;                                                          // interval decrease per difficulty level

    // Fall speed tuning
    const float ENEMY_FALL_SPEED_START = 45.0f;                                                                             // starting fall speed
    const float ENEMY_FALL_SPEED_INCREASE_PER_LEVEL = 1.5f;                                                                 // fall speed increase per difficulty level

    // Death drop tuning
    const float ENEMY_DEATH_FALL_SPEED = 850.0f;                                                                            // fast drop speed when killed
    const float ENEMY_DEATH_LINGER_SECONDS = 0.5f;                                                                          // time to keep a killed enemy visible before removing it

    // Friendly fly-away tuning
    const float ENEMY_FRIENDLY_FLY_SPEED = 300.0f;                                                                          // upward fly speed for bird phase

    // Spacing
    const float ENEMY_OVERLAP_PADDING_PX = 12.0f;                                                                           // extra padding between enemies

    // Lane system
    const int   ENEMY_LANE_COUNT = 8;                                                                                       // number of horizontal lanes
    const float ENEMY_LANE_MIN_GAP_PX = ENEMY_SIZE_PX + ENEMY_OVERLAP_PADDING_PX;                                           // min vertical spacing per lane


    // letter orbit effect tuning values
    const float ORBIT_RADIUS_MAX_NDC = 0.2f;                                                                                // maximum orbit radius in NDC space that letters reach at full effect
    const float ORBIT_SPEED = 3.5f;                                                                                         // how fast letters orbit in radians per second
    const float ORBIT_FADE_SPEED = 1.5f;                                                                                    // lerp speed when orbit radius fades in or out
    const float ORBIT_IDLE_DURATION_SECONDS = 4.0f;                                                                         // how long letters sit at baseline between orbits
    const int   ORBIT_SPAWN_CHANCE = 40;                                                                                    // percentage chance 0 to 99 that a spawned enemy gets the orbit effect
} // EnemyConfig namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      Utility
@brief          Provides small, reusable helper functions used across the game.

This namespace groups common "building block" helpers (math, random, input checks, coordinate conversion,
collision tests, and simple string utilities) to keep gameplay code cleaner and avoid duplicating logic.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace Utility
{
    /*
    @function       Utility_Clamp
    @params         value (float), minimum (float), maximum (float)
    @brief          Clamps a float value to a given inclusive range.

    This function returns 'minimum' if 'value' is below the range, 'maximum' if it is above the range,
    or the original 'value' if it is already within [minimum, maximum].
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float utility_clamp(float value, float minimum, float maximum)
    {
        if (value < minimum) return minimum;                                                                                // clamp low end
        if (value > maximum) return maximum;                                                                                // clamp high end
        return value;                                                                                                       // return clamped or original value
    }

    /*
    @function       Utility_Lerp
    @params         a (float), to (float), t (float)
    @brief          Linearly interpolates between two float values.

    This function blends from 'a' to 'b' using 't' as the interpolation factor.
    't' is clamped to [0, 1], where 0 returns 'a', 1 returns 'b', and values in-between return a mix.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float utility_lerp(float a, float b, float t)
    {
        t = utility_clamp(t, 0.0f, 1.0f);                                                                                   // keep blend factor within 0 to 1
        return a + (b - a) * t;                                                                                             // return blended value
    }

    /*
    @function       Utility_RandInt
    @params         minimum (float), maximum_inclusive (float)
    @brief          Returns a random integer within an inclusive range.

    This function generates a uniformly distributed integer in [minimum, maximum_inclusive].
    If the range is invalid (maximum_inclusive < minimum), it safely returns 'minimum'.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int utility_rand_int(int minimum, int maximum_inclusive)
    {
        if (maximum_inclusive < minimum) return minimum;                                                                    // handle invalid range safely
        int range = maximum_inclusive - minimum + 1;                                                                        // compute inclusive range size
        return minimum + (rand() % range);                                                                                  // return random value in range
    }

    /*
    @function       Convert_WorldToNdc
    @params         world_x (float), world_y (float), ndc_x (float&), ndc_y (float&)
    @brief          Converts world-space pixel coordinates into normalized device coordinates (NDC).

    This function maps a position given in world pixels to NDC space, where the window center is (0, 0)
    and the visible range is typically -1..1 on each axis. The result is written to 'ndc_x' and 'ndc_y'.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void convert_world_to_ndc(float world_x, float world_y, float& ndc_x, float& ndc_y)
    {
        float half_window_width = (float)AEGfxGetWindowWidth() * 0.5f;                                                      // half window width in pixels
        float half_window_height = (float)AEGfxGetWindowHeight() * 0.5f;                                                    // half window height in pixels

        ndc_x = (half_window_width > 0.0f) ? (world_x / half_window_width) : 0.0f;                                          // convert x to -1..1 range
        ndc_y = (half_window_height > 0.0f) ? (world_y / half_window_height) : 0.0f;                                        // convert y to -1..1 range
    }

    /*
    @function       Collision_Overlap
    @params         a_center_x (float), a_center_y (float), a_half_width (float), a_half_height (float)
                    b_center_x (float), b_center_y (float), b_half_width (float), b_half_height (float)
    @brief          Tests whether two axis-aligned bounding boxes (AABBs) overlap.

    This function performs an AABB overlap test using the centers and half-extents of two rectangles.
    It returns true if the rectangles intersect (including touching edges), otherwise false.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool collision_overlap(float a_center_x, float a_center_y, float a_half_width, float a_half_height, float b_center_x, float b_center_y, float b_half_width, float b_half_height)
    {
        float a_left = a_center_x - a_half_width;                                                                           // compute 'a' left edge
        float a_right = a_center_x + a_half_width;                                                                          // compute 'a' right edge
        float a_bottom = a_center_y - a_half_height;                                                                        // compute 'a' bottom edge
        float a_top = a_center_y + a_half_height;                                                                           // compute 'a' top edge

        float b_left = b_center_x - b_half_width;                                                                           // compute 'b' left edge
        float b_right = b_center_x + b_half_width;                                                                          // compute 'b' right edge
        float b_bottom = b_center_y - b_half_height;                                                                        // compute 'b' bottom edge
        float b_top = b_center_y + b_half_height;                                                                           // compute 'b' top edge

        if (a_right < b_left) return false;                                                                                 // 'a' is left of 'b'
        if (a_left > b_right) return false;                                                                                 // 'a' is right of 'b'
        if (a_top < b_bottom) return false;                                                                                 // 'a' is below 'b'
        if (a_bottom > b_top) return false;                                                                                 // 'a' is above 'b'
        return true;                                                                                                        // return true if boxes overlap
    }

    /*
    @function       String_ToUpperInPlace
    @params         s (std::string&)
    @brief          Converts all characters in a string to uppercase.

    This function modifies the input string directly by uppercasing each character using 'std::toupper'.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void string_to_upper_in_place(std::string& s)
    {
        for (char& c : s)                                                                                                   // loop over each character
            c = (char)std::toupper((unsigned char)c);                                                                       // convert character to uppercase
    }

} // Utility namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      Word
@brief          Handles loading, storing, and selecting words for gameplay.

This namespace provides the word bank data structure and helper functions to read word lists from
asset files, organize them by length, and pick random words for enemies.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace Word
{
    /*
    @function       Word_LoadFromFile
    @params         file_path (const char*), expected_length (int)
    @brief          Loads words from a text file and filters them by a specific length.

    This function reads whitespace-separated words from the file at 'file_path', keeps only words whose
    length matches 'expected_length', converts them to uppercase, and returns them as a vector.
    If the file cannot be opened, it returns an empty list.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::vector<std::string> word_load_from_file(const char* file_path, int expected_length)
    {
        std::vector<std::string> loaded_words;                                                                              // storage for loaded words
        std::ifstream file_stream(file_path);                                                                               // open the file for reading
        if (!file_stream.is_open())                                                                                         // check if file opened correctly
            return loaded_words;                                                                                            // return empty list on failure

        std::string candidate_word;                                                                                         // temporary word buffer
        while (file_stream >> candidate_word)                                                                               // read words separated by whitespace
        {
            if ((int)candidate_word.size() == expected_length)                                                              // keep only words of the expected length
            {
                Utility::string_to_upper_in_place(candidate_word);                                                             // convert word to uppercase
                loaded_words.push_back(candidate_word);                                                                     // store word in list
            }
        }
        return loaded_words;                                                                                                // return list of loaded words
    }

    /*
    @function       Word_LoadAll
    @params         word_bank (Data::WordBank&)
    @brief          Loads all word lists (3 to 8 letters) into the WordBank.

    This function populates the WordBank by loading words from the corresponding asset files for each
    supported length (3 to 8). If any file fails to load or results in an empty list, a small fallback
    set is assigned so the game can still run.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void word_load_all(Data::WordBank& word_bank)
    {
        word_bank.words_3 = word_load_from_file("Assets/3letterWords.txt", 3);                                                // load 3-letter words
        word_bank.words_4 = word_load_from_file("Assets/4letterWords.txt", 4);                                                // load 4-letter words
        word_bank.words_5 = word_load_from_file("Assets/5letterWords.txt", 5);                                                // load 5-letter words
        word_bank.words_6 = word_load_from_file("Assets/6letterWords.txt", 6);                                                // load 6-letter words
        word_bank.words_7 = word_load_from_file("Assets/7letterWords.txt", 7);                                                // load 7-letter words
        word_bank.words_8 = word_load_from_file("Assets/8letterWords.txt", 8);                                                // load 8-letter words

        if (word_bank.words_3.empty()) word_bank.words_3 = { "ABC", "XYZ" };                                                // fallback 3-letter words
        if (word_bank.words_4.empty()) word_bank.words_4 = { "ABCD", "WXYZ" };                                              // fallback 4-letter words
        if (word_bank.words_5.empty()) word_bank.words_5 = { "ABCDE", "VWXYZ" };                                            // fallback 5-letter words
        if (word_bank.words_6.empty()) word_bank.words_6 = { "ABCDEF", "UVWXYZ" };                                          // fallback 6-letter words
        if (word_bank.words_7.empty()) word_bank.words_7 = { "ABCDEFG", "TUVWXYZ" };                                        // fallback 7-letter words
        if (word_bank.words_8.empty()) word_bank.words_8 = { "ABCDEFGH", "STUVWXYZ" };                                      // fallback 8-letter words
    }

    /*
    @function       Word_GetListByLength
    @params         word_bank (const Data::WordBank&), word_length (int)
    @brief          Returns the word list that matches the requested word length.

    This function selects and returns a const reference to the appropriate word vector inside 'word_bank'
    based on 'word_length' (3 to 8). If the length is not 3 to 7, it defaults to returning the 8-letter list.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    const std::vector<std::string>& word_get_list_by_length(const Data::WordBank& word_bank, int word_length)
    {
        if (word_length == 3) return word_bank.words_3;                                                                     // return 3-letter list
        if (word_length == 4) return word_bank.words_4;                                                                     // return 4-letter list
        if (word_length == 5) return word_bank.words_5;                                                                     // return 5-letter list
        if (word_length == 6) return word_bank.words_6;                                                                     // return 6-letter list
        if (word_length == 7) return word_bank.words_7;                                                                     // return 7-letter list
        return word_bank.words_8;                                                                                           // return 8-letter list as default
    }

    /*
    @function       Word_PickRandom
    @params         words (const std::vector<std::string>&)
    @brief          Picks and returns a random word from a list.

    This function returns a randomly selected string from 'words'.
    If the list is empty, it returns a placeholder string ("????") to avoid crashes.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::string word_pick_random(const std::vector<std::string>& words)
    {
        if (words.empty()) return "????";                                                                                   // return placeholder if list is empty
        int random_index = Utility::utility_rand_int(0, (int)words.size() - 1);                                              // pick random index
        return words[(size_t)random_index];                                                                                 // return chosen word
    }
} // Word namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      Graphics
@brief          Provides helper functions for rendering sprites, debug shapes, and building simple meshes.

This namespace centralizes small rendering utilities such as selecting enemy textures by word length,
setting common render states (sprite vs solid color), building reusable quad/rect meshes, and drawing
meshes with a standard 2D transform.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace Graphics
{
    AEGfxTexture* s_enemy_textures[9] = { nullptr };                                                                        // enemy textures indexed by word length
    AEGfxTexture* s_friendly_texture = nullptr;                                                                             // princess texture
    AEGfxTexture* s_shield_texture = nullptr;                                                                               // shield enemy texture
    AEGfxTexture* s_heart_texture = nullptr;                                                                                // heart health icon sprite sheet
    s8 g_font_id = -1;                                                                                                      // font handle id

    /*
    @function       Graphics_GetEnemyTextureByWordLength
    @params         word_length (int)
    @brief          Returns the enemy sprite texture that matches a given word length.

    This function clamps 'word_length' to the supported range (3 to 8) and returns the corresponding
    texture from the enemy texture table. It is used to select the correct enemy sprite based on
    the length of the word assigned to that enemy.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    AEGfxTexture* Graphics_GetEnemyTextureByWordLength(int word_length)
    {
        if (word_length < 3) word_length = 3;                                                                               // clamp minimum length
        if (word_length > 8) word_length = 8;                                                                               // clamp maximum length
        return s_enemy_textures[word_length];                                                                               // return the matching texture
    }

    /*
    @function       Graphics_SetColorState
    @params         alpha_value (float)
    @brief          Configures the graphics pipeline for solid-color rendering with transparency.

    This function switches the renderer to color mode, enables alpha blending, and applies the requested
    transparency value. It also resets the multiply/add color modifiers to neutral so debug shapes draw
    with predictable colors.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void graphics_set_color_state(float alpha_value)
    {
        AEGfxSetRenderMode(AE_GFX_RM_COLOR);                                                                                // switch to solid color mode
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // enable blending
        AEGfxSetTransparency(alpha_value);                                                                                  // set alpha for drawing
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                    // keep original colors
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);                                                                         // add no extra color
    }

    /*
    @function       Graphics_SetSpriteState
    @brief          Configures the graphics pipeline for drawing textured sprites.

    This function switches the renderer to texture mode, enables alpha blending, resets transparency
    to fully opaque, and clears any color multiply/add modifiers so sprites render normally.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void graphics_set_sprite_state()
    {
        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);                                                                              // switch to textured mode
        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // enable blending
        AEGfxSetTransparency(1.0f);                                                                                         // draw fully opaque by default
        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                    // keep original colors
        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);                                                                         // add no extra color
    }

    /*
    @function       Mesh_BuildSpriteQuad
    @params         uv_width (float), uv_height (float), color_value (u32)
    @brief          Builds and returns a unit quad mesh with the given UV size and vertex color.

    This function creates a centered 1x1 quad (from -0.5 to 0.5 in both axes) made of two triangles.
    The UVs span [0..uv_width] and [0..uv_height], allowing sprite sheets or partial textures.
    The mesh is returned as an AEGfxVertexList* ready to be used for rendering.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    AEGfxVertexList* Mesh_BuildSpriteQuad(float uv_width, float uv_height, u32 color_value)
    {
        const float left = -0.5f, right = 0.5f;                                                                             // quad x extents
        const float bottom = -0.5f, top = 0.5f;                                                                             // quad y extents

        AEGfxMeshStart();                                                                                                   // start building mesh
        AEGfxTriAdd(left, bottom, color_value, 0.0f, uv_height,                                                             // first triangle for quad
            right, bottom, color_value, uv_width, uv_height,                                                                // second point for triangle
            left, top, color_value, 0.0f, 0.0f);                                                                            // third point for triangle

        AEGfxTriAdd(right, bottom, color_value, uv_width, uv_height,                                                        // second triangle for quad
            right, top, color_value, uv_width, 0.0f,                                                                        // second point for triangle
            left, top, color_value, 0.0f, 0.0f);                                                                            // third point for triangle

        return AEGfxMeshEnd();                                                                                              // finish and return mesh
    }

    /*
    @function       Mesh_BuildUnitRect
    @params         color_value (u32)
    @brief          Builds and returns a centered unit rectangle mesh for solid-color rendering.

    This function creates a 1x1 rectangle (from -0.5 to 0.5 in both axes) made of two triangles with a
    constant vertex color and dummy UVs. It is intended for drawing colored shapes such as UI bars and
    debug rectangles by scaling it to the desired size at render time.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    AEGfxVertexList* Mesh_BuildUnitRect(u32 color_value)
    {
        const float left = -0.5f, right = 0.5f;                                                                             // rect x extents
        const float bottom = -0.5f, top = 0.5f;                                                                             // rect y extents

        AEGfxMeshStart();                                                                                                   // start building mesh
        AEGfxTriAdd(left, bottom, color_value, 0.0f, 0.0f,                                                                  // first triangle for rect
            right, bottom, color_value, 0.0f, 0.0f,                                                                         // second point for triangle
            left, top, color_value, 0.0f, 0.0f);                                                                            // third point for triangle

        AEGfxTriAdd(right, bottom, color_value, 0.0f, 0.0f,                                                                 // second triangle for rect
            right, top, color_value, 0.0f, 0.0f,                                                                            // second point for triangle
            left, top, color_value, 0.0f, 0.0f);                                                                            // third point for triangle

        return AEGfxMeshEnd();                                                                                              // finish and return mesh
    }

    /*
    @function       Draw_DrawMeshAt
    @params         mesh (AEGfxVertexList*), position_x (float), position_y (float), scale_x (float), scale_y (float), rotation_radians (float)
    @brief          Draws a mesh at a given position with scale and rotation.

    This function builds a 2D transform (scale then rotate then translate), applies it to the renderer,
    and draws the provided mesh using triangle mode.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void draw_mesh_at(AEGfxVertexList* mesh, float position_x, float position_y, float scale_x, float scale_y, float rotation_radians)
    {
        AEMtx33 scale_matrix, rotation_matrix, translation_matrix, transform_matrix;                                        // matrices for draw transform
        AEMtx33Scale(&scale_matrix, scale_x, scale_y);                                                                      // build scale matrix
        AEMtx33Rot(&rotation_matrix, rotation_radians);                                                                     // build rotation matrix
        AEMtx33Trans(&translation_matrix, position_x, position_y);                                                          // build translation matrix

        AEMtx33Concat(&transform_matrix, &rotation_matrix, &scale_matrix);                                                  // combine rotation and scale
        AEMtx33Concat(&transform_matrix, &translation_matrix, &transform_matrix);                                           // apply translation

        AEGfxSetTransform(transform_matrix.m);                                                                              // send transform to renderer
        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);                                                                          // draw mesh as triangles
    }
} // Graphics namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      DifficultySystem
@brief          Controls the game difficulty ramp and exposes difficulty-based tuning values.

This namespace tracks the current difficulty value and provides helper functions to update it over time
and convert it into gameplay parameters such as spawn interval, unlocked word lengths, and enemy fall
speed scaling.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace DifficultySystem
{
    float s_difficulty_value = 0.0f;                                                                                          // current difficulty value

    /*
    @function       Difficulty_Get
    @brief          Returns the current difficulty value.

    This function provides read-only access to the internal difficulty variable so other systems
    (such as debug UI) can display or use the current difficulty level.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float difficulty_get() {
        return s_difficulty_value;                                                                                            // read difficulty value
    }

    /*
    @function       Difficulty_Reset
    @brief          Resets the current difficulty value back to zero.

    This function sets the internal difficulty value to 0.0, returning the game to its starting
    difficulty level. It is called when restarting a level or entering the game from the main menu.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void difficulty_reset() {
        s_difficulty_value = 0.0f;
    }

    /*
    @function       Difficulty_Add
    @params         amount (float)
    @brief          Adjusts the current difficulty value and clamps it to a valid range.

    This function adds a signed amount to the internal difficulty value, then clamps it between 0 and
    DIFFICULTY_MAX to prevent the difficulty from going negative or exceeding the configured cap.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void difficulty_add(float amount)
    {
        s_difficulty_value += amount;                                                                                         // add amount to difficulty
        s_difficulty_value = Utility::utility_clamp(s_difficulty_value, 0.0f, EnemyConfig::DIFFICULTY_MAX);                     // clamp to valid range
    }

    /*
    @function       Difficulty_UpdateOverTime
    @params         dt (float)
    @brief          Increases difficulty gradually over time.

    This function ramps the difficulty based on elapsed time by adding a fixed rate
    (DIFFICULTY_RAMP_PER_SECOND) scaled by delta time.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void difficulty_update_over_time(float dt)
    {
        difficulty_add(EnemyConfig::DIFFICULTY_RAMP_PER_SECOND * dt);                                                       // add difficulty based on elapsed time
    }

    /*
    @function       Difficulty_GetSpawnIntervalSeconds
    @brief          Calculates the current enemy spawn interval based on difficulty.

    This function reduces the spawn interval as difficulty increases, making enemies spawn more often.
    The interval is clamped so it never goes below RANDOM_SPAWN_INTERVAL_MIN_SECONDS.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float difficulty_get_spawn_interval_seconds()
    {
        float interval_decrease = s_difficulty_value * EnemyConfig::RANDOM_SPAWN_INTERVAL_DECREASE_PER_LEVEL;                 // how much the interval shrinks from difficulty
        float interval = EnemyConfig::RANDOM_SPAWN_INTERVAL_START_SECONDS - interval_decrease;                              // compute interval
        if (interval < EnemyConfig::RANDOM_SPAWN_INTERVAL_MIN_SECONDS)                                                      // check minimum
            interval = EnemyConfig::RANDOM_SPAWN_INTERVAL_MIN_SECONDS;                                                      // clamp to minimum interval
        return interval;                                                                                                    // return final interval
    }

    /*
    @function       Difficulty_GetUnlockedMaxWordLength
    @brief          Returns the maximum word length currently unlocked by the difficulty value.

    This function maps the current difficulty into tiers that gradually unlock longer words.
    As difficulty increases, the allowed maximum word length increases from 3 up to 8.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int difficulty_get_unlocked_max_word_length()
    {
        float d = s_difficulty_value;                                                                                         // copy difficulty value
        if (d < 5.0f)  return 3;                                                                                            // allow only 3-letter words
        if (d < 10.0f) return 4;                                                                                            // allow up to 4-letter words
        if (d < 15.0f) return 5;                                                                                            // allow up to 5-letter words
        if (d < 20.0f) return 6;                                                                                            // allow up to 6-letter words
        if (d < 25.0f) return 7;                                                                                            // allow up to 7-letter words
        return 8;                                                                                                           // allow up to 8-letter words
    }

    /*
    @function       Difficulty_GetRandomWordLength
    @brief          Returns a random word length based on the current difficulty.

    This function chooses a random word length between 3 and the currently unlocked maximum word length
    (determined by difficulty). It is used to vary enemy word lengths while still respecting the
    difficulty progression.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int difficulty_get_random_word_length()
    {
        int min_word_length = 3;                                                                                            // minimum word length
        int max_word_length = difficulty_get_unlocked_max_word_length();                                                        // maximum word length based on difficulty
        return Utility::utility_rand_int(min_word_length, max_word_length);                                                  // pick random length in range
    }

    /*
    @function       Difficulty_GetEnemyFallSpeed
    @params         word_length (int)
    @brief          Computes an enemy's fall speed based on current difficulty and word length.

    This function starts from a base fall speed, increases it with difficulty, then applies a word length
    multiplier (shorter words fall faster, longer words fall slower). The multiplier effect ramps in as
    difficulty increases, so early gameplay stays gentler while later gameplay becomes more punishing.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float difficulty_get_enemy_fall_speed(int word_length)
    {
        float fall_speed_bonus = s_difficulty_value * EnemyConfig::ENEMY_FALL_SPEED_INCREASE_PER_LEVEL;                       // bonus speed from difficulty scaling
        float base_speed = EnemyConfig::ENEMY_FALL_SPEED_START + fall_speed_bonus;                                          // base speed from difficulty

        float t = Utility::utility_clamp(s_difficulty_value / 14.0f, 0.0f, 1.0f);                                             // normalize difficulty into 0..1

        float target_multiplier = 1.0f;                                                                                     // default multiplier
        if (word_length == 3) target_multiplier = 1.20f;                                                                    // 3-letter enemies fall faster
        else if (word_length == 4) target_multiplier = 1.14f;                                                               // 4-letter enemies fall faster
        else if (word_length == 5) target_multiplier = 1.08f;                                                               // 5-letter enemies fall slightly faster
        else if (word_length == 6) target_multiplier = 1.02f;                                                               // 6-letter enemies fall near base speed
        else if (word_length == 7) target_multiplier = 0.96f;                                                               // 7-letter enemies fall slightly slower
        else target_multiplier = 0.90f;                                                                                     // 8-letter enemies fall slower

        float multiplier = Utility::utility_lerp(1.0f, target_multiplier, t);                                               // blend multiplier based on difficulty
        return base_speed * multiplier;                                                                                     // return final fall speed
    }
} // DifficultySystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      HealthSystem
@brief          Manages player health updates and clamping.

This namespace contains helper functions for modifying the player health safely, ensuring the
current health value stays within a valid range (0 to max).
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace HealthSystem
{
    /*
    @function       Health_Init
    @params         health (Data::Health&)
    @brief          Sets the current health back to its maximum value.

    This function resets health_current to health_max, restoring the player to full health.
    It is called when entering the game or restarting a level.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void health_init(Data::Health& health) {
        health.health_max = 50.0f;                                                                                          // reset max health to default
        health.health_current = health.health_max;
    }

    /*
    @function       Health_Add
    @params         h (Data::Health&), amount (float)
    @brief          Adds (or subtracts) health and clamps the result to a valid range.

    This function applies a signed health change to 'h.health_current' and then clamps it between
    0 and 'h.health_max' to prevent underflow or overflow.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void health_add(Data::Health& h, float amount)
    {
        h.health_current += amount;                                                                                         // add change to current health
        h.health_current = Utility::utility_clamp(h.health_current, 0.0f, h.health_max);                                    // clamp to valid range
    }

} // HealthSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      Animation
@brief          Updates sprite sheet animation timing and UV frame selection.

This namespace contains helper functions for advancing sprite animations based on elapsed time,
handling frame looping, and computing the UV offsets for the current frame.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace Animation
{
    /*
    @function       Animation_ApplyUv
    @params         anim (Data::SpriteAnim&)
    @brief          Recomputes and writes the UV x/y offsets from the current frame index and row.

    This function calculates the column from the current frame index, reads the current row index,
    and updates the UV coordinates on the animation struct. It is called whenever the frame changes
    to keep the UV offsets in sync with the sprite sheet layout.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void animation_apply_uv(Data::SpriteAnim& anim)
    {
        int current_column = anim.animation_frame_index % anim.animation_columns;                                           // compute column from frame index
        int current_row = anim.animation_row_index;                                                                         // use the currently selected row
        anim.animation_uv_x = current_column * anim.Anim_UvWidth();                                                         // update UV x offset for this frame
        anim.animation_uv_y = current_row * anim.Anim_UvHeight();                                                           // update UV y offset for this row
    }

    /*
    @function       Animation_SetFrame
    @params         anim (Data::SpriteAnim&), frame_index (int)
    @brief          Forces the animation to a specific frame and updates the UV coordinates.

    This function directly sets the current animation frame index (typically to show a specific pose, like
    a death frame), then recalculates the sprite sheet UV offsets based on the current column and the
    selected animation row.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void animation_set_frame(Data::SpriteAnim& anim, int frame_index)
    {
        anim.animation_frame_index = frame_index;                                                                           // set the current frame index directly
        animation_apply_uv(anim);                                                                                            // update UV coords for the new frame
    }

    /*
    @function       Animation_Update
    @params         anim (Data::SpriteAnim&), dt (float)
    @brief          Advances a sprite animation and updates its UV coordinates.

    This function accumulates delta time, advances the animation frame when the frame duration is reached,
    wraps the frame index back to 0 when it exceeds the number of frames, and then updates the sprite sheet
    UV offsets using the current frame column and the selected animation row.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void animation_update(Data::SpriteAnim& anim, float dt)
    {
        anim.animation_timer_seconds += dt;                                                                                 // add delta time to timer
        if (anim.animation_timer_seconds < anim.animation_frame_duration_seconds)                                           // check if frame time not reached
            return;                                                                                                         // exit early if still waiting

        anim.animation_timer_seconds = 0.0f;                                                                                // reset timer after advancing frame

        anim.animation_frame_index++;                                                                                       // move to next frame
        if (anim.animation_frame_index >= anim.Anim_FrameCount())                                                           // wrap if beyond last frame
            anim.animation_frame_index = 0;                                                                                 // reset to first frame

        animation_apply_uv(anim);                                                                                            // update UV coords for the new frame
    }
} // Animation namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      EnemySystem
@brief          Manages enemy spawning, updating, and rendering using a pooled lane based system.

This namespace contains the core logic for enemies: picking unique words, finding safe spawn positions,
creating enemies from a pool, updating movement/state (including death drops and health damage), resolving
lane spacing to prevent stacking, and drawing both sprites and their word text.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace EnemySystem
{
    /*
    @function       Enemy_IsWordInUse
    @params         pool (const std::vector<Data::Enemy>&), word_text (const std::string&), include_hidden_words (bool)
    @brief          Checks if a word is already assigned to any active enemy in the pool.

    This function scans all active enemies and returns true if any of them already use the given word.
    If include_hidden_words is true, it also checks the hidden second word on shielded enemies.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool enemy_is_word_in_use(const std::vector<Data::Enemy>& pool, const std::string& word_text, bool include_hidden_words)
    {
        if (word_text.empty()) return true;

        for (const Data::Enemy& e : pool)                                                                                   // loop through enemy pool
        {
            if (!e.enemy_is_active) continue;                                                                               // skip inactive enemies

            if (e.enemy_word_text == word_text) return true;                                                                // report word is already used

            if (include_hidden_words && e.enemy_type == Data::EnemyType::SHIELDED)                                          // check the hidden second word on shielded enemies
            {
                if (!e.enemy_word_text_alt.empty() && e.enemy_word_text_alt == word_text) return true;                      // hidden word is being used
            }
        }
        return false;                                                                                                       // report word is not used
    }

    /*
    @function       Enemy_PickUniqueWord
    @params         word_length (int), bank (const Data::WordBank&), pool (const std::vector<Data::Enemy>&), include_hidden_words (bool)
    @brief          Picks a random word of the given length that is not already used by any active enemy.

    This function randomly picks words from the word bank and keeps trying until it finds one that
    is not already in use. Returns an empty string if no unique word is found within the attempt limit.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::string enemy_pick_unique_word(int word_length, const Data::WordBank& bank, const std::vector<Data::Enemy>& pool, bool include_hidden_words)
    {
        const auto& list = Word::word_get_list_by_length(bank, word_length);                                                   // fetch list for desired length
        if (list.empty()) return std::string{};

        for (int tries = 0; tries < EnemyConfig::MAX_WORD_TRIES; ++tries)                                                   // try several times to find a unique word
        {
            std::string candidate = Word::word_pick_random(list);                                                            // pick a random word

            if (!enemy_is_word_in_use(pool, candidate, include_hidden_words))                                                  // check if word is unused
                return candidate;                                                                                           // return unique word
        }
        return std::string{};                                                                                               // return empty string on failure
    }

    /*
    @function       Enemy_PickUniqueWordRandomLength
    @params         min_len (int), max_len (int), bank (const Data::WordBank&), pool (const std::vector<Data::Enemy>&), include_hidden_words (bool), max_tries_total (int)
    @brief          Picks a unique word with a random length chosen between min_len and max_len.

    This function picks a random word length each attempt and tries to find a unique word of that
    length. It keeps trying up to max_tries_total times. Returns an empty string if nothing is found.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::string enemy_pick_unique_word_random_length(int min_len, int max_len, const Data::WordBank& bank, const std::vector<Data::Enemy>& pool, bool include_hidden_words, int max_tries_total)
    {
        if (min_len > max_len) std::swap(min_len, max_len);

        if (min_len < 3) min_len = 3;                                                                                       // clamp to shortest supported length
        if (max_len > 8) max_len = 8;                                                                                       // clamp to longest supported length

        for (int tries = 0; tries < max_tries_total; ++tries)                                                               // keep trying until the attempt limit is reached
        {
            int len = Utility::utility_rand_int(min_len, max_len);                                                           // pick a random length in the allowed range
            std::string w = enemy_pick_unique_word(len, bank, pool, include_hidden_words);                                    // try to find a unique word of that length
            if (!w.empty()) return w;                                                                                       // return the word if one was found
        }
        return std::string{};                                                                                               // return empty if nothing found within the limit
    }

    /*
    @function       Enemy_GetWordHalfWidthWorld
    @params         word (const std::string&)
    @brief          Calculates the half-width of a word in world pixels for spacing checks.

    This function measures the rendered width of 'word' using the current font and TEXT_SCALE, then converts
    that screen-space size into world pixels. It is used to keep spawned enemies (and their text) within
    usable horizontal bounds and to help avoid overlaps.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float enemy_get_word_half_width_world(const std::string& word)
    {
        if (Graphics::g_font_id < 0) return 0.0f;                                                                             // return 0 if font is missing

        float text_w_ndc = 0.0f, text_h_ndc = 0.0f;                                                                         // storage for text size in screen space
        AEGfxGetPrintSize(Graphics::g_font_id, word.c_str(), EnemyConfig::TEXT_SCALE, &text_w_ndc, &text_h_ndc);              // measure text

        float half_window_width = (float)AEGfxGetWindowWidth() * 0.5f;                                                      // half window width in pixels
        return (text_w_ndc * 0.5f) * half_window_width;                                                                     // convert half width to world pixels
    }

    /*
    @function       Enemy_GetLaneX
    @params         lane_id (int), usable_min_x (float), usable_max_x (float)
    @brief          Computes the world X position for a given lane index.

    This function maps a lane id into the usable horizontal range and returns the
    corresponding X coordinate.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float enemy_get_lane_x(int lane_id, float usable_min_x, float usable_max_x)
    {
        float t = (float)lane_id / (float)(EnemyConfig::ENEMY_LANE_COUNT - 1);                                              // lane position as 0..1
        return Utility::utility_lerp(usable_min_x, usable_max_x, t);                                                        // return lane x position
    }

    /*
    @function       Enemy_IsLaneSpawnYFree
    @params         lane_id (int), spawn_y (float), pool (const std::vector<Data::Enemy>&)
    @brief          Checks whether a lane has enough vertical space to spawn a new enemy at the spawn line.

    This function scans all active enemies in the specified lane and finds the closest enemy below the
    spawn line (smallest positive gap). It returns true only if the nearest gap is at least
    ENEMY_LANE_MIN_GAP_PX, ensuring newly spawned enemies won't overlap or stack too tightly.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool enemy_is_lane_spawn_y_free(int lane_id, float spawn_y, const std::vector<Data::Enemy>& pool)
    {
        float closest_gap = 999.0f;                                                                                         // start with very large gap

        for (const auto& e : pool)                                                                                          // loop through enemy pool
        {
            if (!e.enemy_is_active) continue;                                                                               // skip inactive enemies
            if (e.enemy_lane_id != lane_id) continue;                                                                       // skip other lanes

            float gap = spawn_y - e.enemy_position_y;                                                                       // compute vertical gap from spawn line
            if (gap >= 0.0f && gap < closest_gap)                                                                           // keep the smallest positive gap
                closest_gap = gap;                                                                                          // store the closest gap
        }

        return (closest_gap >= EnemyConfig::ENEMY_LANE_MIN_GAP_PX);                                                         // return true if spacing is safe
    }

    /*
    @function       Enemy_Create
    @params         area (const Data::EnemyArea&), word_length (int), word_text (const std::string&), fall_speed (float), lane_id (int)
    @brief          Creates and initializes a new enemy instance with the provided spawn settings.

    This function constructs a Data::Enemy with its starting position at the spawn line, assigns its word
    (text and length), fall speed, and lane id, and resets all state flags, timers, and animation fields to
    their initial values so the enemy is ready to update and render.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    Data::Enemy Enemy_Create(const Data::EnemyArea& area, int word_length, const std::string& word_text, float fall_speed, int lane_id)
    {
        Data::Enemy e{};                                                                                                    // create enemy with default values
        e.enemy_position_x = 0.0f;                                                                                          // set initial x
        e.enemy_position_y = area.area_spawn_y;                                                                             // set initial y at spawn line

        e.enemy_is_active = true;                                                                                           // mark as active
        e.enemy_has_landed = false;                                                                                         // clear landed flag
        e.enemy_did_damage = false;                                                                                         // clear damage flag
        e.enemy_is_dying = false;                                                                                           // clear dying flag
        e.enemy_dying_linger_timer_seconds = 0.0f;                                                                          // reset death linger timer

        e.enemy_word_length = word_length;                                                                                  // store word length
        e.enemy_word_text = word_text;                                                                                      // store word text

        e.enemy_fall_speed_units_per_second = fall_speed;                                                                   // store fall speed
        e.enemy_lane_id = lane_id;                                                                                          // store lane id

        e.enemy_animation.animation_row_index = 0;                                                                          // start on the default animation row
        e.enemy_animation.animation_frame_index = 0;                                                                        // reset frame index
        e.enemy_animation.animation_timer_seconds = 0.0f;                                                                   // reset animation timer

        e.enemy_animation.animation_uv_x = 0.0f;                                                                            // reset uv x
        e.enemy_animation.animation_uv_y = 0.0f;                                                                            // reset uv y

        return e;                                                                                                           // return created enemy
    }

    /*
    @function       Enemy_BreakShield
    @params         e (Data::Enemy&)
    @brief          Breaks the shield on a shielded enemy and switches it to its second word.

    This function removes the shield from a shielded enemy. It swaps in the hidden second word,
    updates the word length, and resets the animation to the unshielded row of the sprite sheet.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_break_shield(Data::Enemy& e)
    {
        if (!e.enemy_is_active) return;                                                                                     // skip if enemy is not active
        if (e.enemy_is_dying) return;                                                                                       // skip if enemy is already dying
        if (e.enemy_type != Data::EnemyType::SHIELDED) return;                                                              // skip if not a shielded enemy
        if (!e.enemy_shield_intact) return;                                                                                 // skip if shield is already broken

        e.enemy_shield_intact = false;                                                                                      // mark the shield as broken

        e.enemy_word_text = e.enemy_word_text_alt;                                                                          // swap in the hidden second word
        e.enemy_word_length = (int)e.enemy_word_text.size();                                                                // update word length to match the new word

        e.enemy_animation.animation_columns = 6;                                                                            // shielded sheet has 6 columns
        e.enemy_animation.animation_rows = 3;                                                                               // shielded sheet has 3 rows

        e.enemy_animation.animation_row_index = 1;                                                                          // switch to the unshielded animation row
        e.enemy_animation.animation_frame_index = 0;                                                                        // restart animation from frame 0
        e.enemy_animation.animation_timer_seconds = 0.0f;                                                                   // reset the frame timer
        Animation::animation_set_frame(e.enemy_animation, 0);                                                                // update UV coords to match the new row and frame
    }

    /*
    @function       Enemy_Killed
    @params         e (Data::Enemy&)
    @brief          Marks an enemy as killed and switches it into the death drop state.

    This function safely transitions an active enemy into a dying state (only once). It clears the landed
    flag so the enemy can drop off-screen, and marks damage as already handled so a killed enemy will not
    apply landing damage.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_killed(Data::Enemy& e, Data::Health& health)
    {
        if (!e.enemy_is_active) return;                                                                                     // ignore if already inactive
        if (e.enemy_is_dying)   return;                                                                                     // ignore if already dying

        // shared state
        e.enemy_is_dying = true;
        e.enemy_has_landed = false;                                                                                         // land flag
        e.enemy_did_damage = true;                                                                                          // prevent any further landing damage
        e.enemy_dying_linger_timer_seconds = 0.0f;                                                                          // start linger timer from zero

        e.enemy_animation.animation_columns = 6;                                                                            // all sheets uses 6 columns
        e.enemy_animation.animation_frame_index = 0;                                                                        // start death animation from frame 0
        e.enemy_animation.animation_timer_seconds = 0.0f;                                                                   // restart timing

        if (e.enemy_type == Data::EnemyType::FRIENDLY)                                                                      // friendly npc
        {
            HealthSystem::health_add(health, 10.0f);                                                                        // heal player
            e.enemy_word_text.clear();                                                                                      // hide word label immediately
            e.enemy_word_length = 0;                                                                                        // clear word length to match
            e.enemy_animation.animation_rows = 3;                                                                           // friendly sheet has 3 rows
            e.enemy_animation.animation_row_index = e.enemy_death_row_index;                                                // break free from cage row
        }
        else if (e.enemy_type == Data::EnemyType::SHIELDED)                                                                 // shield enemy
        {
            e.enemy_animation.animation_rows = 3;                                                                           // shielded sheet has 3 rows
            e.enemy_animation.animation_row_index = e.enemy_death_row_index;                                                // death row
        }
        else
        {
            e.enemy_animation.animation_rows = 2;                                                                           // normal sheet has 2 rows
            e.enemy_animation.animation_row_index = e.enemy_death_row_index;                                                // death row
        }

        Animation::animation_set_frame(e.enemy_animation, 0);                                                                // update UV coords to match the new row and frame
    }

    /*
    @function       Enemy_Update
    @params         e (Data::Enemy&), area (const Data::EnemyArea&), dt (float), health (Data::Health&)
    @brief          Updates an enemy's animation, movement, and state transitions for the current frame.

    This function handles both the normal falling behaviour and the dying "death drop" behaviour. It updates
    animation frames, moves the enemy downward, clamps it within the enemy area, applies landing damage once,
    and deactivates the enemy when its lifecycle is complete (landed or finished dying + linger).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_update(Data::Enemy& e, const Data::EnemyArea& area, float dt, Data::Health& health)
    {
        if (!e.enemy_is_active) return;                                                                                     // skip inactive enemies

        // friendly lifecycle
        if (e.enemy_type == Data::EnemyType::FRIENDLY)                                                                      // friendly has a unique 3 stage lifecycle, handle separately
        {
            if (e.enemy_animation.animation_row_index == 0)                                                                 // row 0: cage falling, behaves like a normal enemy
            {
                Animation::animation_update(e.enemy_animation, dt);                                                         // advance cage animation

                if (!e.enemy_has_landed)                                                                                    // cage is still falling
                {
                    e.enemy_position_y -= e.enemy_fall_speed_units_per_second * dt;                                         // fall downward

                    e.enemy_position_x = Utility::utility_clamp(e.enemy_position_x,
                        area.area_min_x, area.area_max_x);                                                                  // clamp x within enemy area
                    e.enemy_position_y = Utility::utility_clamp(e.enemy_position_y,
                        area.Area_BoxMinY(), area.Area_BoxMaxY());                                                          // clamp y within enemy area

                    if (e.enemy_position_y <= area.area_landing_y)                                                          // reached landing line
                    {
                        e.enemy_position_y = area.area_landing_y;                                                           // snap to landing line
                        e.enemy_has_landed = true;                                                                          // mark as landed

                        if (!e.enemy_did_damage)                                                                            // damage not applied yet
                        {
                            e.enemy_did_damage = true;                                                                      // mark damage handled
                            e.enemy_is_active = false;                                                                      // remove after landing

                            if (!PowerUpSystem::is_shield_active())                                                           // only damage if shield is not active
                            {
                                HealthSystem::health_add(health, -10.0f);                                                   // damage player

                            }
                        }
                        else                                                                                                // damage already handled
                        {
                            e.enemy_is_active = false;                                                                      // remove enemy
                        }
                    }
                }
            }
            else if (e.enemy_animation.animation_row_index == 1)                                                            // row 1: break free animation, plays once with no movement
            {
                const int last_frame = e.enemy_animation.animation_columns - 1;                                             // last frame index in the row

                if (e.enemy_animation.animation_frame_index < last_frame)                                                   // keep advancing until the final frame
                {
                    e.enemy_animation.animation_timer_seconds += dt;                                                        // accumulate time for break free animation

                    if (e.enemy_animation.animation_timer_seconds >= e.enemy_animation.animation_frame_duration_seconds)    // advance when frame duration is reached
                    {
                        e.enemy_animation.animation_timer_seconds = 0.0f;                                                   // reset timer after frame advance
                        Animation::animation_set_frame(e.enemy_animation, e.enemy_animation.animation_frame_index + 1);      // move to the next frame
                    }
                }
                else                                                                                                        // final frame reached, transition to bird fly
                {
                    e.enemy_animation.animation_row_index = 2;                                                              // switch to bird fly row
                    e.enemy_animation.animation_frame_index = 0;                                                            // reset to first frame
                    e.enemy_animation.animation_timer_seconds = 0.0f;                                                       // reset frame timer
                    Animation::animation_set_frame(e.enemy_animation, 0);                                                    // update UV coords to match the new row and frame
                    e.enemy_is_dying = false;                                                                               // no longer in break free phase, now flying
                }
            }
            else if (e.enemy_animation.animation_row_index == 2)                                                            // row 2: bird fly away, moves upward until off screen
            {
                Animation::animation_update(e.enemy_animation, dt);                                                         // advance bird animation

                e.enemy_position_y += EnemyConfig::ENEMY_FRIENDLY_FLY_SPEED * dt;                                           // fly upward

                if (e.enemy_position_y > area.area_spawn_y + EnemyConfig::ENEMY_SIZE_PX)                                    // fully above the spawn line
                    e.enemy_is_active = false;                                                                              // despawn once off screen
            }
            return;                                                                                                         // friendly fully handled, skip normal and dying paths
        }

        // Dying fall
        if (e.enemy_is_dying)                                                                                               // handle dying state (death animation + drop)
        {
            const int last_frame = e.enemy_animation.animation_columns - 1;                                                 // last frame index in the current row

            if (e.enemy_animation.animation_frame_index < last_frame)                                                       // keep advancing until the final frame
            {
                e.enemy_animation.animation_timer_seconds += dt;                                                            // accumulate time for death animation

                if (e.enemy_animation.animation_timer_seconds >=
                    e.enemy_animation.animation_frame_duration_seconds)                                                     // advance when frame duration is reached
                {
                    e.enemy_animation.animation_timer_seconds = 0.0f;                                                       // reset timer after frame advance
                    Animation::animation_set_frame(e.enemy_animation,
                        e.enemy_animation.animation_frame_index + 1);                                                       // move to the next frame
                }
            }
            else                                                                                                            // final frame reached
            {
                e.enemy_animation.animation_timer_seconds = 0.0f;                                                           // keep timer reset while frozen
            }

            if (e.enemy_position_y > area.area_landing_y)                                                                   // still above the landing line
            {
                e.enemy_position_y -= EnemyConfig::ENEMY_DEATH_FALL_SPEED * dt;                                             // drop quickly while dying

                if (e.enemy_position_y <= area.area_landing_y)                                                              // clamp at landing line
                    e.enemy_position_y = area.area_landing_y;                                                               // snap to landing line
            }
            else                                                                                                            // already at landing line
            {
                e.enemy_dying_linger_timer_seconds += dt;                                                                   // accumulate linger time at landing

                if (e.enemy_dying_linger_timer_seconds >= EnemyConfig::ENEMY_DEATH_LINGER_SECONDS)                          // check linger duration
                    e.enemy_is_active = false;                                                                              // deactivate enemy after lingering
            }

            return;                                                                                                         // skip normal falling logic
        }

        // Normal fall
        Animation::animation_update(e.enemy_animation, dt);                                                                 // advance normal animation

        if (!e.enemy_has_landed)                                                                                            // enemy is still falling
        {
            e.enemy_position_y -= e.enemy_fall_speed_units_per_second * dt;                                                 // move down based on fall speed

            e.enemy_position_x = Utility::utility_clamp(e.enemy_position_x,
                area.area_min_x, area.area_max_x);                                                                          // clamp x within enemy area
            e.enemy_position_y = Utility::utility_clamp(e.enemy_position_y,
                area.Area_BoxMinY(), area.Area_BoxMaxY());                                                                  // clamp y within enemy area

            if (e.enemy_position_y <= area.area_landing_y)                                                                  // reached landing line
            {
                e.enemy_position_y = area.area_landing_y;                                                                   // snap to landing line
                e.enemy_has_landed = true;                                                                                  // mark as landed

                if (!e.enemy_did_damage)                                                                                    // damage not applied yet
                {
                    e.enemy_did_damage = true;                                                                              // mark damage handled
                    e.enemy_is_active = false;                                                                              // remove enemy after landing

                    if (!PowerUpSystem::is_shield_active())
                    {
                        HealthSystem::health_add(health, -10.0f);                                                           // apply landing damage once
                        SoundManager::play_player_hit_sfx();
                    }
                }
                else                                                                                                        // damage already handled
                {
                    e.enemy_is_active = false;                                                                              // remove enemy
                }
            }
        }

        // letter orbit effect update, runs while enemy is falling and has a word
        if (!e.enemy_word_text.empty() && !e.enemy_has_landed)                                                              // only run orbit while enemy is actively falling
        {
            float raw_dt = dt / PowerUpSystem::get_time_scale();                                                              // unscaled dt so orbit is unaffected by the slow power-up

            if (!e.enemy_orbit_initialised)                                                                                 // first frame this enemy is active, do the random roll
            {
                e.enemy_has_orbit_effect = (Utility::utility_rand_int(0, 99) < EnemyConfig::ORBIT_SPAWN_CHANCE);             // randomly decide if this enemy gets the orbit effect
                e.enemy_orbit_initialised = true;                                                                           // mark as initialised so the roll never happens again
                e.enemy_orbit_phase = 0;                                                                                    // start in idle phase
                e.enemy_orbit_idle_timer = EnemyConfig::ORBIT_IDLE_DURATION_SECONDS;                                        // start with a full idle wait before first orbit
            }

            if (e.enemy_has_orbit_effect && !e.enemy_is_targeted)                                                           // only run the orbit cycle if assigned and player has not locked on
            {
                if (e.enemy_orbit_phase == 0)                                                                               // IDLE phase: sitting at baseline, waiting for next orbit
                {
                    e.enemy_orbit_idle_timer -= raw_dt;                                                                     // count down the idle wait
                    e.enemy_orbit_radius = Utility::utility_lerp(                                                           // smoothly return radius to zero while idle
                        e.enemy_orbit_radius, 0.0f, EnemyConfig::ORBIT_FADE_SPEED * raw_dt);

                    if (e.enemy_orbit_idle_timer <= 0.0f)                                                                   // idle wait finished, start orbiting
                    {
                        e.enemy_orbit_phase = 1;                                                                            // switch to ORBITING phase
                        e.enemy_orbit_timer = 0.0f;                                                                         // reset orbit angle so the next orbit starts clean
                    }
                }
                else if (e.enemy_orbit_phase == 1)                                                                          // ORBITING phase: letters spin outward for one full rotation
                {
                    e.enemy_orbit_timer += raw_dt;                                                                          // advance the orbit angle timer

                    e.enemy_orbit_radius = Utility::utility_lerp(                                                           // smoothly lerp radius up toward max
                        e.enemy_orbit_radius, EnemyConfig::ORBIT_RADIUS_MAX_NDC, EnemyConfig::ORBIT_FADE_SPEED * raw_dt);

                    float full_rotation_seconds = (6.2831853f / EnemyConfig::ORBIT_SPEED);                                  // time for one full 360 degree orbit
                    if (e.enemy_orbit_timer >= full_rotation_seconds)                                                       // one full rotation completed, start returning
                    {
                        e.enemy_orbit_phase = 2;                                                                            // switch to RETURNING phase
                    }
                }
                else if (e.enemy_orbit_phase == 2)                                                                          // RETURNING phase: radius fades back to zero
                {
                    e.enemy_orbit_timer += raw_dt;                                                                          // keep advancing timer so letters continue rotating while returning

                    e.enemy_orbit_radius = Utility::utility_lerp(                                                           // smoothly lerp radius back toward zero
                        e.enemy_orbit_radius, 0.0f, EnemyConfig::ORBIT_FADE_SPEED * raw_dt);

                    if (e.enemy_orbit_radius < 0.001f)                                                                      // radius close enough to zero, letters are back at baseline
                    {
                        e.enemy_orbit_radius = 0.0f;                                                                        // snap cleanly to zero
                        e.enemy_orbit_phase = 0;                                                                            // switch back to IDLE phase
                        e.enemy_orbit_idle_timer = EnemyConfig::ORBIT_IDLE_DURATION_SECONDS;                                // reset the 2 second idle wait
                    }
                }
            }
            else if (e.enemy_is_targeted)                                                                                   // player locked on, fade radius to zero immediately
            {
                e.enemy_orbit_radius = Utility::utility_lerp(                                                               // smoothly pull letters back to baseline
                    e.enemy_orbit_radius, 0.0f, EnemyConfig::ORBIT_FADE_SPEED * raw_dt);
            }
        }
        /*--------------------------------*/
    }

    /*
    @function       Enemy_ResolveLaneStacking
    @params         pool (std::vector<Data::Enemy>&), area (const Data::EnemyArea&)
    @brief          Enforces minimum vertical spacing between enemies within each lane.

    This function groups active, non-dying enemies by lane, sorts them by Y position (lowest first),
    and then adjusts positions so each enemy is at least ENEMY_LANE_MIN_GAP_PX above the one below it.
    Adjusted positions are clamped to the top of the enemy area to keep enemies within bounds.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_resolve_lane_stacking(std::vector<Data::Enemy>& pool, const Data::EnemyArea& area)
    {
        for (int lane = 0; lane < EnemyConfig::ENEMY_LANE_COUNT; ++lane)                                                    // loop over each lane
        {
            std::vector<int> idxs;                                                                                          // list of enemy indices in this lane
            idxs.reserve(16);                                                                                               // reserve small capacity

            for (int i = 0; i < (int)pool.size(); ++i)                                                                      // loop over enemy pool indices
            {
                if (!pool[(size_t)i].enemy_is_active) continue;                                                             // skip inactive enemies
                if (pool[(size_t)i].enemy_lane_id != lane) continue;                                                        // skip other lanes
                if (pool[(size_t)i].enemy_is_dying) continue;                                                               // skip dying enemies
                if (pool[(size_t)i].enemy_type == Data::EnemyType::FRIENDLY
                    && pool[(size_t)i].enemy_animation.animation_row_index >= 1) continue;                                  // skip friendly enemies in break free or fly away phase
                idxs.push_back(i);                                                                                          // keep this enemy index
            }

            if (idxs.size() < 2) continue;                                                                                  // skip if 0 or 1 enemy in lane

            std::sort(idxs.begin(), idxs.end(),                                                                             // sort indices by y position
                [&](int a, int b)                                                                                           // sort comparison function
                {
                    return pool[(size_t)a].enemy_position_y < pool[(size_t)b].enemy_position_y;                             // lower y first
                });

            float prev_y = pool[(size_t)idxs[0]].enemy_position_y;                                                          // store y of the first enemy
            for (size_t k = 1; k < idxs.size(); ++k)                                                                        // loop over remaining enemies
            {
                Data::Enemy& e = pool[(size_t)idxs[k]];                                                                     // get enemy by index
                float min_allowed_y = prev_y + EnemyConfig::ENEMY_LANE_MIN_GAP_PX;                                          // compute minimum allowed y

                if (e.enemy_position_y < min_allowed_y)                                                                     // check if too close
                    e.enemy_position_y = min_allowed_y;                                                                     // adjust to enforce spacing

                if (e.enemy_position_y > area.Area_BoxMaxY())                                                               // check if beyond top of area
                    e.enemy_position_y = area.Area_BoxMaxY();                                                               // clamp to top of area

                prev_y = e.enemy_position_y;                                                                                // update last y for next spacing check
            }
        }
    }

    /*
    @function       Enemy_Draw
    @params         meshes (const Data::Meshes&), e (const Data::Enemy&)
    @brief          Renders the enemy sprite at its current position using the correct texture and animation frame.

    This function skips inactive enemies, sets the render state for textured sprites, selects the enemy
    texture based on enemy type (friendly/shield/normal), applies the current animation UV offset, and draws the
    enemy quad scaled to ENEMY_SIZE_PX. Shield enemies use a separate mesh built for 3-row UV cutting.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_draw(const Data::Meshes& meshes, const Data::Enemy& e)
    {
        if (!e.enemy_is_active) return;                                                                                     // skip inactive enemies

        Graphics::graphics_set_sprite_state();                                                                                // set textured draw state

        AEGfxTexture* tex = nullptr;                                                                                        // start with no texture selected

        if (e.enemy_type == Data::EnemyType::FRIENDLY)                                                                      // check if is princess npc
            tex = Graphics::s_friendly_texture;                                                                               // use princess texture
        else if (e.enemy_type == Data::EnemyType::SHIELDED)                                                                 // check if is shielded enemies
            tex = Graphics::s_shield_texture;                                                                                 // use shielded enemies texture
        else
            tex = Graphics::Graphics_GetEnemyTextureByWordLength(e.enemy_word_length);                                      // use normal enemies texture

        if (!tex) return;                                                                                                   // skip if texture missing

        AEGfxVertexList* mesh = meshes.enemy_sprite_mesh;                                                                   // default to 2-row mesh for normal enemies
        if (e.enemy_type == Data::EnemyType::SHIELDED)                                                                      // check if is shielded enemies
            mesh = meshes.shield_sprite_mesh;                                                                               // correct UV slicing for shielded enemies mesh

        if (e.enemy_type == Data::EnemyType::FRIENDLY)                                                                      // check if is friendly npc
            mesh = meshes.friendly_sprite_mesh;                                                                             // correct UV slicing for shielded enemies mesh

        if (!mesh) return;                                                                                                  // skip if mesh missing

        AEGfxTextureSet(tex, e.enemy_animation.animation_uv_x, e.enemy_animation.animation_uv_y);                           // set texture and uv offset

        Graphics::draw_mesh_at(mesh,                                                                                     // draw the enemy sprite at its current position
            e.enemy_position_x,
            e.enemy_position_y,
            EnemyConfig::ENEMY_SIZE_PX, EnemyConfig::ENEMY_SIZE_PX,
            0.0f);
    }

    /*
    @function       Enemy_DrawWord
    @params         e (const Data::Enemy&)
    @brief          Draws the enemy's word centered on the enemy in screen space.

    This function converts the enemy's world position to NDC coordinates, measures the rendered text size,
    then offsets the draw position so the word is centered over the enemy sprite. It skips drawing if the
    enemy is inactive, the font is missing, or the enemy has no assigned word.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_draw_word(const Data::Enemy& e)
    {
        if (!e.enemy_is_active) return;                                                                                     // skip inactive enemies
        if (Graphics::g_font_id < 0) return;                                                                                  // skip if font missing
        if (e.enemy_word_text.empty()) return;                                                                              // skip if no text
        if (e.enemy_is_dying) return;                                                                                       // skip during death animation so word disappears when enemy is killed

        float ndc_x = 0.0f, ndc_y = 0.0f;
        Utility::convert_world_to_ndc(e.enemy_position_x, e.enemy_position_y, ndc_x, ndc_y);                                  // convert enemy position to NDC

        float text_w = 0.0f, text_h = 0.0f;
        AEGfxGetPrintSize(Graphics::g_font_id, e.enemy_word_text.c_str(), EnemyConfig::TEXT_SCALE, &text_w, &text_h);         // measure full word size for centering

        float half_win_h = AEGfxGetWinMaxY();
        float label_offset_ndc = (half_win_h > 0.0f) ? (EnemyConfig::WORD_LABEL_OFFSET_PX / half_win_h) : 0.0f;

        float draw_x = ndc_x - (text_w * 0.5f);                                                                             // center text horizontally
        float draw_y = ndc_y + label_offset_ndc;                                                                            // position text above enemy

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // enable blending for text

        // draw each letter individually with orbit effect applied per letter
        int word_len = (int)e.enemy_word_text.size();                                                                       // number of letters in the word
        float cursor_x = draw_x;                                                                                            // tracks baseline x position as we step through letters

        for (int li = 0; li < word_len; ++li)                                                                               // loop through every letter in the word
        {
            char single[2] = { e.enemy_word_text[(size_t)li], '\0' };                                                       // isolate this letter as a single char string
            float lw = 0.0f, lh = 0.0f;
            AEGfxGetPrintSize(Graphics::g_font_id, single, EnemyConfig::TEXT_SCALE, &lw, &lh);                                // measure this letter's width

            float final_x = cursor_x;                                                                                       // default to baseline x position
            float final_y = draw_y;                                                                                         // default to baseline y position

            if (e.enemy_has_orbit_effect && e.enemy_orbit_radius > 0.001f && !e.enemy_has_landed)                           // only apply orbit if assigned, radius is meaningful, and enemy is still falling
            {
                float angle = e.enemy_orbit_timer * EnemyConfig::ORBIT_SPEED                                                // orbit angle advances over time
                    + (float)li * (6.2831853f / (float)word_len);                                                           // each letter is evenly spaced around the full circle

                float orbit_x = ndc_x + cosf(angle) * e.enemy_orbit_radius;                                                 // orbit x position around enemy center
                float orbit_y = ndc_y + sinf(angle) * e.enemy_orbit_radius;                                                 // orbit y position around enemy center

                float t = e.enemy_orbit_radius / EnemyConfig::ORBIT_RADIUS_MAX_NDC;                                         // blend factor 0 at baseline, 1 at full orbit
                final_x = cursor_x + (orbit_x - cursor_x) * t;                                                              // blend between baseline and orbit x
                final_y = draw_y + (orbit_y - draw_y) * t;                                                                  // blend between baseline and orbit y
            }

            AEGfxPrint(Graphics::g_font_id, single, final_x, final_y, EnemyConfig::TEXT_SCALE, 0, 0, 0, 1);                   // draw letter in black at final blended position
            cursor_x += lw;                                                                                                 // advance baseline cursor to the next letter position
        }
        /*--------------------------------*/
    }

    /*
    @function       Enemy_IsSpawnSpotFree
    @params         spawn_x (float), spawn_y (float), pool (const std::vector<Data::Enemy>&), half_size_with_padding (float)
    @brief          Checks whether a proposed spawn position would overlap any active enemy.

    This function tests the candidate spawn AABB against all active enemies in the pool using
    Utility::Collision_Overlap. It returns true if no overlaps are found, and false if the new enemy
    would collide with an existing one (including padding).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool enemy_is_spawn_spot_free(float spawn_x, float spawn_y, const std::vector<Data::Enemy>& pool, float half_size_with_padding)
    {
        for (const auto& other : pool)                                                                                      // loop through enemies
        {
            if (!other.enemy_is_active) continue;                                                                           // skip inactive enemies

            bool overlaps = Utility::collision_overlap(                                                                     // compute overlap result
                spawn_x, spawn_y,                                                                                           // new spawn center
                half_size_with_padding, half_size_with_padding,                                                             // new spawn half size
                other.enemy_position_x, other.enemy_position_y,                                                             // existing enemy center
                half_size_with_padding, half_size_with_padding                                                              // existing enemy half size approximation
            );

            if (overlaps) return false;                                                                                     // fail if overlap found
        }
        return true;                                                                                                        // pass if no overlaps found
    }

    /*
    @function       Enemy_FindSpawnLane
    @params         pool (const std::vector<Data::Enemy>&), area (const Data::EnemyArea&), overlap_half (float), usable_min_x (float), usable_max_x (float), out_chosen_x (float&), out_chosen_lane (int&)
    @brief          Searches for a free lane and x position to spawn an enemy at the spawn line.

    This function first tries random lanes up to MAX_SPAWN_TRIES, then falls back to scanning all lanes
    in order. Returns true if a valid lane was found, false otherwise. The chosen x and lane index are
    written to out_chosen_x and out_chosen_lane.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool enemy_find_spawn_lane(const std::vector<Data::Enemy>& pool, const Data::EnemyArea& area, float overlap_half, float usable_min_x, float usable_max_x, float& out_chosen_x, int& out_chosen_lane)
    {
        float spawn_y = area.area_spawn_y;                                                                                  // spawn y position

        for (int i = 0; i < EnemyConfig::MAX_SPAWN_TRIES; ++i)                                                              // try random lanes for spawn
        {
            int lane_id = Utility::utility_rand_int(0, EnemyConfig::ENEMY_LANE_COUNT - 1);                                   // pick random lane

            if (!enemy_is_lane_spawn_y_free(lane_id, spawn_y, pool))                                                            // reject if lane is too crowded near spawn
                continue;                                                                                                   // try next

            float lane_x = enemy_get_lane_x(lane_id, usable_min_x, usable_max_x);                                             // compute lane x

            if (enemy_is_spawn_spot_free(lane_x, spawn_y, pool, overlap_half))                                                 // check overlap at lane position
            {
                out_chosen_x = lane_x;                                                                                      // store chosen x
                out_chosen_lane = lane_id;                                                                                  // store chosen lane
                return true;                                                                                                // found a valid spot
            }
        }

        for (int lane_id = 0; lane_id < EnemyConfig::ENEMY_LANE_COUNT; ++lane_id)                                           // fallback: scan lanes in order
        {
            if (!enemy_is_lane_spawn_y_free(lane_id, spawn_y, pool))                                                            // reject if lane is too crowded
                continue;                                                                                                   // keep scanning

            float lane_x = enemy_get_lane_x(lane_id, usable_min_x, usable_max_x);                                             // compute lane x
            if (enemy_is_spawn_spot_free(lane_x, spawn_y, pool, overlap_half))                                                 // check overlap at lane position
            {
                out_chosen_x = lane_x;                                                                                      // store chosen x
                out_chosen_lane = lane_id;                                                                                  // store chosen lane
                return true;                                                                                                // found a valid spot
            }
        }

        return false;                                                                                                       // no valid lane found
    }

    /*
    @function       Enemy_SpawnFromPool
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), word_length (int)
    @brief          Spawns a new enemy into an available pool slot using lane based, non overlapping placement.

    This function pulls a free enemy slot, selects a unique word of the requested length, computes safe
    horizontal bounds based on sprite size and word width, then searches for a valid lane position at the
    spawn line. A spawn is accepted only if the lane has enough vertical spacing near the spawn line and
    the new enemy won't overlap any active enemy. When a valid spot is found, the enemy is created with a
    difficulty-scaled fall speed and inserted into the pool.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_spawn_from_pool(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, int word_length)
    {
        if (free_slots.empty()) return;                                                                                     // skip if no free slots

        std::string unique_word = enemy_pick_unique_word(word_length, bank, pool, true);                                      // choose unique word
        if (unique_word.empty()) return;                                                                                    // skip if no unique word found

        float enemy_half = EnemyConfig::ENEMY_SIZE_PX * 0.5f;                                                               // enemy half size for spacing

        float word_half_w = enemy_get_word_half_width_world(unique_word);                                                       // word half width in world pixels
        float safe_half_w = (word_half_w > enemy_half) ? word_half_w : enemy_half;                                          // use bigger of word or sprite

        float usable_min_x = area.area_min_x + safe_half_w;                                                                 // left usable x bound
        float usable_max_x = area.area_max_x - safe_half_w;                                                                 // right usable x bound
        if (usable_max_x <= usable_min_x) return;                                                                           // skip if there is no usable width

        float padding_half = EnemyConfig::ENEMY_OVERLAP_PADDING_PX * 0.5f;                                                  // half padding for overlap checks
        float overlap_half = enemy_half + padding_half;                                                                     // half size used for overlap tests

        float chosen_x = 0.0f;                                                                                              // chosen spawn x
        int   chosen_lane = -1;                                                                                             // chosen lane id

        if (!enemy_find_spawn_lane(pool, area, overlap_half, usable_min_x, usable_max_x, chosen_x, chosen_lane))              // search for a free lane
            return;                                                                                                         // skip if no valid lane found

        float fall_speed = DifficultySystem::difficulty_get_enemy_fall_speed(word_length);                                     // compute fall speed

        int slot = free_slots.front();                                                                                      // take the oldest free slot
        free_slots.pop_front();                                                                                             // remove that slot from queue

        pool[(size_t)slot] = Enemy_Create(area, word_length, unique_word, fall_speed, chosen_lane);                         // place new enemy in slot

        Data::Enemy& e = pool[(size_t)slot];                                                                                // reference created enemy
        e.enemy_position_x = Utility::utility_clamp(chosen_x, usable_min_x, usable_max_x);                                  // clamp spawn x
        e.enemy_position_y = Utility::utility_clamp(area.area_spawn_y, area.Area_BoxMinY(), area.Area_BoxMaxY());           // clamp spawn y

        e.enemy_type = Data::EnemyType::NORMAL;                                                                             // normal enemy
        e.enemy_death_row_index = 1;                                                                                        // normal death row is row 1
        e.enemy_shield_intact = false;                                                                                      // not shielded
        e.enemy_word_text_alt.clear();                                                                                      // no alt word
    }

    /*
    @function       Enemy_SpawnFriendlyFromPool
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), min_word_len (int), max_word_len (int), speed_multiplier (float);
    @brief          Spawns a friendly enemy into an available pool slot using lane based placement.

    This function works the same as Enemy_SpawnFromPool but creates a friendly enemy. Friendly
    enemies do not damage the player when they land. The speed_multiplier scales the fall speed.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_spawn_friendly_from_pool(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, int min_word_len, int max_word_len, float speed_multiplier)
    {
        if (free_slots.empty()) return;                                                                                     // skip if no free slots

        std::string unique_word = enemy_pick_unique_word_random_length(min_word_len, max_word_len, bank, pool, true, EnemyConfig::MAX_WORD_TRIES);
        if (unique_word.empty()) return;                                                                                    // skip if no unique word found

        int word_length = (int)unique_word.size();                                                                          // word length from chosen word

        float enemy_half = EnemyConfig::ENEMY_SIZE_PX * 0.5f;                                                               // enemy half size for spacing

        float word_half_w = enemy_get_word_half_width_world(unique_word);                                                       // word half width in world pixels
        float safe_half_w = (word_half_w > enemy_half) ? word_half_w : enemy_half;                                          // use bigger of word or sprite

        float usable_min_x = area.area_min_x + safe_half_w;                                                                 // left usable x bound
        float usable_max_x = area.area_max_x - safe_half_w;                                                                 // right usable x bound
        if (usable_max_x <= usable_min_x) return;                                                                           // skip if there is no usable width

        float padding_half = EnemyConfig::ENEMY_OVERLAP_PADDING_PX * 0.5f;                                                  // half padding for overlap checks
        float overlap_half = enemy_half + padding_half;                                                                     // half size used for overlap tests

        float chosen_x = 0.0f;                                                                                              // chosen spawn x
        int   chosen_lane = -1;                                                                                             // chosen lane id

        if (!enemy_find_spawn_lane(pool, area, overlap_half, usable_min_x, usable_max_x, chosen_x, chosen_lane))              // search for a free lane
            return;                                                                                                         // skip if no valid lane found

        float base_speed = DifficultySystem::difficulty_get_enemy_fall_speed(word_length);                                     // compute base fall speed
        float fall_speed = base_speed * speed_multiplier;                                                                   // friendly slightly faster

        int slot = free_slots.front();                                                                                      // take free slot
        free_slots.pop_front();                                                                                             // remove from queue

        pool[(size_t)slot] = Enemy_Create(area, word_length, unique_word, fall_speed, chosen_lane);                         // create enemy

        Data::Enemy& e = pool[(size_t)slot];                                                                                // get reference to the newly created enemy
        e.enemy_animation.animation_columns = 6;                                                                            // friendly sheet has 6 columns
        e.enemy_animation.animation_rows = 3;                                                                               // friendly sheet has 3 rows
        e.enemy_position_x = Utility::utility_clamp(chosen_x, usable_min_x, usable_max_x);                                  // clamp spawn x within usable bounds
        e.enemy_position_y = Utility::utility_clamp(area.area_spawn_y, area.Area_BoxMinY(), area.Area_BoxMaxY());           // clamp spawn y within the enemy area

        e.enemy_type = Data::EnemyType::FRIENDLY;                                                                           // mark as a friendly enemy
        e.enemy_death_row_index = 1;                                                                                        // break free animation
        e.enemy_shield_intact = false;                                                                                      // no shield
        e.enemy_word_text_alt.clear();                                                                                      // no second word

        e.enemy_animation.animation_row_index = 0;                                                                          // start on cage falling animation
        e.enemy_animation.animation_frame_index = 0;                                                                        // start from frame 0
        e.enemy_animation.animation_timer_seconds = 0.0f;                                                                   // reset the frame timer
        Animation::animation_set_frame(e.enemy_animation, 0);                                                                // update UV coords to match the new row and frame
    }

    /*
    @function       Enemy_SpawnShieldedFromPool
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), min_word_len (int), max_word_len (int), speed_multiplier (float);
    @brief          Spawns a shielded enemy into an available pool slot using lane based placement.

    This function spawns a shielded enemy that needs two words to defeat. It picks two unique
    words: the first is shown while the shield is up, the second is revealed after the shield breaks.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_spawn_shielded_from_pool(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, int min_word_len, int max_word_len, float speed_multiplier)
    {
        if (free_slots.empty()) return;                                                                                     // skip if no free slots

        std::string word1 = enemy_pick_unique_word_random_length(min_word_len, max_word_len, bank, pool, true, EnemyConfig::MAX_WORD_TRIES);
        if (word1.empty()) return;                                                                                          // skip if no first word was found

        std::string word2{};                                                                                                // storage for the hidden second word
        for (int tries = 0; tries < EnemyConfig::MAX_WORD_TRIES; ++tries)                                                   // keep trying to find a valid second word
        {
            std::string candidate = enemy_pick_unique_word_random_length(min_word_len, max_word_len, bank, pool, true, 32);     // pick a random candidate word
            if (candidate.empty()) continue;                                                                                // skip if nothing was found this try
            if (candidate == word1) continue;                                                                               // skip if it matches the first word

            word2 = candidate;                                                                                              // found a valid second word
            break;                                                                                                          // stop searching
        }
        if (word2.empty()) return;                                                                                          // fail if no valid second word

        int word_length = (int)word1.size();                                                                                // starts showing word1

        float enemy_half = EnemyConfig::ENEMY_SIZE_PX * 0.5f;                                                               // enemy half size for spacing

        float word_half_w = enemy_get_word_half_width_world(word1);                                                             // measure word1 width (visible)
        float safe_half_w = (word_half_w > enemy_half) ? word_half_w : enemy_half;                                          // use the bigger of word width or sprite size

        float usable_min_x = area.area_min_x + safe_half_w;                                                                 // left usable x bound
        float usable_max_x = area.area_max_x - safe_half_w;                                                                 // right usable x bound
        if (usable_max_x <= usable_min_x) return;                                                                           // skip if there is no usable width

        float padding_half = EnemyConfig::ENEMY_OVERLAP_PADDING_PX * 0.5f;                                                  // half overlap padding
        float overlap_half = enemy_half + padding_half;                                                                     // total half size used for overlap checks

        float chosen_x = 0.0f;                                                                                              // x position of the chosen spawn
        int   chosen_lane = -1;                                                                                             // lane index of the chosen spawn

        if (!enemy_find_spawn_lane(pool, area, overlap_half, usable_min_x, usable_max_x, chosen_x, chosen_lane))              // search for a free lane
            return;                                                                                                         // skip if no valid lane was found

        float base_speed = DifficultySystem::difficulty_get_enemy_fall_speed(word_length);                                     // compute base fall speed from difficulty
        float fall_speed = base_speed * speed_multiplier;                                                                   // apply the speed multiplier

        int slot = free_slots.front();                                                                                      // take the next free pool slot
        free_slots.pop_front();                                                                                             // remove that slot from the queue

        pool[(size_t)slot] = Enemy_Create(area, word_length, word1, fall_speed, chosen_lane);                               // create the enemy in the pool slot

        Data::Enemy& e = pool[(size_t)slot];                                                                                // get reference to the newly created enemy
        e.enemy_animation.animation_columns = 6;                                                                            // shielded sheet has 6 columns
        e.enemy_animation.animation_rows = 3;                                                                               // shielded sheet has 3 rows
        e.enemy_position_x = Utility::utility_clamp(chosen_x, usable_min_x, usable_max_x);                                  // clamp spawn x within usable bounds
        e.enemy_position_y = Utility::utility_clamp(area.area_spawn_y, area.Area_BoxMinY(), area.Area_BoxMaxY());           // clamp spawn y within the enemy area

        e.enemy_type = Data::EnemyType::SHIELDED;                                                                           // mark as shielded enemy
        e.enemy_shield_intact = true;                                                                                       // starts with shield
        e.enemy_word_text_alt = word2;                                                                                      // second word stored
        e.enemy_death_row_index = 2;                                                                                        // shield death row is row 2

        e.enemy_animation.animation_row_index = 0;                                                                          // start on the shielded animation row
        e.enemy_animation.animation_frame_index = 0;                                                                        // start from frame 0
        e.enemy_animation.animation_timer_seconds = 0.0f;                                                                   // reset the frame timer
        Animation::animation_set_frame(e.enemy_animation, 0);                                                                // update UV coords to match the new row and frame
    }

} // EnemySystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      UiSystem
@brief          Contains debug UI drawing helpers used to visualize gameplay state.

This namespace groups together rendering utilities for on-screen debug visuals, such as the enemy area
bounds, lane guides, and a small HUD readout for difficulty/spawn pacing/health.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace UiSystem
{
    /*
    @function       Debug_DrawEnemyAreaBox
    @params         meshes (const Data::Meshes&), area (const Data::EnemyArea&)
    @brief          Draws a semi-transparent debug rectangle showing the enemy movement/spawn area.

    This function computes the bounding box of the enemy area from 'area', then renders a tinted,
    semi-transparent rectangle using the debug mesh. It helps visualize where enemies are allowed
    to spawn, fall, and land.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void debug_draw_enemy_area_box(const Data::Meshes& meshes, const Data::EnemyArea& area)
    {
        float box_min_y = area.Area_BoxMinY();                                                                              // read bottom y
        float box_max_y = area.Area_BoxMaxY();                                                                              // read top y

        float center_x = (area.area_min_x + area.area_max_x) * 0.5f;                                                        // compute box center x
        float center_y = (box_min_y + box_max_y) * 0.5f;                                                                    // compute box center y

        float w = (area.area_max_x - area.area_min_x);                                                                      // compute box width
        float h = (box_max_y - box_min_y);                                                                                  // compute box height

        if (w <= 0.0f || h <= 0.0f) return;                                                                                 // skip invalid box

        Graphics::graphics_set_color_state(0.50f);                                                                            // set semi-transparent draw state
        AEGfxSetColorToMultiply(0.2f, 0.9f, 1.0f, 1.0f);                                                                    // tint debug box color

        Graphics::draw_mesh_at(meshes.debug_box_mesh, center_x, center_y, w, h, 0.0f);                                   // draw debug box

        AEGfxSetColorToMultiply(1, 1, 1, 1);                                                                                // reset tint color
        AEGfxSetTransparency(1.0f);                                                                                         // reset alpha
    }

    /*
    @function       Debug_DrawEnemyLaneLines
    @params         meshes (const Data::Meshes&), area (const Data::EnemyArea&)
    @brief          Draws vertical debug lines to visualize each enemy lane inside the enemy area.

    This function renders one vertical line per lane across the enemy area height, using distinct colors
    to make lanes easy to tell apart. It helps debug lane placement, spawn positions, and spacing rules.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void debug_draw_enemy_lane_lines(const Data::Meshes& meshes, const Data::EnemyArea& area)
    {
        float box_min_y = area.Area_BoxMinY();                                                                              // read bottom y
        float box_max_y = area.Area_BoxMaxY();                                                                              // read top y

        float h = (box_max_y - box_min_y);                                                                                  // compute height
        if (h <= 0.0f) return;                                                                                              // skip invalid height

        float center_y = (box_min_y + box_max_y) * 0.5f;                                                                    // compute center y

        float min_x = area.area_min_x;                                                                                      // read left x
        float max_x = area.area_max_x;                                                                                      // read right x

        float thickness = 4.0f;                                                                                             // thickness of each lane line

        const float lane_colors[EnemyConfig::ENEMY_LANE_COUNT][3] =                                                         // colors for each lane line
        {
            { 1.0f, 0.2f, 0.2f },                                                                                           // lane 0 color
            { 1.0f, 0.6f, 0.2f },                                                                                           // lane 1 color
            { 1.0f, 1.0f, 0.2f },                                                                                           // lane 2 color
            { 0.2f, 1.0f, 0.2f },                                                                                           // lane 3 color
            { 0.2f, 1.0f, 1.0f },                                                                                           // lane 4 color
            { 0.2f, 0.4f, 1.0f },                                                                                           // lane 5 color
            { 0.7f, 0.2f, 1.0f },                                                                                           // lane 6 color
            { 1.0f, 0.2f, 0.8f }                                                                                            // lane 7 color
        };

        Graphics::graphics_set_color_state(0.60f);                                                                            // set semi-transparent draw state

        for (int lane = 0; lane < EnemyConfig::ENEMY_LANE_COUNT; ++lane)                                                    // loop over lanes
        {
            float t = (EnemyConfig::ENEMY_LANE_COUNT <= 1) ? 0.5f :
                (float)lane / (float)(EnemyConfig::ENEMY_LANE_COUNT - 1);                                                   // lane position as 0..1
            float lane_x = Utility::utility_lerp(min_x, max_x, t);                                                          // compute lane x

            AEGfxSetColorToMultiply(lane_colors[lane][0], lane_colors[lane][1], lane_colors[lane][2], 1.0f);                // set line tint

            Graphics::draw_mesh_at(meshes.debug_box_mesh, lane_x, center_y, thickness, h, 0.0f);                         // draw lane line
        }

        AEGfxSetColorToMultiply(1, 1, 1, 1);                                                                                // reset tint color
        AEGfxSetTransparency(1.0f);                                                                                         // reset alpha
    }

    /*
    @function       Debug_DrawDifficultyTopRight
    @params         show (bool), auto_spawn_on (bool), health (const Data::Health&)
    @brief          Draws a debug HUD line showing health, difficulty, spawn pacing, and word length unlocks.

    This function prints a single line of diagnostic text near the top-right area of the screen, including
    current/max HP, the current difficulty value, the current spawn interval, whether auto spawn is enabled,
    and the unlocked word length range.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void debug_draw_difficulty_top_right(bool show, bool auto_spawn_on, const Data::Health& health)
    {
        if (!show) return;                                                                                                  // skip if hidden
        if (Graphics::g_font_id < 0) return;                                                                                  // skip if font missing

        char buffer[220];                                                                                                   // text buffer
        float hp_now = health.health_current;                                                                               // read current health
        float hp_max = health.health_max;                                                                                   // read max health
        float diff = DifficultySystem::difficulty_get();                                                                    // read difficulty value
        float interval = DifficultySystem::difficulty_get_spawn_interval_seconds();                                            // read spawn interval
        const char* auto_text = auto_spawn_on ? "ON" : "OFF";                                                               // text for auto spawn state
        int max_len = DifficultySystem::difficulty_get_unlocked_max_word_length();                                              // read unlocked max word length

        std::snprintf(buffer, sizeof(buffer),                                                                               // format debug line
            "HP: %.0f/%.0f  DIFF: %.1f  SPAWN: %.2fs  AUTO: %s  WORDS: 3-%d",                                               // format string
            hp_now, hp_max, diff, interval, auto_text, max_len);                                                            // values for format

        AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // enable blending for text
        AEGfxPrint(Graphics::g_font_id, buffer, -0.13f, 0.90f, 0.5f, 1.0f, 0.9f, 0.2f, 1.0f);                                 // draw top-right-ish text
    }

    /*
    @function       Ui_DrawHealthBar
    @params         health (Data::Health&), meshes (Data::Meshes&), heart_anim (Data::SpriteAnim&), dt (float)
    @brief          Draws the player health bar as a grid of animated heart sprites.

    This function renders one heart icon per 10 HP of current health, laid out in rows of up to
    ten hearts each. Hearts are only drawn for HP blocks that exist in the current health total,
    so losing health removes hearts and gaining health adds them back. Each heart shows a full
    animated sprite when the corresponding 10 HP block is fully intact, and a static partial frame
    when the block is only partially filled. All heart animations share a single global timer so
    every sprite advances in lockstep.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void ui_draw_health_bar(Data::Health& health, Data::Meshes& meshes, Data::SpriteAnim& heart_anim, float dt)
    {
        if (!Graphics::s_heart_texture) return;                                                                               // skip if heart texture not loaded
        if (!meshes.heart_sprite_mesh) return;                                                                              // skip if heart mesh not built

        // advance the shared animation timer so all hearts animate in lockstep
        Animation::animation_update(heart_anim, dt);                                                                        // advance the shared animation timer each frame

        const int   HEART_COLUMNS = 6;                                                                                      // frames across the sprite sheet
        const float HEART_SIZE_PX = 50.0f;                                                                                  // rendered size of one heart in pixels
        const float HEART_SPACING_PX = 40.0f;                                                                               // center-to-center distance between hearts (2px gap)
        const float HEART_ROW_GAP_PX = 44.0f;                                                                               // center-to-center distance between rows (4px gap)
        const int   HEARTS_PER_ROW = 5;                                                                                     // maximum hearts drawn per row
        const float START_X = -745.0f;                                                                                      // world x of the first heart, aligned to top-left under score
        const float START_Y = 360.0f;                                                                                       // world y of the first row, just below the score text

        float hp_max = health.health_max;                                                                                   // read max health
        float hp_current = Utility::utility_clamp(health.health_current, 0.0f, hp_max);                                     // read current health, clamped

        // only draw hearts that exist within the current HP total:
        // a full 10-HP block gets one heart, and a leftover partial block also gets one heart
        int total_hearts = (int)(hp_current / 10.0f);                                                                       // one heart per full 10 HP block
        if (hp_current > (float)total_hearts * 10.0f) total_hearts += 1;                                                    // add one extra heart for any partial block
        // total_hearts is now 0 when the player is dead and grows as HP is added back

        float uv_w = heart_anim.Anim_UvWidth();                                                                             // uv width of one frame column

        Graphics::graphics_set_sprite_state();                                                                                // switch to textured sprite draw mode

        for (int i = 0; i < total_hearts; ++i)                                                                              // loop only over hearts that should be visible
        {
            int col = i % HEARTS_PER_ROW;                                                                                   // column index within the current row
            int row = i / HEARTS_PER_ROW;                                                                                   // row index (0 is the topmost row)

            float heart_x = START_X + (float)col * HEART_SPACING_PX;                                                        // world x center of this heart
            float heart_y = START_Y - (float)row * HEART_ROW_GAP_PX;                                                        // world y center of this heart (rows go downward)

            float block_hp_min = (float)(i) * 10.0f;                                                                        // hp at the start of this block
            float block_hp_max = (float)(i + 1) * 10.0f;                                                                    // hp at the end of this block

            float uv_x = 0.0f;                                                                                              // default UV x to frame 0 (full animated heart)

            if (hp_current < block_hp_max)
            {
                // block partially filled (this is the last visible heart): static frame proportional to fill
                float fill = (hp_current - block_hp_min) / 10.0f;                                                           // 1.0 = full, 0.0 = empty
                fill = Utility::utility_clamp(fill, 0.0f, 1.0f);                                                            // clamp fill ratio to valid range
                int partial_frame = (int)((1.0f - fill) * (float)(HEART_COLUMNS - 1) + 0.5f);                               // round to nearest depletion frame
                if (partial_frame < 0)                 partial_frame = 0;                                                   // clamp low end
                if (partial_frame > HEART_COLUMNS - 2) partial_frame = HEART_COLUMNS - 2;                                   // clamp to second-to-last frame (last frame = fully empty, never shown)
                uv_x = (float)partial_frame * uv_w;                                                                         // static partial frame UV
            }
            else
            {
                // block fully intact: cycle all frames using the shared animation index
                uv_x = (float)heart_anim.animation_frame_index * uv_w;                                                      // animated UV from shared timer
            }

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                // draw hearts in their natural colors
            AEGfxTextureSet(Graphics::s_heart_texture, uv_x, 0.0f);                                                           // bind texture at the chosen frame, row 0
            Graphics::draw_mesh_at(meshes.heart_sprite_mesh, heart_x, heart_y, HEART_SIZE_PX, HEART_SIZE_PX, 0.0f);      // draw the heart sprite
        }

        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);                                                                    // restore color multiply to neutral
    }
} // UiSystem namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      EnemyManage
@brief          Handles game startup and shutdown tasks, including engine setup and resource management.

This namespace contains high-level game bootstrapping code: initializing the AE engine/window,
loading required assets (words, textures, meshes, fonts), and cleaning everything up safely on exit.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace EnemyManage
{
    /*
    @function       EnemyManage_InitEngine
    @params         bank (Data::WordBank&)
    @brief          Initializes the engine, window settings, random seed, word data, and font resources.

    This function boots the AE engine and creates the game window, sets the window title, seeds the random
    number generator, loads all word lists into the provided WordBank, and loads the font used for rendering
    enemy words and debug text.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_manage_init_engine(Data::WordBank& bank)
    {
        srand((unsigned)time(NULL));                                                                                        // seed random generator

        Word::word_load_all(bank);                                                                                           // load word lists

        if (Graphics::g_font_id >= 0) {
            AEGfxDestroyFont(Graphics::g_font_id);
        }

        Graphics::g_font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 80);                                                   // load font
        AE_ASSERT_MESG(Graphics::g_font_id >= 0, "Failed to load font: Assets/Pixel-Game.otf");                               // stop if font fails
    }

    /*
    @function       EnemyManage_LoadMeshes
    @params         enemy_anim (const Data::SpriteAnim&)
    @brief          Loads enemy textures and builds the meshes needed for rendering.

    This function loads the enemy sprite textures for word lengths 3 through 8, builds the textured quad
    mesh used to draw animated enemy sprites (using the sprite sheet UV frame size from 'enemy_anim'),
    and builds a unit rectangle mesh used for debug drawing. It returns a Data::Meshes struct containing
    the created mesh handles.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    Data::Meshes enemy_manage_load_meshes(const Data::SpriteAnim& enemy_anim)
    {
        Data::Meshes meshes{};                                                                                              // create mesh container

        Graphics::s_friendly_texture = AEGfxTextureLoad("Assets/princessEnemy.png");                                          // load princess npc texture
        AE_ASSERT_MESG(Graphics::s_friendly_texture, "Failed to load: Assets/princessEnemy.png");                             // stop if missing

        Graphics::s_shield_texture = AEGfxTextureLoad("Assets/shieldEnemy.png");                                              // load shield enemy texture
        AE_ASSERT_MESG(Graphics::s_shield_texture, "Failed to load: Assets/shieldEnemy.png");                                 // stop if missing

        Graphics::s_enemy_textures[3] = AEGfxTextureLoad("Assets/3letterEnemy.png");                                          // load 3-letter enemy texture
        AE_ASSERT_MESG(Graphics::s_enemy_textures[3], "Failed to load: Assets/3letterEnemy.png");                             // stop if missing

        Graphics::s_enemy_textures[4] = AEGfxTextureLoad("Assets/4letterEnemy.png");                                          // load 4-letter enemy texture
        AE_ASSERT_MESG(Graphics::s_enemy_textures[4], "Failed to load: Assets/4letterEnemy.png");                             // stop if missing

        Graphics::s_enemy_textures[5] = AEGfxTextureLoad("Assets/5letterEnemy.png");                                          // load 5-letter enemy texture
        AE_ASSERT_MESG(Graphics::s_enemy_textures[5], "Failed to load: Assets/5letterEnemy.png");                             // stop if missing

        Graphics::s_enemy_textures[6] = AEGfxTextureLoad("Assets/6letterEnemy.png");                                          // load 6-letter enemy texture
        AE_ASSERT_MESG(Graphics::s_enemy_textures[6], "Failed to load: Assets/6letterEnemy.png");                             // stop if missing

        Graphics::s_enemy_textures[7] = AEGfxTextureLoad("Assets/7letterEnemy.png");                                          // load 7-letter enemy texture
        AE_ASSERT_MESG(Graphics::s_enemy_textures[7], "Failed to load: Assets/7letterEnemy.png");                             // stop if missing

        Graphics::s_enemy_textures[8] = AEGfxTextureLoad("Assets/8letterEnemy.png");                                          // load 8-letter enemy texture
        AE_ASSERT_MESG(Graphics::s_enemy_textures[8], "Failed to load: Assets/8letterEnemy.png");                             // stop if missing

        Graphics::s_heart_texture = AEGfxTextureLoad("Assets/hearts.png");                                                    // load heart health icon sprite sheet
        AE_ASSERT_MESG(Graphics::s_heart_texture, "Failed to load: Assets/hearts.png");                                       // stop if missing

        // build 2-row mesh (normal enemies)
        {
            Data::SpriteAnim anim2 = enemy_anim;
            anim2.animation_columns = 6;
            anim2.animation_rows = 2;

            meshes.enemy_sprite_mesh = Graphics::Mesh_BuildSpriteQuad(anim2.Anim_UvWidth(), anim2.Anim_UvHeight(), 0xFFFFFFFF);
        }

        // build 3-row mesh (shield enemy)
        {
            Data::SpriteAnim anim3 = enemy_anim;
            anim3.animation_columns = 6;
            anim3.animation_rows = 3;

            meshes.shield_sprite_mesh = Graphics::Mesh_BuildSpriteQuad(anim3.Anim_UvWidth(), anim3.Anim_UvHeight(), 0xFFFFFFFF);
        }

        // build 3-row mesh (friendly)
        {
            Data::SpriteAnim anim3f = enemy_anim;
            anim3f.animation_columns = 6;
            anim3f.animation_rows = 3;

            meshes.friendly_sprite_mesh = Graphics::Mesh_BuildSpriteQuad(anim3f.Anim_UvWidth(), anim3f.Anim_UvHeight(), 0xFFFFFFFF);
        }

        // build 1-row mesh (heart health icons, 6 frames across a single row)
        {
            Data::SpriteAnim heart_anim_temp{};
            heart_anim_temp.animation_columns = 6;
            heart_anim_temp.animation_rows = 1;

            meshes.heart_sprite_mesh = Graphics::Mesh_BuildSpriteQuad(heart_anim_temp.Anim_UvWidth(), heart_anim_temp.Anim_UvHeight(), 0xFFFFFFFF);
        }

        meshes.debug_box_mesh = Graphics::Mesh_BuildUnitRect(0xFFFFFFFF);                                                   // build debug rectangle mesh
        return meshes;                                                                                                      // return loaded meshes
    }

    /*
    @function       EnemyManage_Free
    @params         meshes (Data::Meshes&), pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&)
    @brief          Deactivates all enemies in the pool, rebuilds the free slot queue, and releases
                    all loaded meshes, textures, and font resources used by the game.

    Every slot is reset to a clean inactive state regardless of enemy type (normal, friendly,
    shielded). The free_slots queue is rebuilt from scratch so the pool is ready for re-init
    without needing a full resize. Mesh pointers are reset back to safe defaults to prevent
    dangling references.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void enemy_manage_free(Data::Meshes& meshes, std::vector<Data::Enemy>& pool, std::deque<int>& free_slots)
    {
        free_slots.clear();                                                                                                 // discard any stale indices

        for (int i = 0; i < (int)pool.size(); ++i)
        {
            Data::Enemy& e = pool[(size_t)i];
            e.enemy_is_active = false;                                                                                      // mark slot inactive
            e.enemy_is_dying = false;                                                                                       // clear dying flag
            e.enemy_has_landed = false;                                                                                     // clear landed flag
            e.enemy_did_damage = false;                                                                                     // clear damage flag
            e.enemy_word_text.clear();                                                                                      // clear word text
            e.enemy_word_text_alt.clear();                                                                                  // clear alternate word text
            e.enemy_word_length = 0;                                                                                        // reset word length
            e.enemy_lane_id = -1;                                                                                           // reset lane assignment
            e.enemy_shield_intact = false;                                                                                  // clear shield flag
            e.enemy_dying_linger_timer_seconds = 0.0f;                                                                      // reset linger timer

            // reset letter orbit effect state so pool slot is fully clean
            e.enemy_orbit_timer = 0.0f;                                                                                     // reset orbit angle timer
            e.enemy_orbit_radius = 0.0f;                                                                                    // reset orbit radius
            e.enemy_orbit_idle_timer = 0.0f;                                                                                // reset idle timer
            e.enemy_orbit_phase = 0;                                                                                        // reset to idle phase
            e.enemy_has_orbit_effect = false;                                                                               // clear orbit effect flag
            e.enemy_orbit_initialised = false;                                                                              // clear initialised flag so next spawn re-rolls
            e.enemy_is_targeted = false;                                                                                    // clear targeted flag

            free_slots.push_back(i);                                                                                        // return slot to the free queue
        }

        if (meshes.debug_box_mesh) AEGfxMeshFree(meshes.debug_box_mesh);                                                    // free debug mesh
        if (meshes.enemy_sprite_mesh) AEGfxMeshFree(meshes.enemy_sprite_mesh);                                              // free enemy mesh
        if (meshes.shield_sprite_mesh) AEGfxMeshFree(meshes.shield_sprite_mesh);                                            // free shield mesh
        if (meshes.friendly_sprite_mesh) AEGfxMeshFree(meshes.friendly_sprite_mesh);                                        // free friendly mesh
        if (meshes.heart_sprite_mesh) AEGfxMeshFree(meshes.heart_sprite_mesh);                                              // free heart health icon mesh
        meshes = {};                                                                                                        // reset mesh pointers to defaults

        for (int i = 3; i <= 8; ++i)                                                                                        // loop through enemy texture slots
        {
            if (Graphics::s_enemy_textures[i])                                                                                // check if texture exists
            {
                AEGfxTextureUnload(Graphics::s_enemy_textures[i]);                                                            // unload texture
                Graphics::s_enemy_textures[i] = nullptr;                                                                      // clear pointer
            }
        }

        if (Graphics::s_friendly_texture) AEGfxTextureUnload(Graphics::s_friendly_texture);                                     // unload princess npc texture
        Graphics::s_friendly_texture = nullptr;                                                                               // clear pointer

        if (Graphics::s_shield_texture) AEGfxTextureUnload(Graphics::s_shield_texture);                                         // unload shield enemy texture
        Graphics::s_shield_texture = nullptr;                                                                                 // clear pointer

        if (Graphics::s_heart_texture) AEGfxTextureUnload(Graphics::s_heart_texture);                                           // unload heart health icon texture
        Graphics::s_heart_texture = nullptr;                                                                                  // clear pointer

        if (Graphics::g_font_id >= 0)                                                                                         // check if font exists
        {
            AEGfxDestroyFont(Graphics::g_font_id);                                                                            // destroy font
            Graphics::g_font_id = -1;                                                                                         // reset font id
        }

        std::deque<int>().swap(free_slots);                                                                                 // release free_slots internal buffer
        std::vector<Data::Enemy>().swap(pool);                                                                              // release enemy pool internal buffer
    }
} // EnemyManage namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/