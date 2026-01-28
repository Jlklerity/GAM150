    /*!
    @file           Enemy.cpp
    @author         Wong Zi Yuan (w.ziyuan@digipen.edu)
    @course         CSD1451
    @section        A
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/

    #include "Enemy.h"      // Enemy API

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

        // Difficulty tuning
        const float DIFFICULTY_RAMP_PER_SECOND = 0.15f;                                                                         // difficulty gain per second
        const float DIFFICULTY_MAX = 30.0f;                                                                                     // cap for difficulty value

        // Random spawn tuning
        const float RANDOM_SPAWN_INTERVAL_START_SECONDS = 2.0f;                                                                 // starting spawn interval
        const float RANDOM_SPAWN_INTERVAL_MIN_SECONDS = 0.85f;                                                                  // minimum spawn interval
        const float RANDOM_SPAWN_INTERVAL_DECREASE_PER_LEVEL = 0.030f;                                                          // interval decrease per difficulty level

        // Fall speed tuning
        const float ENEMY_FALL_SPEED_START = 48.0f;                                                                             // starting fall speed
        const float ENEMY_FALL_SPEED_INCREASE_PER_LEVEL = 1.35f;                                                                // fall speed increase per difficulty level

        // Death drop tuning
        const float ENEMY_DEATH_FALL_SPEED = 850.0f;                                                                            // fast drop speed when killed
        const float ENEMY_DEATH_LINGER_SECONDS = 0.5f;                                                                          // time to keep a killed enemy visible before removing it

        // Spacing
        const float ENEMY_OVERLAP_PADDING_PX = 12.0f;                                                                           // extra padding between enemies

        // Lane system
        const int   ENEMY_LANE_COUNT = 8;                                                                                       // number of horizontal lanes
        const float ENEMY_LANE_MIN_GAP_PX = ENEMY_SIZE_PX + ENEMY_OVERLAP_PADDING_PX;                                           // min vertical spacing per lane
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

        @usage          Utility_Lerp, Difficulty_Add, Difficulty_GetEnemyFallSpeed, Health_Add, Enemy_Update, Enemy_SpawnFromPool

        This function returns 'minimum' if 'value' is below the range, 'maximum' if it is above the range,
        or the original 'value' if it is already within [minimum, maximum].
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        float Utility_Clamp(float value, float minimum, float maximum)
        {
            if (value < minimum) return minimum;                                                                                // clamp low end
            if (value > maximum) return maximum;                                                                                // clamp high end
            return value;                                                                                                       // return clamped or original value
        }

        /*
        @function       Utility_Lerp
        @params         a (float), to (float), t (float)
        @brief          Linearly interpolates between two float values.

        @usage          Difficulty_GetEnemyFallSpeed, Enemy_GetLaneX, Debug_DrawEnemyLaneLines

        This function blends from 'a' to 'b' using 't' as the interpolation factor.
        't' is clamped to [0, 1], where 0 returns 'a', 1 returns 'b', and values in-between return a mix.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        float Utility_Lerp(float a, float b, float t)
        {
            t = Utility_Clamp(t, 0.0f, 1.0f);                                                                                   // keep blend factor within 0 to 1
            return a + (b - a) * t;                                                                                             // return blended value
        }

        /*
        @function       Utility_RandInt
        @params         minimum (float), maximum_inclusive (float)
        @brief          Returns a random integer within an inclusive range.

        @usage          Word_PickRandom, Difficulty_GetRandomWordLength, Enemy_SpawnFromPool

        This function generates a uniformly distributed integer in [minimum, maximum_inclusive].
        If the range is invalid (maximum_inclusive < minimum), it safely returns 'minimum'.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        int Utility_RandInt(int minimum, int maximum_inclusive)
        {
            if (maximum_inclusive < minimum) return minimum;                                                                    // handle invalid range safely
            int range = maximum_inclusive - minimum + 1;                                                                        // compute inclusive range size
            return minimum + (rand() % range);                                                                                  // return random value in range
        }

        /*
        @function       Input_CtrlDown
        @brief          Checks whether either Control key is currently held down.

        @usage          wWinMain

        This function returns true if the left CTRL or right CTRL key is pressed in the current frame,
        otherwise returns false.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        bool Input_CtrlDown()
        {
            return AEInputCheckCurr(AEVK_LCTRL) || AEInputCheckCurr(AEVK_RCTRL);                                                // return true if either ctrl is held
        }

        /*
        @function       Convert_WorldToNdc
        @params         world_x (float), world_y (float), ndc_x (float&), ndc_y (float&)
        @brief          Converts world-space pixel coordinates into normalized device coordinates (NDC).

        @usage          Enemy_DrawWord

        This function maps a position given in world pixels to NDC space, where the window center is (0, 0)
        and the visible range is typically -1..1 on each axis. The result is written to 'ndc_x' and 'ndc_y'.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Convert_WorldToNdc(float world_x, float world_y, float& ndc_x, float& ndc_y)
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

        @usage          Enemy_IsSpawnSpotFree

        This function performs an AABB overlap test using the centers and half-extents of two rectangles.
        It returns true if the rectangles intersect (including touching edges), otherwise false.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        bool Collision_Overlap(float a_center_x, float a_center_y, float a_half_width, float a_half_height, float b_center_x, float b_center_y, float b_half_width, float b_half_height)
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

        @usage          Word_LoadFromFile

        This function modifies the input string directly by uppercasing each character using 'std::toupper'.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void String_ToUpperInPlace(std::string& s)
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

        @usage          Word_LoadAll

        This function reads whitespace-separated words from the file at 'file_path', keeps only words whose
        length matches 'expected_length', converts them to uppercase, and returns them as a vector.
        If the file cannot be opened, it returns an empty list.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        std::vector<std::string> Word_LoadFromFile(const char* file_path, int expected_length)
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
                    Utility::String_ToUpperInPlace(candidate_word);                                                             // convert word to uppercase
                    loaded_words.push_back(candidate_word);                                                                     // store word in list
                }
            }
            return loaded_words;                                                                                                // return list of loaded words
        }

        /*
        @function       Word_LoadAll
        @params         word_bank (Data::WordBank&)
        @brief          Loads all word lists (3 to 8 letters) into the WordBank.

        @usage          Game_InitEngine

        This function populates the WordBank by loading words from the corresponding asset files for each
        supported length (3–8). If any file fails to load or results in an empty list, a small fallback
        set is assigned so the game can still run.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Word_LoadAll(Data::WordBank& word_bank)
        {
            word_bank.words_3 = Word_LoadFromFile("Assets/3letterWords.txt", 3);                                                // load 3-letter words
            word_bank.words_4 = Word_LoadFromFile("Assets/4letterWords.txt", 4);                                                // load 4-letter words
            word_bank.words_5 = Word_LoadFromFile("Assets/5letterWords.txt", 5);                                                // load 5-letter words
            word_bank.words_6 = Word_LoadFromFile("Assets/6letterWords.txt", 6);                                                // load 6-letter words
            word_bank.words_7 = Word_LoadFromFile("Assets/7letterWords.txt", 7);                                                // load 7-letter words
            word_bank.words_8 = Word_LoadFromFile("Assets/8letterWords.txt", 8);                                                // load 8-letter words

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

        @usage          Enemy_PickUniqueWord

        This function selects and returns a const reference to the appropriate word vector inside 'word_bank'
        based on 'word_length' (3–8). If the length is not 3–7, it defaults to returning the 8-letter list.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        const std::vector<std::string>& Word_GetListByLength(const Data::WordBank& word_bank, int word_length)
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

        @usage          Enemy_PickUniqueWord

        This function returns a randomly selected string from 'words'.
        If the list is empty, it returns a placeholder string ("????") to avoid crashes.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        std::string Word_PickRandom(const std::vector<std::string>& words)
        {
            if (words.empty()) return "????";                                                                                   // return placeholder if list is empty
            int random_index = Utility::Utility_RandInt(0, (int)words.size() - 1);                                              // pick random index
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
        AEGfxTexture* enemy_textures[9] = { nullptr };                                                                          // enemy textures indexed by word length
        s8 font_id = -1;                                                                                                        // font handle id

        /*
        @function       Graphics_GetEnemyTextureByWordLength
        @params         word_length (int)
        @brief          Returns the enemy sprite texture that matches a given word length.

        @usage          Enemy_Draw

        This function clamps 'word_length' to the supported range (3–8) and returns the corresponding
        texture from the enemy texture table. It is used to select the correct enemy sprite based on
        the length of the word assigned to that enemy.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        AEGfxTexture* Graphics_GetEnemyTextureByWordLength(int word_length)
        {
            if (word_length < 3) word_length = 3;                                                                               // clamp minimum length
            if (word_length > 8) word_length = 8;                                                                               // clamp maximum length
            return enemy_textures[word_length];                                                                                 // return the matching texture
        }

        /*
        @function       Graphics_SetColorState
        @params         alpha_value (float)
        @brief          Configures the graphics pipeline for solid-color rendering with transparency.

        @usage          Debug_DrawEnemyAreaBox, Debug_DrawEnemyLaneLines

        This function switches the renderer to color mode, enables alpha blending, and applies the requested
        transparency value. It also resets the multiply/add color modifiers to neutral so debug shapes draw
        with predictable colors.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Graphics_SetColorState(float alpha_value)
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

        @usage          Enemy_Draw

        This function switches the renderer to texture mode, enables alpha blending, resets transparency
        to fully opaque, and clears any color multiply/add modifiers so sprites render normally.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Graphics_SetSpriteState()
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

        @usage          Game_LoadMeshes

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

        @usage          Game_LoadMeshes

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

        @usage          Enemy_Draw, Debug_DrawEnemyAreaBox, Debug_DrawEnemyLaneLines

        This function builds a 2D transform (scale -> rotate -> translate), applies it to the renderer,
        and draws the provided mesh using triangle mode.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Draw_DrawMeshAt(AEGfxVertexList* mesh, float position_x, float position_y, float scale_x, float scale_y, float rotation_radians)
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
        float difficulty_value = 0.0f;                                                                                          // current difficulty value

        /*
        @function       Difficulty_Get
        @brief          Returns the current difficulty value.

        @usage          Debug_DrawDifficultyTopRight

        This function provides read-only access to the internal difficulty variable so other systems
        (such as debug UI) can display or use the current difficulty level.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        float Difficulty_Get() {
            return difficulty_value;                                                                                            // read difficulty value
        }

        /*
        @function       Difficulty_Add
        @params         amount (float)
        @brief          Adjusts the current difficulty value and clamps it to a valid range.

        @usage          Difficulty_UpdateOverTime, wWinMain

        This function adds a signed amount to the internal difficulty value, then clamps it between 0 and
        DIFFICULTY_MAX to prevent the difficulty from going negative or exceeding the configured cap.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Difficulty_Add(float amount)
        {
            difficulty_value += amount;                                                                                         // add amount to difficulty
            difficulty_value = Utility::Utility_Clamp(difficulty_value, 0.0f, EnemyConfig::DIFFICULTY_MAX);                     // clamp to valid range
        }

        /*
        @function       Difficulty_UpdateOverTime
        @params         dt (float)
        @brief          Increases difficulty gradually over time.

        @usage          wWinMain

        This function ramps the difficulty based on elapsed time by adding a fixed rate
        (DIFFICULTY_RAMP_PER_SECOND) scaled by delta time.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Difficulty_UpdateOverTime(float dt)
        {
            Difficulty_Add(EnemyConfig::DIFFICULTY_RAMP_PER_SECOND * dt);                                                       // add difficulty based on elapsed time
        }

        /*
        @function       Difficulty_GetSpawnIntervalSeconds
        @brief          Calculates the current enemy spawn interval based on difficulty.

        @usage          Debug_DrawDifficultyTopRight, wWinMain

        This function reduces the spawn interval as difficulty increases, making enemies spawn more often.
        The interval is clamped so it never goes below RANDOM_SPAWN_INTERVAL_MIN_SECONDS.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        float Difficulty_GetSpawnIntervalSeconds()
        {
            float interval_decrease = difficulty_value * EnemyConfig::RANDOM_SPAWN_INTERVAL_DECREASE_PER_LEVEL;                 // how much the interval shrinks from difficulty
            float interval = EnemyConfig::RANDOM_SPAWN_INTERVAL_START_SECONDS - interval_decrease;                              // compute interval
            if (interval < EnemyConfig::RANDOM_SPAWN_INTERVAL_MIN_SECONDS)                                                      // check minimum
                interval = EnemyConfig::RANDOM_SPAWN_INTERVAL_MIN_SECONDS;                                                      // clamp to minimum interval
            return interval;                                                                                                    // return final interval
        }

        /*
        @function       Difficulty_GetUnlockedMaxWordLength
        @brief          Returns the maximum word length currently unlocked by the difficulty value.

        @usage          Difficulty_GetRandomWordLength, Debug_DrawDifficultyTopRight

        This function maps the current difficulty into tiers that gradually unlock longer words.
        As difficulty increases, the allowed maximum word length increases from 3 up to 8.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        int Difficulty_GetUnlockedMaxWordLength()
        {
            float d = difficulty_value;                                                                                         // copy difficulty value
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

        @usage          wWinMain

        This function chooses a random word length between 3 and the currently unlocked maximum word length
        (determined by difficulty). It is used to vary enemy word lengths while still respecting the
        difficulty progression.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        int Difficulty_GetRandomWordLength()
        {
            int min_word_length = 3;                                                                                            // minimum word length
            int max_word_length = Difficulty_GetUnlockedMaxWordLength();                                                        // maximum word length based on difficulty
            return Utility::Utility_RandInt(min_word_length, max_word_length);                                                  // pick random length in range
        }

        /*
        @function       Difficulty_GetEnemyFallSpeed
        @params         word_length (int)
        @brief          Computes an enemy's fall speed based on current difficulty and word length.

        @usage          Enemy_SpawnFromPool

        This function starts from a base fall speed, increases it with difficulty, then applies a word-length
        multiplier (shorter words fall faster, longer words fall slower). The multiplier effect ramps in as
        difficulty increases, so early gameplay stays gentler while later gameplay becomes more punishing.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        float Difficulty_GetEnemyFallSpeed(int word_length)
        {
            float fall_speed_bonus = difficulty_value * EnemyConfig::ENEMY_FALL_SPEED_INCREASE_PER_LEVEL;                       // bonus speed from difficulty scaling
            float base_speed = EnemyConfig::ENEMY_FALL_SPEED_START + fall_speed_bonus;                                          // base speed from difficulty

            float t = Utility::Utility_Clamp(difficulty_value / 14.0f, 0.0f, 1.0f);                                             // normalize difficulty into 0..1

            float target_multiplier = 1.0f;                                                                                     // default multiplier
            if (word_length == 3) target_multiplier = 1.20f;                                                                    // 3-letter enemies fall faster
            else if (word_length == 4) target_multiplier = 1.14f;                                                               // 4-letter enemies fall faster
            else if (word_length == 5) target_multiplier = 1.08f;                                                               // 5-letter enemies fall slightly faster
            else if (word_length == 6) target_multiplier = 1.02f;                                                               // 6-letter enemies fall near base speed
            else if (word_length == 7) target_multiplier = 0.96f;                                                               // 7-letter enemies fall slightly slower
            else target_multiplier = 0.90f;                                                                                     // 8-letter enemies fall slower

            float multiplier = Utility::Utility_Lerp(1.0f, target_multiplier, t);                                               // blend multiplier based on difficulty
            return base_speed * multiplier;                                                                                     // return final fall speed
        }
    } // DifficultySystem namespace end
    /*___________________________________________________________________________________________________________________________________________________________________________________________________*/

    /*
    @namespace      HealthSystem
    @brief          Manages player health updates and clamping.

    This namespace contains helper functions for modifying the player's health safely, ensuring the
    current health value stays within a valid range (0 to max).
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    namespace HealthSystem
    {
        void Health_Init(Data::Health& health) {
            health.health_current = health.health_max;
        }

        /*
        @function       Health_Add
        @params         h (Data::Health&), amount (float)
        @brief          Adds (or subtracts) health and clamps the result to a valid range.

        @usage          Enemy_Update, wWinMain

        This function applies a signed health change to 'h.health_current' and then clamps it between
        0 and 'h.health_max' to prevent underflow or overflow.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Health_Add(Data::Health& h, float amount)
        {
            h.health_current += amount;                                                                                         // add change to current health
            h.health_current = Utility::Utility_Clamp(h.health_current, 0.0f, h.health_max);                                    // clamp to valid range
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
        @function       Animation_SetFrame
        @params         anim (Data::SpriteAnim&), frame_index (int)
        @brief          Forces the animation to a specific frame and updates the UV coordinates.

        @usage          Enemy_Killed

        This function directly sets the current animation frame index (typically to show a specific pose, like
        a death frame), then recalculates the sprite sheet UV offsets based on the current column and the
        selected animation row.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Animation_SetFrame(Data::SpriteAnim& anim, int frame_index)
        {
            anim.animation_frame_index = frame_index;                                                                           // set the current frame index directly

            int current_column = anim.animation_frame_index % anim.animation_columns;                                           // compute column from frame index
            int current_row = anim.animation_row_index;                                                                         // use the currently selected row

            anim.animation_uv_x = current_column * anim.Anim_UvWidth();                                                         // update UV x offset for this frame
            anim.animation_uv_y = current_row * anim.Anim_UvHeight();                                                           // update UV y offset for this row
        }

        /*
        @function       Animation_Update
        @params         anim (Data::SpriteAnim&), dt (float)
        @brief          Advances a sprite animation and updates its UV coordinates.

        @usage          Enemy_Update

        This function accumulates delta time, advances the animation frame when the frame duration is reached,
        wraps the frame index back to 0 when it exceeds the number of frames, and then updates the sprite sheet
        UV offsets using the current frame column and the selected animation row.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Animation_Update(Data::SpriteAnim& anim, float dt)
        {
            anim.animation_timer_seconds += dt;                                                                                 // add delta time to timer
            if (anim.animation_timer_seconds < anim.animation_frame_duration_seconds)                                           // check if frame time not reached
                return;                                                                                                         // exit early if still waiting

            anim.animation_timer_seconds = 0.0f;                                                                                // reset timer after advancing frame

            anim.animation_frame_index++;                                                                                       // move to next frame
            if (anim.animation_frame_index >= anim.Anim_FrameCount())                                                           // wrap if beyond last frame
                anim.animation_frame_index = 0;                                                                                 // reset to first frame

            int current_column = anim.animation_frame_index % anim.animation_columns;                                           // compute column index
            int current_row = anim.animation_row_index;                                                                         // use the selected row of the sprite sheet

            anim.animation_uv_x = current_column * anim.Anim_UvWidth();                                                         // set uv x offset for frame
            anim.animation_uv_y = current_row * anim.Anim_UvHeight();                                                           // set uv y offset for frame
        }
    } // Animation namespace end
    /*___________________________________________________________________________________________________________________________________________________________________________________________________*/

    /*
    @namespace      EnemySystem
    @brief          Manages enemy spawning, updating, and rendering using a pooled lane-based system.

    This namespace contains the core logic for enemies: picking unique words, finding safe spawn positions,
    creating enemies from a pool, updating movement/state (including death drops and health damage), resolving
    lane spacing to prevent stacking, and drawing both sprites and their word text.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    namespace EnemySystem
    {
        /*
        @function       Enemy_IsWordInUse
        @params         pool (const std::vector<Data::Enemy>&), word_text (const std::string&)
        @brief          Checks whether a word is already assigned to an active enemy.

        @usage          Enemy_PickUniqueWord

        This function scans the enemy pool and returns true if any active enemy currently has 'word_text'
        as its assigned word. Inactive enemies are ignored.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        bool Enemy_IsWordInUse(const std::vector<Data::Enemy>& pool, const std::string& word_text)
        {
            for (const auto& e : pool)                                                                                          // loop through enemy pool
            {
                if (!e.enemy_is_active) continue;                                                                               // skip inactive enemies
                if (e.enemy_word_text == word_text) return true;                                                                // report word is already used
            }
            return false;                                                                                                       // report word is not used
        }

        /*
        @function       Enemy_PickUniqueWord
        @params         word_length (int), bank (const Data::WordBank&), pool (const std::vector<Data::Enemy>&)
        @brief          Selects a random word of a given length that is not currently used by any active enemy.

        @usage          Enemy_SpawnFromPool

        This function looks up the word list for 'word_length', then repeatedly picks random candidates until it
        finds one that is not already assigned to an active enemy (up to MAX_WORD_TRIES attempts). If it cannot
        find a unique word, it returns an empty string.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        std::string Enemy_PickUniqueWord(int word_length, const Data::WordBank& bank, const std::vector<Data::Enemy>& pool)
        {
            const auto& list = Word::Word_GetListByLength(bank, word_length);                                                   // fetch list for desired length

            for (int i = 0; i < EnemyConfig::MAX_WORD_TRIES; ++i)                                                               // try several times to find a unique word
            {
                std::string candidate = Word::Word_PickRandom(list);                                                            // pick a random word
                if (!Enemy_IsWordInUse(pool, candidate))                                                                        // check if word is unused
                    return candidate;                                                                                           // return unique word
            }
            return std::string();                                                                                               // return empty string on failure
        }

        /*
        @function       Enemy_GetWordHalfWidthWorld
        @params         word (const std::string&)
        @brief          Calculates the half-width of a word in world pixels for spacing checks.

        @usage          Enemy_SpawnFromPool

        This function measures the rendered width of 'word' using the current font and TEXT_SCALE, then converts
        that screen-space size into world pixels. It is used to keep spawned enemies (and their text) within
        usable horizontal bounds and to help avoid overlaps.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        float Enemy_GetWordHalfWidthWorld(const std::string& word)
        {
            if (Graphics::font_id < 0) return 0.0f;                                                                             // return 0 if font is missing

            float text_w_ndc = 0.0f, text_h_ndc = 0.0f;                                                                         // storage for text size in screen space
            AEGfxGetPrintSize(Graphics::font_id, word.c_str(), EnemyConfig::TEXT_SCALE, &text_w_ndc, &text_h_ndc);              // measure text

            float half_window_width = (float)AEGfxGetWindowWidth() * 0.5f;                                                      // half window width in pixels
            return (text_w_ndc * 0.5f) * half_window_width;                                                                     // convert half width to world pixels
        }

        /*
        @function       Enemy_GetLaneX
        @params         lane_id (int), usable_min_x (float), usable_max_x (float)
        @brief          Computes the world X position for a given lane index.

        @usage          Enemy_SpawnFromPool

        This function maps a lane id into the usable horizontal range and returns the
        corresponding X coordinate. 
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        float Enemy_GetLaneX(int lane_id, float usable_min_x, float usable_max_x)
        {
            float t = (float)lane_id / (float)(EnemyConfig::ENEMY_LANE_COUNT - 1);                                              // lane position as 0..1
            return Utility::Utility_Lerp(usable_min_x, usable_max_x, t);                                                        // return lane x position
        }

        /*
        @function       Enemy_IsLaneSpawnYFree
        @params         lane_id (int), spawn_y (float), pool (const std::vector<Data::Enemy>&)
        @brief          Checks whether a lane has enough vertical space to spawn a new enemy at the spawn line.

        @usage          Enemy_SpawnFromPool

        This function scans all active enemies in the specified lane and finds the closest enemy below the
        spawn line (smallest positive gap). It returns true only if the nearest gap is at least
        ENEMY_LANE_MIN_GAP_PX, ensuring newly spawned enemies won't overlap or stack too tightly.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        bool Enemy_IsLaneSpawnYFree(int lane_id, float spawn_y, const std::vector<Data::Enemy>& pool)
        {
            float closest_gap = 999999.0f;                                                                                      // start with very large gap

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

        @usage          Enemy_SpawnFromPool

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
        @function       Enemy_Killed
        @params         e (Data::Enemy&)
        @brief          Marks an enemy as killed and switches it into the death-drop state.

        @usage          wWinMain

        This function safely transitions an active enemy into a dying state (only once). It clears the landed
        flag so the enemy can drop off-screen, and marks damage as already handled so a killed enemy will not
        apply landing damage.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Enemy_Killed(Data::Enemy& e)
        {
            if (!e.enemy_is_active) return;                                                                                     // ignore if already inactive
            if (e.enemy_is_dying) return;                                                                                       // ignore if already dying

            e.enemy_is_dying = true;                                                                                            // enter dying state
            e.enemy_has_landed = false;                                                                                         // clear landed flag for death drop

            e.enemy_dying_linger_timer_seconds = 0.0f;                                                                          // start linger timer from zero
            e.enemy_did_damage = true;                                                                                          // mark damage as already handled

            e.enemy_animation.animation_row_index = 1;                                                                          // use dying row
            e.enemy_animation.animation_frame_index = 0;                                                                        // start dying animation from frame 0
            e.enemy_animation.animation_timer_seconds = 0.0f;                                                                   // restart timing

            Animation::Animation_SetFrame(e.enemy_animation, 0);                                                                // start at dying frame 0 (and set UVs)
        }

        /*
        @function       Enemy_Update
        @params         e (Data::Enemy&), area (const Data::EnemyArea&), dt (float), health (Data::Health&)
        @brief          Updates an enemy’s animation, movement, and state transitions for the current frame.

        @usage          wWinMain

        This function handles both the normal falling behaviour and the dying "death drop" behaviour. It updates
        animation frames, moves the enemy downward, clamps it within the enemy area, applies landing damage once,
        and deactivates the enemy when its lifecycle is complete (landed or finished dying + linger).
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Enemy_Update(Data::Enemy& e, const Data::EnemyArea& area, float dt, Data::Health& health)
        {
            if (!e.enemy_is_active) return;                                                                                     // skip inactive enemies

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
                        Animation::Animation_SetFrame(e.enemy_animation,
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
            Animation::Animation_Update(e.enemy_animation, dt);                                                                 // advance normal animation

            if (!e.enemy_has_landed)                                                                                            // enemy is still falling
            {
                e.enemy_position_y -= e.enemy_fall_speed_units_per_second * dt;                                                 // move down based on fall speed

                e.enemy_position_x = Utility::Utility_Clamp(e.enemy_position_x,
                    area.area_min_x, area.area_max_x);                                                                          // clamp x within enemy area
                e.enemy_position_y = Utility::Utility_Clamp(e.enemy_position_y,
                    area.Area_BoxMinY(), area.Area_BoxMaxY());                                                                  // clamp y within enemy area

                if (e.enemy_position_y <= area.area_landing_y)                                                                  // reached landing line
                {
                    e.enemy_position_y = area.area_landing_y;                                                                   // snap to landing line
                    e.enemy_has_landed = true;                                                                                  // mark as landed

                    if (!e.enemy_did_damage)                                                                                    // damage not applied yet
                    {
                        e.enemy_did_damage = true;                                                                              // mark damage applied
                        e.enemy_is_active = false;                                                                              // remove enemy after landing
                        HealthSystem::Health_Add(health, -10.0f);                                                               // apply landing damage once
                    }
                    else                                                                                                        // damage already handled
                    {
                        e.enemy_is_active = false;                                                                              // remove enemy
                    }
                }
            }
            else                                                                                                                // already landed
            {
                e.enemy_position_y = area.area_landing_y;                                                                       // keep enemy on landing line
            }
        }

        /*
        @function       Enemy_ResolveLaneStacking
        @params         pool (std::vector<Data::Enemy>&), area (const Data::EnemyArea&)
        @brief          Enforces minimum vertical spacing between enemies within each lane.

        @usage          wWinMain

        This function groups active, non-dying enemies by lane, sorts them by Y position (lowest first),
        and then adjusts positions so each enemy is at least ENEMY_LANE_MIN_GAP_PX above the one below it.
        Adjusted positions are clamped to the top of the enemy area to keep enemies within bounds.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Enemy_ResolveLaneStacking(std::vector<Data::Enemy>& pool, const Data::EnemyArea& area)
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
                        e.enemy_position_y = min_allowed_y;                                                                     // push down to keep spacing

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

        @usage          wWinMain

        This function skips inactive enemies, sets the render state for textured sprites, selects the enemy
        texture based on the enemy's word length, applies the current animation UV offset, and draws the
        enemy quad scaled to ENEMY_SIZE_PX.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Enemy_Draw(const Data::Meshes& meshes, const Data::Enemy& e)
        {
            if (!e.enemy_is_active) return;                                                                                     // skip inactive enemies

            Graphics::Graphics_SetSpriteState();                                                                                // set textured draw state
            AEGfxTexture* tex = Graphics::Graphics_GetEnemyTextureByWordLength(e.enemy_word_length);                            // pick correct texture
            if (!tex) return;                                                                                                   // skip if texture missing

            AEGfxTextureSet(tex, e.enemy_animation.animation_uv_x, e.enemy_animation.animation_uv_y);                           // set texture and uv offset

            Graphics::Draw_DrawMeshAt(meshes.enemy_sprite_mesh,                                                                 // draw enemy quad mesh
                e.enemy_position_x,                                                                                             // x position
                e.enemy_position_y,                                                                                             // y position
                EnemyConfig::ENEMY_SIZE_PX, EnemyConfig::ENEMY_SIZE_PX,                                                         // scale to sprite size
                0.0f);                                                                                                          // no rotation
        }

        /*
        @function       Enemy_DrawWord
        @params         e (const Data::Enemy&)
        @brief          Draws the enemy's word centered on the enemy in screen space.

        @usage          wWinMain

        This function converts the enemy's world position to NDC coordinates, measures the rendered text size,
        then offsets the draw position so the word is centered over the enemy sprite. It skips drawing if the
        enemy is inactive, the font is missing, or the enemy has no assigned word.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Enemy_DrawWord(const Data::Enemy& e)
        {
            if (!e.enemy_is_active) return;                                                                                     // skip inactive enemies
            if (Graphics::font_id < 0) return;                                                                                  // skip if font missing
            if (e.enemy_word_text.empty()) return;                                                                              // skip if no text

            float ndc_x = 0.0f, ndc_y = 0.0f;                                                                                   // screen-space position output
            Utility::Convert_WorldToNdc(e.enemy_position_x, e.enemy_position_y, ndc_x, ndc_y);                                  // convert enemy position

            float text_w = 0.0f, text_h = 0.0f;                                                                                 // text size output
            AEGfxGetPrintSize(Graphics::font_id, e.enemy_word_text.c_str(), EnemyConfig::TEXT_SCALE, &text_w, &text_h);         // measure text size

            float draw_x = ndc_x - (text_w * 0.5f);                                                                             // center text horizontally
            float draw_y = ndc_y - (text_h * -2.5f);                                                                             // center text vertically

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // enable blending for text
            AEGfxPrint(Graphics::font_id, e.enemy_word_text.c_str(), draw_x, draw_y, EnemyConfig::TEXT_SCALE, 0, 0, 0, 1);      // draw the word
        }

        /*
        @function       Enemy_IsSpawnSpotFree
        @params         spawn_x (float), spawn_y (float), pool (const std::vector<Data::Enemy>&), half_size_with_padding (float)
        @brief          Checks whether a proposed spawn position would overlap any active enemy.

        @usage          Enemy_SpawnFromPool

        This function tests the candidate spawn AABB against all active enemies in the pool using
        Utility::Collision_Overlap. It returns true if no overlaps are found, and false if the new enemy
        would collide with an existing one (including padding).
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        bool Enemy_IsSpawnSpotFree(float spawn_x, float spawn_y, const std::vector<Data::Enemy>& pool, float half_size_with_padding)
        {
            for (const auto& other : pool)                                                                                      // loop through enemies
            {
                if (!other.enemy_is_active) continue;                                                                           // skip inactive enemies

                bool overlaps = Utility::Collision_Overlap(                                                                     // compute overlap result
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
        @function       Enemy_SpawnFromPool
        @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), word_length (int)
        @brief          Spawns a new enemy into an available pool slot using lane-based, non-overlapping placement.

        @usage          wWinMain

        This function pulls a free enemy slot, selects a unique word of the requested length, computes safe
        horizontal bounds based on sprite size and word width, then searches for a valid lane position at the
        spawn line. It first tries random lanes up to MAX_SPAWN_TRIES, then falls back to scanning all lanes.
        A spawn is accepted only if the lane has enough vertical spacing near the spawn line and the new enemy
        won’t overlap any active enemy. When a valid spot is found, the enemy is created with a difficulty-
        scaled fall speed and inserted into the pool.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Enemy_SpawnFromPool(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, int word_length)
        {
            if (free_slots.empty()) return;                                                                                     // skip if no free slots

            std::string unique_word = Enemy_PickUniqueWord(word_length, bank, pool);                                            // choose unique word
            if (unique_word.empty()) return;                                                                                    // skip if no unique word found

            float enemy_half = EnemyConfig::ENEMY_SIZE_PX * 0.5f;                                                               // enemy half size for spacing

            float word_half_w = Enemy_GetWordHalfWidthWorld(unique_word);                                                       // word half width in world pixels
            float safe_half_w = (word_half_w > enemy_half) ? word_half_w : enemy_half;                                          // use bigger of word or sprite

            float usable_min_x = area.area_min_x + safe_half_w;                                                                 // left usable x bound
            float usable_max_x = area.area_max_x - safe_half_w;                                                                 // right usable x bound
            if (usable_max_x <= usable_min_x) return;                                                                           // skip if there is no usable width

            float padding_half = EnemyConfig::ENEMY_OVERLAP_PADDING_PX * 0.5f;                                                  // half padding for overlap checks
            float overlap_half = enemy_half + padding_half;                                                                     // half size used for overlap tests

            float spawn_y = area.area_spawn_y;                                                                                  // spawn y position

            bool  found = false;                                                                                                // spawn found flag
            float chosen_x = 0.0f;                                                                                              // chosen spawn x
            int   chosen_lane = -1;                                                                                             // chosen lane id

            for (int i = 0; i < EnemyConfig::MAX_SPAWN_TRIES; ++i)                                                              // try random lanes for spawn
            { 
                int lane_id = Utility::Utility_RandInt(0, EnemyConfig::ENEMY_LANE_COUNT - 1);                                   // pick random lane

                if (!Enemy_IsLaneSpawnYFree(lane_id, spawn_y, pool))                                                            // reject if lane is too crowded near spawn
                    continue;                                                                                                   // continue to next try

                float lane_x = Enemy_GetLaneX(lane_id, usable_min_x, usable_max_x);                                             // compute lane x

                if (Enemy_IsSpawnSpotFree(lane_x, spawn_y, pool, overlap_half))                                                 // check overlap at lane position
                {
                    chosen_x = lane_x;                                                                                          // store chosen x
                    chosen_lane = lane_id;                                                                                      // store chosen lane
                    found = true;                                                                                               // mark found
                    break;                                                                                                      // stop searching
                }
            }

            if (!found)                                                                                                         // check fallback path
            {
                for (int lane_id = 0; lane_id < EnemyConfig::ENEMY_LANE_COUNT; ++lane_id)                                       // scan lanes in order
                {
                    if (!Enemy_IsLaneSpawnYFree(lane_id, spawn_y, pool))                                                        // reject if lane is too crowded
                        continue;                                                                                               // continue scanning

                    float lane_x = Enemy_GetLaneX(lane_id, usable_min_x, usable_max_x);                                         // compute lane x
                    if (Enemy_IsSpawnSpotFree(lane_x, spawn_y, pool, overlap_half))                                             // check overlap at lane position
                    {
                        chosen_x = lane_x;                                                                                      // store chosen x
                        chosen_lane = lane_id;                                                                                  // store chosen lane
                        found = true;                                                                                           // mark found
                        break;                                                                                                  // stop searching
                    }
                }
            }

            if (!found || chosen_lane < 0) return;                                                                              // skip if no valid spawn lane found

            float fall_speed = DifficultySystem::Difficulty_GetEnemyFallSpeed(word_length);                                     // compute fall speed

            int slot = free_slots.front();                                                                                      // take the oldest free slot
            free_slots.pop_front();                                                                                             // remove that slot from queue

            pool[(size_t)slot] = Enemy_Create(area, word_length, unique_word, fall_speed, chosen_lane);                         // place new enemy in slot
            pool[(size_t)slot].enemy_position_x = Utility::Utility_Clamp(chosen_x, usable_min_x, usable_max_x);                 // clamp spawn x
            pool[(size_t)slot].enemy_position_y = Utility::Utility_Clamp(area.area_spawn_y,
                                                                         area.Area_BoxMinY(), area.Area_BoxMaxY());             // clamp spawn y
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

        @usage          wWinMain

        This function computes the bounding box of the enemy area from 'area', then renders a tinted,
        semi-transparent rectangle using the debug mesh. It helps visualize where enemies are allowed
        to spawn, fall, and land.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Debug_DrawEnemyAreaBox(const Data::Meshes& meshes, const Data::EnemyArea& area)
        {
            float box_min_y = area.Area_BoxMinY();                                                                              // read bottom y
            float box_max_y = area.Area_BoxMaxY();                                                                              // read top y

            float center_x = (area.area_min_x + area.area_max_x) * 0.5f;                                                        // compute box center x
            float center_y = (box_min_y + box_max_y) * 0.5f;                                                                    // compute box center y

            float w = (area.area_max_x - area.area_min_x);                                                                      // compute box width
            float h = (box_max_y - box_min_y);                                                                                  // compute box height

            if (w <= 0.0f || h <= 0.0f) return;                                                                                 // skip invalid box

            Graphics::Graphics_SetColorState(0.50f);                                                                            // set semi-transparent draw state
            AEGfxSetColorToMultiply(0.2f, 0.9f, 1.0f, 1.0f);                                                                    // tint debug box color

            Graphics::Draw_DrawMeshAt(meshes.debug_box_mesh, center_x, center_y, w, h, 0.0f);                                   // draw debug box

            AEGfxSetColorToMultiply(1, 1, 1, 1);                                                                                // reset tint color
            AEGfxSetTransparency(1.0f);                                                                                         // reset alpha
        }

        /*
        @function       Debug_DrawEnemyLaneLines
        @params         meshes (const Data::Meshes&), area (const Data::EnemyArea&)
        @brief          Draws vertical debug lines to visualize each enemy lane inside the enemy area.

        @usage          wWinMain

        This function renders one vertical line per lane across the enemy area height, using distinct colors
        to make lanes easy to tell apart. It helps debug lane placement, spawn positions, and spacing rules.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Debug_DrawEnemyLaneLines(const Data::Meshes& meshes, const Data::EnemyArea& area)
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

            Graphics::Graphics_SetColorState(0.60f);                                                                            // set semi-transparent draw state

            for (int lane = 0; lane < EnemyConfig::ENEMY_LANE_COUNT; ++lane)                                                    // loop over lanes
            {
                float t = (EnemyConfig::ENEMY_LANE_COUNT <= 1) ? 0.5f : 
                            (float)lane / (float)(EnemyConfig::ENEMY_LANE_COUNT - 1);                                           // lane position as 0..1
                float lane_x = Utility::Utility_Lerp(min_x, max_x, t);                                                          // compute lane x

                AEGfxSetColorToMultiply(lane_colors[lane][0], lane_colors[lane][1], lane_colors[lane][2], 1.0f);                // set line tint

                Graphics::Draw_DrawMeshAt(meshes.debug_box_mesh, lane_x, center_y, thickness, h, 0.0f);                         // draw lane line
            }

            AEGfxSetColorToMultiply(1, 1, 1, 1);                                                                                // reset tint color
            AEGfxSetTransparency(1.0f);                                                                                         // reset alpha
        }

        /*
        @function       Debug_DrawDifficultyTopRight
        @params         show (bool), auto_spawn_on (bool), health (const Data::Health&)
        @brief          Draws a debug HUD line showing health, difficulty, spawn pacing, and word-length unlocks.

        @usage          wWinMain

        This function prints a single line of diagnostic text near the top-right area of the screen, including
        current/max HP, the current difficulty value, the current spawn interval, whether auto-spawn is enabled,
        and the unlocked word-length range.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Debug_DrawDifficultyTopRight(bool show, bool auto_spawn_on, const Data::Health& health)
        {
            if (!show) return;                                                                                                  // skip if hidden
            if (Graphics::font_id < 0) return;                                                                                  // skip if font missing

            char buffer[220];                                                                                                   // text buffer
            float hp_now = health.health_current;                                                                               // read current health
            float hp_max = health.health_max;                                                                                   // read max health
            float diff = DifficultySystem::Difficulty_Get();                                                                    // read difficulty value
            float interval = DifficultySystem::Difficulty_GetSpawnIntervalSeconds();                                            // read spawn interval
            const char* auto_text = auto_spawn_on ? "ON" : "OFF";                                                               // text for auto spawn state
            int max_len = DifficultySystem::Difficulty_GetUnlockedMaxWordLength();                                              // read unlocked max word length

            std::snprintf(buffer, sizeof(buffer),                                                                               // format debug line
                "HP: %.0f/%.0f  DIFF: %.1f  SPAWN: %.2fs  AUTO: %s  WORDS: 3-%d",                                               // format string
                hp_now, hp_max, diff, interval, auto_text, max_len);                                                            // values for format

            AEGfxSetBlendMode(AE_GFX_BM_BLEND);                                                                                 // enable blending for text
            AEGfxPrint(Graphics::font_id, buffer, -0.13f, 0.90f, 0.5f, 1.0f, 0.9f, 0.2f, 1.0f);                                 // draw top-right-ish text
        }

        void Ui_DrawHealthBar(Data::Health& health, Data::Meshes& meshes) { //(jeremy)
            float bar_w = 250.0f;  // Total width of the bar
            float bar_h = 25.0f;   // Total height of the bar
            float bar_x = -550.0f; // Center X position of the bar
            float bar_y = 350.0f;  // Center Y position of the bar

            Graphics::Graphics_SetColorState(1.0f);
            AEGfxSetColorToMultiply(0.8f, 0.2f, 0.2f, 1.0f); // Dark Red
            Graphics::Draw_DrawMeshAt(meshes.debug_box_mesh, bar_x, bar_y, bar_w, bar_h, 0.0f);

            float hp_percent = health.health_current / health.health_max;
            hp_percent = Utility::Utility_Clamp(hp_percent, 0.0f, 1.0f);

            float current_w = bar_w * hp_percent;

            float left_edge = bar_x - (bar_w * 0.5f);
            float new_center_x = left_edge + (current_w * 0.5f);

            AEGfxSetColorToMultiply(0.2f, 1.0f, 0.2f, 1.0f); // Bright Green
            Graphics::Draw_DrawMeshAt(meshes.debug_box_mesh, new_center_x, bar_y, current_w, bar_h, 0.0f);

            AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
        }
    } // UiSystem namespace end
    /*___________________________________________________________________________________________________________________________________________________________________________________________________*/

    /*
    @namespace      DebugSystem
    @brief          Provides debug-only input handling for toggles, cheats, and test spawns.

    This namespace contains helper code for developer/testing controls, such as toggling debug visuals,
    adjusting difficulty/health, enabling or disabling auto-spawning, and manually spawning enemies of
    specific word lengths.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    namespace DebugSystem
    {
        /*
        @function       Debug_HandleInputs
        @params         is_enemy_area_box_visible (bool&), is_random_spawning_enabled (bool&), is_difficulty_debug_visible (bool&), health (Data::Health&),
                        enemy_pool (std::vector<Data::Enemy>&), free_slot_queue (std::deque<int>&), enemy_area (const Data::EnemyArea&), word_bank (const Data::WordBank&)
        @brief          Processes debug key inputs and applies the corresponding debug actions.

        @usage          wWinMain

        This function listens for CTRL + key combinations to toggle debug overlays, modify health and
        difficulty values, toggle auto-spawning, and manually spawn enemies with specific word lengths.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void Debug_HandleInputs(bool& is_enemy_area_box_visible, bool& is_random_spawning_enabled, bool& is_difficulty_debug_visible, Data::Health& health,
                                std::vector<Data::Enemy>& enemy_pool, std::deque<int>& free_slot_queue, const Data::EnemyArea& enemy_area, const Data::WordBank& word_bank) 
        {
            // Debug keys
            bool is_ctrl_down = Utility::Input_CtrlDown();                                                                      // read ctrl key state

            if (is_ctrl_down && AEInputCheckTriggered(AEVK_1))
                is_enemy_area_box_visible = !is_enemy_area_box_visible;                                                         // toggle debug box

            if (is_ctrl_down && AEInputCheckTriggered(AEVK_2))
                HealthSystem::Health_Add(health, 10.0f);                                                                        // add health

            if (is_ctrl_down && AEInputCheckTriggered(AEVK_9))
                is_random_spawning_enabled = !is_random_spawning_enabled;                                                       // toggle auto spawning

            if (is_ctrl_down && AEInputCheckTriggered(AEVK_0))
                is_difficulty_debug_visible = !is_difficulty_debug_visible;                                                     // toggle difficulty display

            if (is_ctrl_down && AEInputCheckTriggered(AEVK_MINUS))
                DifficultySystem::Difficulty_Add(-1.0f);                                                                        // decrease difficulty

            if (is_ctrl_down && AEInputCheckTriggered(AEVK_EQUAL))
                DifficultySystem::Difficulty_Add(1.0f);                                                                         // increase difficulty

            // Manual spawns
            if (is_ctrl_down && AEInputCheckTriggered(AEVK_3))
                EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 3);                        // spawn 3-letter enemy
            if (is_ctrl_down && AEInputCheckTriggered(AEVK_4))
                EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 4);                        // spawn 4-letter enemy
            if (is_ctrl_down && AEInputCheckTriggered(AEVK_5))
                EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 5);                        // spawn 5-letter enemy
            if (is_ctrl_down && AEInputCheckTriggered(AEVK_6))
                EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 6);                        // spawn 6-letter enemy
            if (is_ctrl_down && AEInputCheckTriggered(AEVK_7))
                EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 7);                        // spawn 7-letter enemy
            if (is_ctrl_down && AEInputCheckTriggered(AEVK_8))
                EnemySystem::Enemy_SpawnFromPool(enemy_pool, free_slot_queue, enemy_area, word_bank, 8);                        // spawn 8-letter enemy
        }
    } // DebugSystem namespace end
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

        @usage          wWinMain

        This function boots the AE engine and creates the game window, sets the window title, seeds the random
        number generator, loads all word lists into the provided WordBank, and loads the font used for rendering
        enemy words and debug text.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
       void EnemyManage_InitEngine(Data::WordBank& bank)
        {
            srand((unsigned)time(NULL));                                                                                        // seed random generator

            Word::Word_LoadAll(bank);                                                                                           // load word lists

            Graphics::font_id = AEGfxCreateFont("Assets/Pixel-Game.otf", 80);                                                   // load font
            AE_ASSERT_MESG(Graphics::font_id >= 0, "Failed to load font: Assets/Pixel-Game.otf");                               // stop if font fails
        }


        /*
        @function       EnemyManage_LoadMeshes
        @params         enemy_anim (const Data::SpriteAnim&)
        @brief          Loads enemy textures and builds the meshes needed for rendering.

        @usage          wWinMain

        This function loads the enemy sprite textures for word lengths 3 through 8, builds the textured quad
        mesh used to draw animated enemy sprites (using the sprite sheet UV frame size from 'enemy_anim'),
        and builds a unit rectangle mesh used for debug drawing. It returns a Data::Meshes struct containing
        the created mesh handles.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        Data::Meshes EnemyManage_LoadMeshes(const Data::SpriteAnim& enemy_anim)
        {
            Data::Meshes meshes{};                                                                                              // create mesh container

            Graphics::enemy_textures[3] = AEGfxTextureLoad("Assets/3letterEnemy.png");                                          // load 3-letter enemy texture
            AE_ASSERT_MESG(Graphics::enemy_textures[3], "Failed to load: Assets/3letterEnemy.png");                             // stop if missing

            Graphics::enemy_textures[4] = AEGfxTextureLoad("Assets/4letterEnemy.png");                                          // load 4-letter enemy texture
            AE_ASSERT_MESG(Graphics::enemy_textures[4], "Failed to load: Assets/4letterEnemy.png");                             // stop if missing

            Graphics::enemy_textures[5] = AEGfxTextureLoad("Assets/5letterEnemy.png");                                          // load 5-letter enemy texture
            AE_ASSERT_MESG(Graphics::enemy_textures[5], "Failed to load: Assets/5letterEnemy.png");                             // stop if missing

            Graphics::enemy_textures[6] = AEGfxTextureLoad("Assets/6letterEnemy.png");                                          // load 6-letter enemy texture
            AE_ASSERT_MESG(Graphics::enemy_textures[6], "Failed to load: Assets/6letterEnemy.png");                             // stop if missing

            Graphics::enemy_textures[7] = AEGfxTextureLoad("Assets/7letterEnemy.png");                                          // load 7-letter enemy texture
            AE_ASSERT_MESG(Graphics::enemy_textures[7], "Failed to load: Assets/7letterEnemy.png");                             // stop if missing

            Graphics::enemy_textures[8] = AEGfxTextureLoad("Assets/8letterEnemy.png");                                          // load 8-letter enemy texture
            AE_ASSERT_MESG(Graphics::enemy_textures[8], "Failed to load: Assets/8letterEnemy.png");                             // stop if missing

            meshes.enemy_sprite_mesh = Graphics::Mesh_BuildSpriteQuad(enemy_anim.Anim_UvWidth(),
                                                                      enemy_anim.Anim_UvHeight(), 0xFFFFFFFF);                  // build enemy sprite mesh

            meshes.debug_box_mesh = Graphics::Mesh_BuildUnitRect(0xFFFFFFFF);                                                   // build debug rectangle mesh

            return meshes;                                                                                                      // return loaded meshes
        }

        /*
        @function       EnemyManage_FreeMeshes
        @params         meshes (Data::Meshes&)
        @brief          Releases all loaded meshes, textures, and font resources used by the game.

        @usage          wWinMain

        This function frees the mesh objects stored in 'meshes', unloads all enemy textures (3–8) from the
        Graphics texture table, and destroys the loaded font. It also resets pointers/handles back to safe
        defaults to prevent dangling references.
        *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
        void EnemyManage_FreeMeshes(Data::Meshes& meshes)
        {
            if (meshes.enemy_sprite_mesh)  AEGfxMeshFree(meshes.enemy_sprite_mesh);                                             // free enemy mesh
            if (meshes.debug_box_mesh)     AEGfxMeshFree(meshes.debug_box_mesh);                                                // free debug mesh

            meshes = {};                                                                                                        // reset mesh pointers to defaults

            for (int i = 3; i <= 8; ++i)                                                                                        // loop through enemy texture slots
            {
                if (Graphics::enemy_textures[i])                                                                                // check if texture exists
                {
                    AEGfxTextureUnload(Graphics::enemy_textures[i]);                                                            // unload texture
                    Graphics::enemy_textures[i] = nullptr;                                                                      // clear pointer
                }
            }

            if (Graphics::font_id >= 0)                                                                                         // check if font exists
            {
                AEGfxDestroyFont(Graphics::font_id);                                                                            // destroy font
                Graphics::font_id = -1;                                                                                         // reset font id
            }
        }
    } // EnemyManage namespace end
    /*___________________________________________________________________________________________________________________________________________________________________________________________________*/