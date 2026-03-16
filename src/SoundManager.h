#pragma once


namespace SoundManager
{
    void Init();
    void Exit();

    // --- BGM ---
    void PlayBackgroundMusic();

    // --- Ambient (Weather) ---
    void PlayRainAmbient();
    void PlaySnowAmbient();

    // --- SFX ---
    void PlayGameOverSfx();
    void PlayPlayerHitSfx();
    void PlayEnemyDeathSfx();
    void PlayClickBtnSfx();
    void PlayTypingSfx();
    void PlayBossSfx();
    void PlayLightningSfx();
    void PlayShieldSfx();
    void PlaySlowSfx();

    // --- Volume Controls ---
    void SetMasterVolume(float volume);
    void SetBGMVolume(float volume);
    void SetSFXVolume(float volume);
    void SetAmbientVolume(float volume); 
};