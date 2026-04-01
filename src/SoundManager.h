#pragma once

/*!
@file           SoundManager.cpp
@author         Jeremy Lim
@brief          Centralized audio system utilizing FMOD/AlphaEngine to play, pause, and mix BGM, SFX, and Ambient tracks.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace SoundManager
{
    void init();
    void cleanup();

    // --- BGM ---
    void play_background_music();

    // --- Ambient (Weather) ---
    void play_rain_ambient();
    void stop_rain_ambient();
    void play_snow_ambient();
    void stop_snow_ambient();

    // --- SFX ---
    void play_game_over_sfx();
    void play_player_hit_sfx();
    void play_enemy_death_sfx();
    void play_click_btn_sfx();
    void play_typing_sfx();
    void play_boss_sfx();
    void stop_boss_sfx();
    void play_lightning_sfx();
    void play_shield_sfx();
    void play_slow_sfx();
    void pause_gameplay_sounds();
    void resume_gameplay_sounds();

    float get_bgm_volume();
    float get_sfx_volume();
    float get_ambient_volume();

    // --- Volume Controls ---
    void set_master_volume(float volume);
    void set_bgm_volume(float volume);
    void set_sfx_volume(float volume);
    void set_ambient_volume(float volume);
}