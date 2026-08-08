#pragma once
#include "core/Logger.h"
#include <SDL.h>
#include <string>
#include <unordered_map>

namespace Nova {

// Simple audio manager using SDL2's built-in audio
// For more advanced features, SDL2_mixer can be integrated later

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    bool Init();
    void Shutdown();

    // Sound effects (short clips, loaded fully in memory)
    bool LoadSound(const std::string& name, const std::string& path);
    void PlaySound(const std::string& name, int loops = 0, float volume = 1.0f);
    void StopSound(const std::string& name);
    void StopAllSounds();

    // Music (streamed)
    bool LoadMusic(const std::string& name, const std::string& path);
    void PlayMusic(const std::string& name, bool loop = true, float volume = 1.0f);
    void StopMusic();
    void PauseMusic();
    void ResumeMusic();
    void SetMusicVolume(float volume);
    bool IsMusicPlaying() const { return musicPlaying_; }

    // Master volume
    void SetMasterVolume(float vol) { masterVolume_ = vol; }
    float GetMasterVolume() const { return masterVolume_; }

    // Mute
    void SetMuted(bool muted) { muted_ = muted; }
    bool IsMuted() const { return muted_; }

private:
    struct SoundData {
        Uint8* buffer = nullptr;
        Uint32 length = 0;
        SDL_AudioSpec spec{};
    };

    std::unordered_map<std::string, SoundData> sounds_;
    std::unordered_map<std::string, SoundData> music_;
    
    SDL_AudioDeviceID deviceId_ = 0;
    float masterVolume_ = 1.0f;
    bool muted_ = false;
    bool musicPlaying_ = false;
    bool initialized_ = false;
};

} // namespace Nova
