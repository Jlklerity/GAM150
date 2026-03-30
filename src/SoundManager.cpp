#include "pch.h"
#include "SoundManager.h"


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

    static void UpdateVolumes()
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
    void Init()
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
            PlayBackgroundMusic();
        }
        else
        {
            if (g_game_state_prev == GS_GAME || g_game_state_prev == GS_GAME_TIMEMODE ||
                g_game_state_prev == GS_TUTORIAL || s_is_first_boot)
            {
                AEAudioStopGroup(s_bgm_group);
                PlayBackgroundMusic();
            }
        }

        UpdateVolumes();
    }

    float GetBGMVolume() { return s_bgm_volume; }
    float GetSFXVolume() { return s_sfx_volume; }
    float GetAmbientVolume() { return s_ambient_volume; }

    void Exit()
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

    void PlayBackgroundMusic() { AEAudioPlay(s_bgm_track, s_bgm_group, 1.0f, 1.0f, -1); }
    void PlayRainAmbient() { AEAudioPlay(s_rain_track, s_ambient_group, 1.0f, 1.0f, -1); }
    void StopRainAmbient() { AEAudioStopGroup(s_ambient_group); }
    void PlaySnowAmbient() { AEAudioPlay(s_snow_track, s_ambient_group, 1.0f, 1.0f, -1); }
    void StopSnowAmbient() { AEAudioStopGroup(s_ambient_group); }
    void PlayGameOverSfx() { AEAudioPlay(s_gameover_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void PlayPlayerHitSfx() { AEAudioPlay(s_player_hit_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void PlayEnemyDeathSfx() { AEAudioPlay(s_enemy_death_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void PlayClickBtnSfx() { AEAudioPlay(s_click_btn_sfx, s_ui_group, 1.0f, 1.0f, 0); }
    void PlayTypingSfx() { AEAudioPlay(s_typing_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void PlayBossSfx() { AEAudioPlay(s_boss_sfx, s_boss_alarm_group, 1.0f, 1.0f, -1); }
    void StopBossSfx() { AEAudioStopGroup(s_boss_alarm_group); }
    void PlayLightningSfx() { AEAudioPlay(s_lightning_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void PlayShieldSfx() { AEAudioPlay(s_shield_sfx, s_sfx_group, 1.0f, 1.0f, 0); }
    void PlaySlowSfx() { AEAudioPlay(s_slow_sfx, s_sfx_group, 1.0f, 1.0f, 0); }

    void PauseGameplaySounds()
    {
        AEAudioPauseGroup(s_ambient_group);
        AEAudioPauseGroup(s_sfx_group);
        AEAudioPauseGroup(s_boss_alarm_group);
    }

    void ResumeGameplaySounds()
    {
        AEAudioResumeGroup(s_ambient_group);
        AEAudioResumeGroup(s_sfx_group);
        AEAudioResumeGroup(s_boss_alarm_group);
    }

    void SetMasterVolume(float volume) { s_master_volume = volume; UpdateVolumes(); }
    void SetBGMVolume(float volume) { s_bgm_volume = volume; UpdateVolumes(); }
    void SetSFXVolume(float volume) { s_sfx_volume = volume; UpdateVolumes(); }
    void SetAmbientVolume(float volume) { s_ambient_volume = volume; UpdateVolumes(); }
}