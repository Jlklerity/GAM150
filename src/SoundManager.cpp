#include "SoundManager.h"
#include "GameStateManager.h"
#include "MainMenu.h"
#include <AEEngine.h>

namespace
{
    AEAudioGroup bgmGroup;
    AEAudioGroup sfxGroup;
    AEAudioGroup ambientGroup;

    AEAudio bgmTrack;
    AEAudio rainTrack;
    AEAudio snowTrack;
    AEAudio gameoverSfx;
    AEAudio playerhitSfx;
    AEAudio enemydeathSfx;
    AEAudio clickbtnSfx;
    AEAudio typingSfx;
    AEAudio bossSfx;
    AEAudio lightningSfx;
    AEAudio shieldSfx;
    AEAudio slowSfx;

    bool isAudioLoaded = false;
    float masterVolume = 1.0f; // 100%
    float bgmVolume = 0.5f; // 50%
    float sfxVolume = 0.8f; // 80%
    float ambientVolume = 0.6f; // 60%

    void UpdateVolumes()
    {
        // Multiply the specific volume by the master volume before telling the engine
        AEAudioSetGroupVolume(bgmGroup, bgmVolume * masterVolume); 
        AEAudioSetGroupVolume(sfxGroup, sfxVolume * masterVolume);  
        AEAudioSetGroupVolume(ambientGroup, ambientVolume * masterVolume); 
    }
}

namespace SoundManager
{
    void Init()
    {
        if (gGameStatePrev == GS_MAIN_MENU || gGameStatePrev == GS_SHOP || gGameStatePrev == GS_CREDITS) {
            return;
        }
        // Create audio groups
        if (isAudioLoaded) return;
        bgmGroup = AEAudioCreateGroup();
        sfxGroup = AEAudioCreateGroup();
        ambientGroup = AEAudioCreateGroup();

        // Load audio files
        bgmTrack = AEAudioLoadMusic("Assets/Game Sound/RUNNING THE BASS - Jazz MSCJAZ1_20.mp3");
        rainTrack = AEAudioLoadMusic("Assets/Game Sound/RAIN_GEN-HDF-19746.mp3");
        snowTrack = AEAudioLoadMusic("Assets/Game Sound/SCI-FI-WIND_GEN-HDF-20895.mp3");
        typingSfx = AEAudioLoadSound("Assets/Game Sound/COMPUTER-KEYBOARD_GEN-HDF-08220.wav");  
        gameoverSfx = AEAudioLoadSound("Assets/Game Sound/FluteComedy CTE05_42.3.mp3");
        playerhitSfx = AEAudioLoadSound("Assets/Game Sound/HUMAN-GRUNT_GEN-HDF-15069.wav");    
        enemydeathSfx = AEAudioLoadSound("Assets/Game Sound/HORROR_GEN-HDF-14351.wav");
        clickbtnSfx = AEAudioLoadSound("Assets/Game Sound/COMPUTER-LAPTOP_GEN-HDF-08250.wav");
        bossSfx = AEAudioLoadSound("Assets/Game Sound/ALARM-ELECTRONIC_GEN-HDF-00613(1).mp3");
        lightningSfx = AEAudioLoadSound("Assets/Game Sound/Lightning_5sec CTE01_90.5.mp3");
        shieldSfx = AEAudioLoadSound("Assets/Game Sound/HitMetal CTE01_70.3.wav");
        slowSfx = AEAudioLoadSound("Assets/Game Sound/Scifi_Maltfuncti.mp3");

        // Set default volumes
        AEAudioSetGroupVolume(bgmGroup, 0.5f);
        AEAudioSetGroupVolume(sfxGroup, 0.8f);
        AEAudioSetGroupVolume(ambientGroup, 0.6f);
         
        UpdateVolumes();
        isAudioLoaded = true;
        if (gGameStateCurr == GS_MAIN_MENU)
        {
            PlayBackgroundMusic();
        }
    }

    void Exit()
    {
        if (gGameStateNext == GS_MAIN_MENU || gGameStateNext == GS_SHOP || gGameStateNext == GS_CREDITS) {
            return;
        }
        // Unload individual sounds to free memory
        if (!isAudioLoaded) return;
        AEAudioUnloadAudio(bgmTrack);
        AEAudioUnloadAudio(rainTrack);
        AEAudioUnloadAudio(snowTrack);
        AEAudioUnloadAudio(gameoverSfx);
        AEAudioUnloadAudio(playerhitSfx);
        AEAudioUnloadAudio(enemydeathSfx);
        AEAudioUnloadAudio(clickbtnSfx);
        AEAudioUnloadAudio(typingSfx);
        AEAudioUnloadAudio(bossSfx);
        AEAudioUnloadAudio(lightningSfx);
        AEAudioUnloadAudio(shieldSfx);
        AEAudioUnloadAudio(slowSfx);

        // Unload the groups
        AEAudioUnloadAudioGroup(bgmGroup);
        AEAudioUnloadAudioGroup(sfxGroup);
        AEAudioUnloadAudioGroup(ambientGroup);
        isAudioLoaded = false;
    }

    void PlayBackgroundMusic()
    {
        AEAudioPlay(bgmTrack, bgmGroup, 1.0f, 1.0f, -1);
    }

    void PlayRainAmbient()
    {
        AEAudioPlay(rainTrack, ambientGroup, 1.0f, 1.0f, -1);
    }

    void PlaySnowAmbient()
    {
        AEAudioPlay(snowTrack, ambientGroup, 1.0f, 1.0f, -1);
    }

    void PlayGameOverSfx()
    {
        AEAudioPlay(gameoverSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlayPlayerHitSfx()
    {
        AEAudioPlay(playerhitSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlayEnemyDeathSfx()
    {
        AEAudioPlay(enemydeathSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlayClickBtnSfx()
    {
        AEAudioPlay(clickbtnSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlayTypingSfx()
    {
        AEAudioPlay(typingSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlayBossSfx()
    {
        AEAudioPlay(bossSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlayLightningSfx()
    {
        AEAudioPlay(lightningSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlayShieldSfx()
    {
        AEAudioPlay(shieldSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    void PlaySlowSfx()
    {
        AEAudioPlay(slowSfx, sfxGroup, 1.0f, 1.0f, 0);
    }

    // ==========================================
    // VOLUME SETTERS
    // ==========================================
    void SetMasterVolume(float volume)
    {
        masterVolume = volume;
        UpdateVolumes();
    }

    void SetBGMVolume(float volume)
    {
        bgmVolume = volume;
        UpdateVolumes();
    }

    void SetSFXVolume(float volume)
    {
        sfxVolume = volume;
        UpdateVolumes();
    }

    void SetAmbientVolume(float volume)
    {
        ambientVolume = volume;
        UpdateVolumes();
    }

}