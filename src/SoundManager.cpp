#include "pch.h"
#include "SoundManager.h"

/*!
@file           SoundManager.cpp
@author         Jeremy Lim
@brief          Centralized audio system utilizing FMOD/AlphaEngine to play, pause, and mix BGM, SFX, and Ambient tracks.
*//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
namespace
{
    static AEAudioGroup s_bgm_group;
    static AEAudioGroup s_sfx_group;
    static AEAudioGroup s_ambient_group;
    static AEAudioGroup s_ui_group;
    static AEAudioGroup s_boss_alarm_group;

    static AEAudio s_bgm_track;
    static AEAudio s_rain_track;
    static AEAudio s_snow_track;
    static AEAudio s_gameover_sfx;
    static AEAudio s_player_hit_sfx;
    static AEAudio s_enemy_death_sfx;
    static AEAudio s_click_btn_sfx;
    static AEAudio s_typing_sfx;
    static AEAudio s_boss_sfx;
    static AEAudio s_lightning_sfx;
    static AEAudio s_shield_sfx;
    static AEAudio s_slow_sfx;

    static bool  s_is_audio_loaded = false;
    static float s_master_volume = 1.0f;
    static float s_bgm_volume = 1.0f;
    static float s_sfx_volume = 1.0f;
    static float s_ambient_volume = 1.0f;

    static void update_volumes()
    {
        AEAudioSetGroupVolume(s_bgm_group, s_bgm_volume * s_master_volume);
        AEAudioSetGroupVolume(s_sfx_group, s_sfx_volume * s_master_volume);
        AEAudioSetGroupVolume(s_ambient_group, s_ambient_volume * s_master_volume);
        AEAudioSetGroupVolume(s_ui_group, s_sfx_volume * s_master_volume);
        AEAudioSetGroupVolume(s_boss_alarm_group, s_sfx_volume * s_master_volume);
    }
}

namespace SoundManager
{
    
