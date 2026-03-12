/*!
@file           Enemy.h
@author         Wong Zi Yuan (w.ziyuan@digipen.edu)
@course         CSD1451
@section        A
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
#pragma once

#include <cstdlib>
#include <vector>
#include <deque>
#include <string>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include "AEEngine.h"

/*
@namespace      EnemyConfig
@brief          Centralized configuration values that control enemy behavior, pacing, and spacing.

This namespace defines the key tuning values for the enemy system, including pool limits, spawn attempts,
sprite and text sizing, difficulty ramp behavior, spawn interval scaling, fall speeds (normal and death),
overlap padding, and the lane layout and spacing rules. Adjusting these values changes gameplay feel without
modifying the core enemy logic.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace EnemyConfig
{
    // Enemy pool sizing and safety limits
    extern const int   MAX_ENEMIES;                                                                                         // max enemies stored in the pool
    extern const int   MAX_SPAWN_TRIES;                                                                                     // max attempts to find a safe spawn spot
    extern const int   MAX_WORD_TRIES;                                                                                      // max attempts to find a unique word

    // Visual and text sizing
    extern const float ENEMY_SIZE_PX;                                                                                       // enemy sprite size
    extern const float TEXT_SCALE;                                                                                          // enemy word text scale

    // Difficulty tuning
    extern const float DIFFICULTY_RAMP_PER_SECOND;                                                                          // difficulty gain per second
    extern const float DIFFICULTY_MAX;                                                                                      // cap for difficulty value

    // Random spawn tuning
    extern const float RANDOM_SPAWN_INTERVAL_START_SECONDS;                                                                 // starting spawn interval
    extern const float RANDOM_SPAWN_INTERVAL_MIN_SECONDS;                                                                   // minimum spawn interval
    extern const float RANDOM_SPAWN_INTERVAL_DECREASE_PER_LEVEL;                                                            // interval decrease per difficulty level

    // Fall speed tuning
    extern const float ENEMY_FALL_SPEED_START;                                                                              // starting fall speed
    extern const float ENEMY_FALL_SPEED_INCREASE_PER_LEVEL;                                                                 // fall speed increase per difficulty level

    // Death drop tuning
    extern const float ENEMY_DEATH_FALL_SPEED;                                                                              // fast drop speed when killed
    extern const float ENEMY_DEATH_LINGER_SECONDS;                                                                          // time to keep a killed enemy visible before removing it

    // Spacing
    extern const float ENEMY_OVERLAP_PADDING_PX;                                                                            // extra padding between enemies

    // Lane system
    extern const int   ENEMY_LANE_COUNT;                                                                                    // number of horizontal lanes
    extern const float ENEMY_LANE_MIN_GAP_PX;                                                                               // min vertical spacing per lane
}
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/

/*
@namespace      Data
@brief          Defines core gameplay data structures used by multiple systems.

This namespace groups simple structs that store game state (health, score), rendering state
(sprite animation, meshes), and enemy related configuration and state so all systems can share
them cleanly between update, spawn, and draw code.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace Data
{
    /*
    @structure      Health
    @brief          Stores the player health values.

    This struct keeps track of the maximum health and the current health during gameplay.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    struct Health
    {
        float health_max = 100.0f;                                                                                          // maximum health value
        float health_current = 100.0f;                                                                                      // current health value
    };

    /*
    @structure      Score
    @brief          Stores the current score value.

    This struct holds the player score as an integer counter.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    struct Score
    {
        int score_value = 0;                                                                                                // current score value
    };

    /*
    @structure      SpriteAnim
    @brief          Holds sprite sheet animation state.

    This struct stores the parameters and runtime state needed to animate a sprite sheet, including the
    sheet layout (rows/columns), which row to use, a frame timer, the current frame index, and the UV
    offset for the current frame.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    struct SpriteAnim
    {
        int animation_columns = 6;                                                                                          // number of columns in the sheet
        int animation_rows = 2;                                                                                             // number of rows in the sheet
        int animation_row_index = 0;                                                                                        // which row of the sprite sheet to animate

        float animation_timer_seconds = 0.0f;                                                                               // time since last frame change
        float animation_frame_duration_seconds = 0.10f;                                                                     // time per animation frame

        int animation_frame_index = 0;                                                                                      // current frame index

        float animation_uv_x = 0.0f;                                                                                        // current frame uv x start
        float animation_uv_y = 0.0f;                                                                                        // current frame uv y start

        float Anim_UvWidth()  const { return 1.0f / (float)animation_columns; }                                             // width of one frame in uv space
        float Anim_UvHeight() const { return 1.0f / (float)animation_rows; }                                                // height of one frame in uv space
        int   Anim_FrameCount() const { return animation_columns; }                                                         // number of frames in the current row

    };

    /*
    @structure      EnemyArea
    @brief          Defines the playable area used for enemy spawning and landing.

    This struct stores the horizontal bounds for enemies, plus the spawn and landing heights.
    Helper functions provide the min/max Y values for treating the area as a box.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    struct EnemyArea
    {
        float area_min_x = -600.0f;                                                                                         // left bound of enemy area
        float area_max_x = 740.0f;                                                                                          // right bound of enemy area

        float area_spawn_y = -200.0f;                                                                                       // spawn height
        float area_landing_y = -250.0f;                                                                                     // landing height

        float Area_BoxMinY() const { return area_landing_y; }                                                               // bottom of the area box
        float Area_BoxMaxY() const { return area_spawn_y; }                                                                 // top of the area box
    };

    /*
    @enum           EnemyType
    @brief          Identifies the behavior type of an enemy (normal, friendly, or shielded).

    Each value tells the update and draw code how to handle that enemy. Normal enemies fall and
    damage the player on landing. Friendly enemies heal the player when typed and fly away instead
    of dying. Shielded enemies require two words to defeat, with the second word hidden until the
    shield is broken.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    enum class EnemyType
    {
        NORMAL,                                                                                                             // standard enemy
        FRIENDLY,                                                                                                           // friendly npc
        SHIELDED,                                                                                                           // enemy with shield
    };

    /*
    @structure      Enemy
    @brief          Stores runtime state for a single enemy instance.

    This struct contains all per enemy data needed for gameplay and rendering, including position, fall
    speed, lane assignment, word (text and length), animation state, and a set of flags and timers that
    track lifecycle states such as active or inactive, landed, damage application, and dying (death drop
    plus linger time).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    struct Enemy
    {
        float enemy_position_x = 0.0f;                                                                                      // enemy x position
        float enemy_position_y = 0.0f;                                                                                      // enemy y position

        float enemy_fall_speed_units_per_second = EnemyConfig::ENEMY_FALL_SPEED_START;                                      // fall speed for normal drop

        bool enemy_is_active = true;                                                                                        // visibility and update flag
        bool enemy_has_landed = false;                                                                                      // landing state flag
        bool enemy_did_damage = false;                                                                                      // damage applied flag

        bool enemy_is_dying = false;                                                                                        // death drop state flag
        float enemy_dying_linger_timer_seconds = 0.0f;                                                                      // time spent sitting on landing line

        EnemyType enemy_type = EnemyType::NORMAL;                                                                           // what kind of enemy this is

        bool enemy_shield_intact = false;                                                                                   // shield stage flag (only for shield enemy)
        std::string enemy_word_text_alt;                                                                                    // hidden/second word (only for shield enemy)

        int enemy_death_row_index = 1;                                                                                      // death animation row

        int enemy_word_length = 3;                                                                                          // word length for this enemy
        std::string enemy_word_text;                                                                                        // word text shown on enemy

        int enemy_lane_id = -1;                                                                                             // lane index for this enemy

        SpriteAnim enemy_animation{};                                                                                       // animation state for this enemy
    };

    /*
    @structure      Meshes
    @brief          Holds mesh handles used for rendering.

    This struct stores pointers to the meshes needed to draw enemies and optional debug shapes.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    struct Meshes
    {
        AEGfxVertexList* enemy_sprite_mesh = nullptr;                                                                       // 2-row mesh (normal enemies)
        AEGfxVertexList* shield_sprite_mesh = nullptr;                                                                      // 3-row mesh (shield enemy)
        AEGfxVertexList* friendly_sprite_mesh = nullptr;                                                                    // 3-row mesh (friendly)
        AEGfxVertexList* debug_box_mesh = nullptr;                                                                          // box mesh for debug shapes
    };

    /*
    @structure      WordBank
    @brief          Stores word lists grouped by word length.

    This struct holds separate arrays of words for each supported length (3 to 8 letters).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    struct WordBank
    {
        std::vector<std::string> words_3;                                                                                   // list of 3-letter words
        std::vector<std::string> words_4;                                                                                   // list of 4-letter words
        std::vector<std::string> words_5;                                                                                   // list of 5-letter words
        std::vector<std::string> words_6;                                                                                   // list of 6-letter words
        std::vector<std::string> words_7;                                                                                   // list of 7-letter words
        std::vector<std::string> words_8;                                                                                   // list of 8-letter words
    };
} // Data namespace end
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
    float Utility_Clamp(float value, float minimum, float maximum);

    /*
    @function       Utility_Lerp
    @params         a (float), to (float), t (float)
    @brief          Linearly interpolates between two float values.

    This function blends from 'a' to 'b' using 't' as the interpolation factor.
    't' is clamped to [0, 1], where 0 returns 'a', 1 returns 'b', and values in-between return a mix.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Utility_Lerp(float a, float b, float t);

    /*
    @function       Utility_RandInt
    @params         minimum (float), maximum_inclusive (float)
    @brief          Returns a random integer within an inclusive range.

    This function generates a uniformly distributed integer in [minimum, maximum_inclusive].
    If the range is invalid (maximum_inclusive < minimum), it safely returns 'minimum'.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Utility_RandInt(int minimum, int maximum_inclusive);

    /*
    @function       Convert_WorldToNdc
    @params         world_x (float), world_y (float), ndc_x (float&), ndc_y (float&)
    @brief          Converts world-space pixel coordinates into normalized device coordinates (NDC).

    This function maps a position given in world pixels to NDC space, where the window center is (0, 0)
    and the visible range is typically -1..1 on each axis. The result is written to 'ndc_x' and 'ndc_y'.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Convert_WorldToNdc(float world_x, float world_y, float& ndc_x, float& ndc_y);

    /*
    @function       Collision_Overlap
    @params         a_center_x (float), a_center_y (float), a_half_width (float), a_half_height (float)
                    b_center_x (float), b_center_y (float), b_half_width (float), b_half_height (float)
    @brief          Tests whether two axis-aligned bounding boxes (AABBs) overlap.

    This function performs an AABB overlap test using the centers and half-extents of two rectangles.
    It returns true if the rectangles intersect (including touching edges), otherwise false.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Collision_Overlap(float a_center_x, float a_center_y, float a_half_width, float a_half_height, float b_center_x, float b_center_y, float b_half_width, float b_half_height);

    /*
    @function       String_ToUpperInPlace
    @params         s (std::string&)
    @brief          Converts all characters in a string to uppercase.

    This function modifies the input string directly by uppercasing each character using 'std::toupper'.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void String_ToUpperInPlace(std::string& s);

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
    @brief          Loads words from a text file and filters by a specific length.

    This function reads whitespace-separated words from the file at 'file_path', keeps only words whose
    length matches 'expected_length', converts them to uppercase, and returns them as a vector.
    If the file cannot be opened, it returns an empty list.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::vector<std::string> Word_LoadFromFile(const char* file_path, int expected_length);

    /*
    @function       Word_LoadAll
    @params         word_bank (Data::WordBank&)
    @brief          Loads all word lists (3 to 8 letters) into the WordBank.

    This function populates the WordBank by loading words from the corresponding asset files for each
    supported length (3 to 8). If any file fails to load or results in an empty list, a small fallback
    set is assigned so the game can still run.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Word_LoadAll(Data::WordBank& word_bank);

    /*
    @function       Word_GetListByLength
    @params         word_bank (const Data::WordBank&), word_length (int)
    @brief          Returns the word list that matches the requested word length.

    This function selects and returns a const reference to the appropriate word vector inside 'word_bank'
    based on 'word_length' (3 to 8). If the length is not 3 to 7, it defaults to returning the 8-letter list.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    const std::vector<std::string>& Word_GetListByLength(const Data::WordBank& word_bank, int word_length);

    /*
    @function       Word_PickRandom
    @params         words (const std::vector<std::string>&)
    @brief          Picks and returns a random word from a list.

    This function returns a randomly selected string from 'words'.
    If the list is empty, it returns a placeholder string ("????") to avoid crashes.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::string Word_PickRandom(const std::vector<std::string>& words);

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
    extern s8 font_id;
    /*
    @function       Graphics_GetEnemyTextureByWordLength
    @params         word_length (int)
    @brief          Returns the enemy sprite texture that matches a given word length.

    This function clamps 'word_length' to the supported range (3 to 8) and returns the corresponding
    texture from the enemy texture table. It is used to select the correct enemy sprite based on
    the length of the word assigned to that enemy.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    AEGfxTexture* Graphics_GetEnemyTextureByWordLength(int word_length);

    /*
    @function       Graphics_SetColorState
    @params         alpha_value (float)
    @brief          Configures the graphics pipeline for solid-color rendering with transparency.

    This function switches the renderer to color mode, enables alpha blending, and applies the requested
    transparency value. It also resets the multiply/add color modifiers to neutral so debug shapes draw
    with predictable colors.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Graphics_SetColorState(float alpha_value);

    /*
    @function       Graphics_SetSpriteState
    @brief          Configures the graphics pipeline for drawing textured sprites.

    This function switches the renderer to texture mode, enables alpha blending, resets transparency
    to fully opaque, and clears any color multiply/add modifiers so sprites render normally.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Graphics_SetSpriteState();

    /*
    @function       Mesh_BuildSpriteQuad
    @params         uv_width (float), uv_height (float), color_value (u32)
    @brief          Builds and returns a unit quad mesh with the given UV size and vertex color.

    This function creates a centered 1x1 quad (from -0.5 to 0.5 in both axes) made of two triangles.
    The UVs span [0..uv_width] and [0..uv_height], allowing sprite sheets or partial textures.
    The mesh is returned as an AEGfxVertexList* ready to be used for rendering.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    AEGfxVertexList* Mesh_BuildSpriteQuad(float uv_width, float uv_height, u32 color_value);

    /*
    @function       Mesh_BuildUnitRect
    @params         color_value (u32)
    @brief          Builds and returns a centered unit rectangle mesh for solid-color rendering.

    This function creates a 1x1 rectangle (from -0.5 to 0.5 in both axes) made of two triangles with a
    constant vertex color and dummy UVs. It is intended for drawing colored shapes such as UI bars and
    debug rectangles by scaling it to the desired size at render time.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    AEGfxVertexList* Mesh_BuildUnitRect(u32 color_value);

    /*
    @function       Draw_DrawMeshAt
    @params         mesh (AEGfxVertexList*), position_x (float), position_y (float), scale_x (float), scale_y (float), rotation_radians (float)
    @brief          Draws a mesh at a given position with scale and rotation.

    This function builds a 2D transform (scale then rotate then translate), applies it to the renderer,
    and draws the provided mesh using triangle mode.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Draw_DrawMeshAt(AEGfxVertexList* mesh, float position_x, float position_y, float scale_x, float scale_y, float rotation_radians);

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
    /*
    @function       Difficulty_Get
    @brief          Returns the current difficulty value.

    This function provides read-only access to the internal difficulty variable so other systems
    (such as debug UI) can display or use the current difficulty level.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Difficulty_Get();

    /*
    @function       Difficulty_Reset
    @brief          Resets the current difficulty value back to zero.

    This function sets the internal difficulty value to 0.0, returning the game to its starting
    difficulty level. It is called when restarting a level or entering the game from the main menu.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Difficulty_Reset();

    /*
    @function       Difficulty_Add
    @params         amount (float)
    @brief          Adjusts the current difficulty value and clamps it to a valid range.

    This function adds a signed amount to the internal difficulty value, then clamps it between 0 and
    DIFFICULTY_MAX to prevent the difficulty from going negative or exceeding the configured cap.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Difficulty_Add(float amount);

    /*
    @function       Difficulty_UpdateOverTime
    @params         dt (float)
    @brief          Increases difficulty gradually over time.

    This function ramps the difficulty based on elapsed time by adding a fixed rate
    (DIFFICULTY_RAMP_PER_SECOND) scaled by delta time.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Difficulty_UpdateOverTime(float dt);

    /*
    @function       Difficulty_GetSpawnIntervalSeconds
    @brief          Calculates the current enemy spawn interval based on difficulty.

    This function reduces the spawn interval as difficulty increases, making enemies spawn more often.
    The interval is clamped so it never goes below RANDOM_SPAWN_INTERVAL_MIN_SECONDS.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Difficulty_GetSpawnIntervalSeconds();

    /*
    @function       Difficulty_GetUnlockedMaxWordLength
    @brief          Returns the maximum word length currently unlocked by the difficulty value.

    This function maps the current difficulty into tiers that gradually unlock longer words.
    As difficulty increases, the allowed maximum word length increases from 3 up to 8.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Difficulty_GetUnlockedMaxWordLength();

    /*
    @function       Difficulty_GetRandomWordLength
    @brief          Returns a random word length based on the current difficulty.

    This function chooses a random word length between 3 and the currently unlocked maximum word length
    (determined by difficulty). It is used to vary enemy word lengths while still respecting the
    difficulty progression.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    int Difficulty_GetRandomWordLength();

    /*
    @function       Difficulty_GetEnemyFallSpeed
    @params         word_length (int)
    @brief          Computes an enemy's fall speed based on current difficulty and word length.

    This function starts from a base fall speed, increases it with difficulty, then applies a word length
    multiplier (shorter words fall faster, longer words fall slower). The multiplier effect ramps in as
    difficulty increases, so early gameplay stays gentler while later gameplay becomes more punishing.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Difficulty_GetEnemyFallSpeed(int word_length);

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
    void Health_Init(Data::Health& health);

    /*
    @function       Health_Add
    @params         h (Data::Health&), amount (float)
    @brief          Adds (or subtracts) health and clamps the result to a valid range.

    This function applies a signed health change to 'h.health_current' and then clamps it between
    0 and 'h.health_max' to prevent underflow or overflow.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Health_Add(Data::Health& h, float amount);

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
    void Animation_ApplyUv(Data::SpriteAnim& anim);

    /*
    @function       Animation_SetFrame
    @params         anim (Data::SpriteAnim&), frame_index (int)
    @brief          Forces the animation to a specific frame and updates the UV coordinates.

    This function directly sets the current animation frame index (typically to show a specific pose, like
    a death frame), then recalculates the sprite sheet UV offsets based on the current column and the
    selected animation row.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Animation_SetFrame(Data::SpriteAnim& anim, int frame_index);

    /*
    @function       Animation_Update
    @params         anim (Data::SpriteAnim&), dt (float)
    @brief          Advances a sprite animation and updates its UV coordinates.

    This function accumulates delta time, advances the animation frame when the frame duration is reached,
    wraps the frame index back to 0 when it exceeds the number of frames, and then updates the sprite sheet
    UV offsets using the current frame column and the selected animation row.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Animation_Update(Data::SpriteAnim& anim, float dt);

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
    bool Enemy_IsWordInUse(const std::vector<Data::Enemy>& pool, const std::string& word_text, bool include_hidden_words);

    /*
    @function       Enemy_PickUniqueWord
    @params         word_length (int), bank (const Data::WordBank&), pool (const std::vector<Data::Enemy>&), include_hidden_words (bool)
    @brief          Picks a random word of the given length that is not already used by any active enemy.

    This function randomly picks words from the word bank and keeps trying until it finds one that
    is not already in use. Returns an empty string if no unique word is found within the attempt limit.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::string Enemy_PickUniqueWord(int word_length, const Data::WordBank& bank, const std::vector<Data::Enemy>& pool, bool include_hidden_words);

    /*
    @function       Enemy_PickUniqueWordRandomLength
    @params         min_len (int), max_len (int), bank (const Data::WordBank&), pool (const std::vector<Data::Enemy>&), include_hidden_words (bool), max_tries_total (int)
    @brief          Picks a unique word with a random length chosen between min_len and max_len.

    This function picks a random word length each attempt and tries to find a unique word of that
    length. It keeps trying up to max_tries_total times. Returns an empty string if nothing is found.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    std::string Enemy_PickUniqueWordRandomLength(int min_len, int max_len, const Data::WordBank& bank, const std::vector<Data::Enemy>& pool, bool include_hidden_words, int max_tries_total);

    /*
    @function       Enemy_GetWordHalfWidthWorld
    @params         word (const std::string&)
    @brief          Calculates the half-width of a word in world pixels for spacing checks.

    This function measures the rendered width of 'word' using the current font and TEXT_SCALE, then converts
    that screen-space size into world pixels. It is used to keep spawned enemies (and their text) within
    usable horizontal bounds and to help avoid overlaps.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Enemy_GetWordHalfWidthWorld(const std::string& word);

    /*
    @function       Enemy_GetLaneX
    @params         lane_id (int), usable_min_x (float), usable_max_x (float)
    @brief          Computes the world X position for a given lane index.

    This function maps a lane id into the usable horizontal range and returns the
    corresponding X coordinate. If there is only one lane, it returns the midpoint of the usable range.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    float Enemy_GetLaneX(int lane_id, float usable_min_x, float usable_max_x);

    /*
    @function       Enemy_IsLaneSpawnYFree
    @params         lane_id (int), spawn_y (float), pool (const std::vector<Data::Enemy>&)
    @brief          Checks whether a lane has enough vertical space to spawn a new enemy at the spawn line.

    This function scans all active enemies in the specified lane and finds the closest enemy below the
    spawn line (smallest positive gap). It returns true only if the nearest gap is at least
    ENEMY_LANE_MIN_GAP_PX, ensuring newly spawned enemies won't overlap or stack too tightly.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Enemy_IsLaneSpawnYFree(int lane_id, float spawn_y, const std::vector<Data::Enemy>& pool);

    /*
    @function       Enemy_Create
    @params         area (const Data::EnemyArea&), word_length (int), word_text (const std::string&), fall_speed (float), lane_id (int)
    @brief          Creates and initializes a new enemy instance with the provided spawn settings.

    This function constructs a Data::Enemy with its starting position at the spawn line, assigns its word
    (text and length), fall speed, and lane id, and resets all state flags, timers, and animation fields to
    their initial values so the enemy is ready to update and render.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    Data::Enemy Enemy_Create(const Data::EnemyArea& area, int word_length, const std::string& word_text, float fall_speed, int lane_id);

    /*
    @function       Enemy_BreakShield
    @params         e (Data::Enemy&)
    @brief          Breaks the shield on a shielded enemy and switches it to its second word.

    This function removes the shield from a shielded enemy. It swaps in the hidden second word,
    updates the word length, and resets the animation to the unshielded row of the sprite sheet.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_BreakShield(Data::Enemy& e);

    /*
    @function       Enemy_Killed
    @params         e (Data::Enemy&)
    @brief          Marks an enemy as killed and switches it into the death drop state.

    This function safely transitions an active enemy into a dying state (only once). It clears the landed
    flag so the enemy can drop off-screen, and marks damage as already handled so a killed enemy will not
    apply landing damage.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_Killed(Data::Enemy& e, Data::Health& health);

    /*
    @function       Enemy_Update
    @params         e (Data::Enemy&), area (const Data::EnemyArea&), dt (float), health (Data::Health&)
    @brief          Updates an enemy's animation, movement, and state transitions for the current frame.

    This function handles both the normal falling behaviour and the dying "death drop" behaviour. It updates
    animation frames, moves the enemy downward, clamps it within the enemy area, applies landing damage once,
    and deactivates the enemy when its lifecycle is complete (landed or finished dying + linger).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_Update(Data::Enemy& e, const Data::EnemyArea& area, float dt, Data::Health& health);

    /*
    @function       Enemy_ResolveLaneStacking
    @params         pool (std::vector<Data::Enemy>&), area (const Data::EnemyArea&)
    @brief          Enforces minimum vertical spacing between enemies within each lane.

    This function groups active, non-dying enemies by lane, sorts them by Y position (lowest first),
    and then adjusts positions so each enemy is at least ENEMY_LANE_MIN_GAP_PX above the one below it.
    Adjusted positions are clamped to the top of the enemy area to keep enemies within bounds.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_ResolveLaneStacking(std::vector<Data::Enemy>& pool, const Data::EnemyArea& area);

    /*
    @function       Enemy_Draw
    @params         meshes (const Data::Meshes&), e (const Data::Enemy&)
    @brief          Renders the enemy sprite at its current position using the correct texture and animation frame.

    This function skips inactive enemies, sets the render state for textured sprites, selects the enemy
    texture based on the enemy's word length, applies the current animation UV offset, and draws the
    enemy quad scaled to ENEMY_SIZE_PX.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_Draw(const Data::Meshes& meshes, const Data::Enemy& e);

    /*
    @function       Enemy_DrawWord
    @params         e (const Data::Enemy&)
    @brief          Draws the enemy's word centered on the enemy in screen space.

    This function converts the enemy's world position to NDC coordinates, measures the rendered text size,
    then offsets the draw position so the word is centered over the enemy sprite. It skips drawing if the
    enemy is inactive, the font is missing, or the enemy has no assigned word.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_DrawWord(const Data::Enemy& e);

    /*
    @function       Enemy_IsSpawnSpotFree
    @params         spawn_x (float), spawn_y (float), pool (const std::vector<Data::Enemy>&), half_size_with_padding (float)
    @brief          Checks whether a proposed spawn position would overlap any active enemy.

    This function tests the candidate spawn AABB against all active enemies in the pool using
    Utility::Collision_Overlap. It returns true if no overlaps are found, and false if the new enemy
    would collide with an existing one (including padding).
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Enemy_IsSpawnSpotFree(float spawn_x, float spawn_y, const std::vector<Data::Enemy>& pool, float half_size_with_padding);

    /*
    @function       Enemy_FindSpawnLane
    @params         pool (const std::vector<Data::Enemy>&), area (const Data::EnemyArea&), overlap_half (float), usable_min_x (float), usable_max_x (float), out_chosen_x (float&), out_chosen_lane (int&)
    @brief          Searches for a free lane and x position to spawn an enemy at the spawn line.

    This function first tries random lanes up to MAX_SPAWN_TRIES, then falls back to scanning all lanes
    in order. Returns true if a valid lane was found, false otherwise. The chosen x and lane index are
    written to out_chosen_x and out_chosen_lane.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    bool Enemy_FindSpawnLane(const std::vector<Data::Enemy>& pool, const Data::EnemyArea& area, float overlap_half, float usable_min_x, float usable_max_x, float& out_chosen_x, int& out_chosen_lane);

    /*
    @function       Enemy_SpawnFromPool
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), word_length (int)
    @brief          Spawns a new enemy into an available pool slot using lane based, non overlapping placement.

    This function pulls a free enemy slot, selects a unique word of the requested length, computes safe
    horizontal bounds based on sprite size and word width, then searches for a valid lane position at the
    spawn line. It first tries random lanes up to MAX_SPAWN_TRIES, then falls back to scanning all lanes.
    A spawn is accepted only if the lane has enough vertical spacing near the spawn line and the new enemy
    won't overlap any active enemy. When a valid spot is found, the enemy is created with a difficulty-
    scaled fall speed and inserted into the pool.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_SpawnFromPool(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, int word_length);

    /*
    @function       Enemy_SpawnFriendlyFromPool
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), min_word_len (int), max_word_len (int), speed_multiplier (float);
    @brief          Spawns a friendly enemy into an available pool slot using lane based placement.

    This function works the same as Enemy_SpawnFromPool but creates a friendly enemy. Friendly
    enemies do not damage the player when they land. The speed_multiplier scales the fall speed.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_SpawnFriendlyFromPool(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, int min_word_len, int max_word_len, float speed_multiplier);

    /*
    @function       Enemy_SpawnShieldedFromPool
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&), area (const Data::EnemyArea&), bank (const Data::WordBank&), min_word_len (int), max_word_len (int), speed_multiplier (float);
    @brief          Spawns a shielded enemy into an available pool slot using lane based placement.

    This function spawns a shielded enemy that needs two words to defeat. It picks two unique
    words: the first is shown while the shield is up, the second is revealed after the shield breaks.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Enemy_SpawnShieldedFromPool(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots, const Data::EnemyArea& area, const Data::WordBank& bank, int min_word_len, int max_word_len, float speed_multiplier);

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
    void Debug_DrawEnemyAreaBox(const Data::Meshes& meshes, const Data::EnemyArea& area);

    /*
    @function       Debug_DrawEnemyLaneLines
    @params         meshes (const Data::Meshes&), area (const Data::EnemyArea&)
    @brief          Draws vertical debug lines to visualize each enemy lane inside the enemy area.

    This function renders one vertical line per lane across the enemy area height, using distinct colors
    to make lanes easy to tell apart. It helps debug lane placement, spawn positions, and spacing rules.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Debug_DrawEnemyLaneLines(const Data::Meshes& meshes, const Data::EnemyArea& area);

    /*
    @function       Debug_DrawDifficultyTopRight
    @params         show (bool), auto_spawn_on (bool), health (const Data::Health&)
    @brief          Draws a debug HUD line showing health, difficulty, spawn pacing, and word length unlocks.

    This function prints a single line of diagnostic text near the top right area of the screen, including
    current and max HP, the current difficulty value, the current spawn interval, whether auto spawn is enabled,
    and the unlocked word length range.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Debug_DrawDifficultyTopRight(bool show, bool auto_spawn_on, const Data::Health& health);

    /*
    @author         Jeremy
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void Ui_DrawHealthBar(Data::Health& h, Data::Meshes& meshes);

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
    void EnemyManage_InitEngine(Data::WordBank& bank);

    /*
    @function       EnemyManage_LoadMeshes
    @params         enemy_anim (const Data::SpriteAnim&)
    @brief          Loads enemy textures and builds the meshes needed for rendering.

    This function loads the enemy sprite textures for word lengths 3 through 8, builds the textured quad
    mesh used to draw animated enemy sprites (using the sprite sheet UV frame size from 'enemy_anim'),
    and builds a unit rectangle mesh used for debug drawing. It returns a Data::Meshes struct containing
    the created mesh handles.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    Data::Meshes EnemyManage_LoadMeshes(const Data::SpriteAnim& enemy_anim);

    /*
    @function       EnemyManage_FreeMeshes
    @params         meshes (Data::Meshes&)
    @brief          Releases all loaded meshes, textures, and font resources used by the game.

    This function frees the mesh objects stored in 'meshes', unloads all enemy textures (3 to 8) from the
    Graphics texture table, and destroys the loaded font. It also resets pointers/handles back to safe
    defaults to prevent dangling references.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void EnemyManage_FreeMeshes(Data::Meshes& meshes);

    /*
    @function       EnemyPool_Free
    @params         pool (std::vector<Data::Enemy>&), free_slots (std::deque<int>&)
    @brief          Deactivates all enemies in the pool and rebuilds the free slot queue.

    Iterates every slot regardless of enemy type (normal, friendly, shielded).  Marks each slot
    inactive, clears its word and state, then pushes its index back into free_slots so the pool
    is ready for a clean re-init without a full resize.  The boss slot must already have been
    returned by Boss_Free before this is called.
    *//*_____________________________________________________________________________________________________________________________________________________________________________________________*/
    void EnemyPool_Free(std::vector<Data::Enemy>& pool, std::deque<int>& free_slots);

} // EnemyManage namespace end
/*___________________________________________________________________________________________________________________________________________________________________________________________________*/