    /*!
    @function       init
    @brief          Initializes audio groups, loads all music and sound effect assets from disk, and sets default volume levels.
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void init()
    {
        static bool s_is_first_boot = false;

        if (s_is_first_boot)
        {
            s_bgm_volume = 1.0f;
            s_sfx_volume = 1.0f;
            s_ambient_volume = 1.0f;
            s_is_first_boot = false;
        }

        if (!s_is_audio_loaded)
        {
            s_bgm_group = AEAudioCreateGroup();
            s_sfx_group = AEAudioCreateGroup();
            s_ambient_group = AEAudioCreateGroup();
            s_ui_group = AEAudioCreateGroup();
            s_boss_alarm_group = AEAudioCreateGroup();

            s_bgm_track = AEAudioLoadMusic("Assets/Game Sound/RUNNING THE BASS - Jazz MSCJAZ1_20.mp3");

            // Load audio files
            s_bgm_track = AEAudioLoadMusic("Assets/Game Sound/RUNNING THE BASS - Jazz MSCJAZ1_20.mp3");
            s_rain_track = AEAudioLoadMusic("Assets/Game Sound/RAIN_GEN-HDF-19746.mp3");
            s_snow_track = AEAudioLoadMusic("Assets/Game Sound/SCI-FI-WIND_GEN-HDF-20895.mp3");
            s_typing_sfx = AEAudioLoadSound("Assets/Game Sound/COMPUTER-KEYBOARD_GEN-HDF-08220.wav");
            s_gameover_sfx = AEAudioLoadSound("Assets/Game Sound/FluteComedy CTE05_42.3.wav");
            s_player_hit_sfx = AEAudioLoadSound("Assets/Game Sound/HUMAN-GRUNT_GEN-HDF-15069.wav");
            s_enemy_death_sfx = AEAudioLoadSound("Assets/Game Sound/HORROR_GEN-HDF-14351.wav");
            s_click_btn_sfx = AEAudioLoadSound("Assets/Game Sound/COMPUTER-LAPTOP_GEN-HDF-08250.wav");
            s_boss_sfx = AEAudioLoadSound("Assets/Game Sound/ALARM-ELECTRONIC_GEN-HDF-00613(1).mp3");
            s_lightning_sfx = AEAudioLoadSound("Assets/Game Sound/Lightning_5sec CTE01_90.5.mp3");
            s_shield_sfx = AEAudioLoadSound("Assets/Game Sound/HitMetal CTE01_70.3.wav");
            s_slow_sfx = AEAudioLoadSound("Assets/Game Sound/Scifi_Maltfuncti.mp3");

            s_is_audio_loaded = true;
            s_is_first_boot = true;
        }

        if (g_game_state_curr == GS_GAME || g_game_state_curr == GS_GAME_TIMEMODE || g_game_state_curr == GS_TUTORIAL)
        {
            AEAudioStopGroup(s_bgm_group);
            play_background_music();
        }
        else
        {
            if (g_game_state_prev == GS_GAME || g_game_state_prev == GS_GAME_TIMEMODE ||
                g_game_state_prev == GS_TUTORIAL || s_is_first_boot)
            {
                AEAudioStopGroup(s_bgm_group);
                play_background_music();
            }
        }

        update_volumes();
    }

    float get_bgm_volume() { return s_bgm_volume; }
    float get_sfx_volume() { return s_sfx_volume; }
    float get_ambient_volume() { return s_ambient_volume; }

    void cleanup()
    {
        if (g_game_state_next == GS_QUIT)
        {
            if (!s_is_audio_loaded) return;

            AEAudioUnloadAudio(s_bgm_track);
            AEAudioUnloadAudio(s_rain_track);
            AEAudioUnloadAudio(s_snow_track);
            AEAudioUnloadAudio(s_gameover_sfx);
            AEAudioUnloadAudio(s_player_hit_sfx);
            AEAudioUnloadAudio(s_enemy_death_sfx);
            AEAudioUnloadAudio(s_click_btn_sfx);
            AEAudioUnloadAudio(s_typing_sfx);
            AEAudioUnloadAudio(s_boss_sfx);
            AEAudioUnloadAudio(s_lightning_sfx);
            AEAudioUnloadAudio(s_shield_sfx);
            AEAudioUnloadAudio(s_slow_sfx);

            AEAudioUnloadAudioGroup(s_bgm_group);
            AEAudioUnloadAudioGroup(s_sfx_group);
            AEAudioUnloadAudioGroup(s_ambient_group);
            AEAudioUnloadAudioGroup(s_ui_group);
            AEAudioUnloadAudioGroup(s_boss_alarm_group);

            s_is_audio_loaded = false;
        }
    }

    void play_background_music() { AEAudioPlay(s_bgm_track, s_bgm_group, 1.0f, 1.0f, -1); }
    void play_rain_ambient() { AEAudioPlay(s_rain_track, s_ambient_group, 1.0f, 1.0f, -1); }
    void stop_rain_ambient() { AEAudioStopGroup(s_ambient_group); }
    void play_snow_ambient() { AEAudioPlay(s_snow_track, s_ambient_group, 1.0f, 1.0f, -1); }
    void stop_snow_ambient() { AEAudioStopGroup(s_ambient_group); }
    void play_game_over_sfx() { AEAudioPlay(s_gameover_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void play_player_hit_sfx() { AEAudioPlay(s_player_hit_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void play_enemy_death_sfx() { AEAudioPlay(s_enemy_death_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void play_click_btn_sfx() { AEAudioPlay(s_click_btn_sfx, s_ui_group, 1.0f, 1.0f, 0); }
    void play_typing_sfx() { AEAudioPlay(s_typing_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void play_boss_sfx() { AEAudioPlay(s_boss_sfx, s_boss_alarm_group, 1.0f, 1.0f, -1); }
    void stop_boss_sfx() { AEAudioStopGroup(s_boss_alarm_group); }
    void play_lightning_sfx() { AEAudioPlay(s_lightning_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void play_shield_sfx() { AEAudioPlay(s_shield_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void play_slow_sfx() { AEAudioPlay(s_slow_sfx, s_sfx_group, 1.0f, 1.0f, 0); }

    /*!
    @function       pause_gameplay_sounds
    @brief          Pauses all audio groups associated with active gameplay (ambient, SFX, boss alarms).
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void pause_gameplay_sounds()
    {
        AEAudioPauseGroup(s_ambient_group);
        AEAudioPauseGroup(s_sfx_group);
        AEAudioPauseGroup(s_boss_alarm_group);
    }

    void resume_gameplay_sounds()
    {
        AEAudioResumeGroup(s_ambient_group);
        AEAudioResumeGroup(s_sfx_group);
        AEAudioResumeGroup(s_boss_alarm_group);
    }

    /*!
    @function       set_master_volume
    @brief          Updates the master volume multiplier and immediately applies it across all active audio groups.
    @param          volume (float) The new volume percentage (0.0f to 1.0f).
    *//*_________________________________________________________________________________________________________________________________________________________________________________________________*/
    void set_master_volume(float volume) { s_master_volume = volume; update_volumes(); }
    void set_bgm_volume(float volume) { s_bgm_volume = volume; update_volumes(); }
    void set_sfx_volume(float volume) { s_sfx_volume = volume; update_volumes(); }
    void set_ambient_volume(float volume) { s_ambient_volume = volume; update_volumes(); }
}