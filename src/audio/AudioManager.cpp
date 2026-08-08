#include "audio/AudioManager.h"

namespace Nova {

AudioManager::~AudioManager() { Shutdown(); }

bool AudioManager::Init() {
    if (initialized_) return true;
    
    // SDL audio should already be initialized by Engine
    initialized_ = true;
    NOVA_LOG("Audio subsystem ready");
    return true;
}

void AudioManager::Shutdown() {
    StopAllSounds();
    StopMusic();

    for (auto& [name, data] : sounds_) {
        if (data.buffer) SDL_FreeWAV(data.buffer);
    }
    sounds_.clear();

    for (auto& [name, data] : music_) {
        if (data.buffer) SDL_FreeWAV(data.buffer);
    }
    music_.clear();

    if (deviceId_) {
        SDL_CloseAudioDevice(deviceId_);
        deviceId_ = 0;
    }
    initialized_ = false;
}

bool AudioManager::LoadSound(const std::string& name, const std::string& path) {
    SoundData data;
    if (SDL_LoadWAV(path.c_str(), &data.spec, &data.buffer, &data.length) == nullptr) {
        NOVA_ERROR("Failed to load sound '", name, "': ", SDL_GetError());
        return false;
    }
    sounds_[name] = data;
    NOVA_TRACE("Sound loaded: ", name);
    return true;
}

void AudioManager::PlaySound(const std::string& name, int loops, float volume) {
    if (muted_) return;
    auto it = sounds_.find(name);
    if (it == sounds_.end()) {
        NOVA_WARN("Sound not found: ", name);
        return;
    }

    auto& data = it->second;
    
    // Open audio device if not yet opened
    if (!deviceId_) {
        deviceId_ = SDL_OpenAudioDevice(nullptr, 0, &data.spec, nullptr, 0);
        if (!deviceId_) {
            NOVA_ERROR("Failed to open audio device: ", SDL_GetError());
            return;
        }
        SDL_PauseAudioDevice(deviceId_, 0);
    }

    // Adjust volume
    float vol = volume * masterVolume_;
    Uint32 len = data.length;
    auto* mixed = new Uint8[len];
    SDL_memset(mixed, 0, len);
    SDL_MixAudioFormat(mixed, data.buffer, data.spec.format, len,
                        (int)(SDL_MIX_MAXVOLUME * vol));
    
    SDL_QueueAudio(deviceId_, mixed, len);
    delete[] mixed;
    
    (void)loops; // Simple implementation - loops handled elsewhere
}

void AudioManager::StopSound(const std::string& name) {
    (void)name;
    // With SDL's simple queue, stopping individual sounds requires
    // a more complex mixing system. Clear the queue for now.
    if (deviceId_) SDL_ClearQueuedAudio(deviceId_);
}

void AudioManager::StopAllSounds() {
    if (deviceId_) SDL_ClearQueuedAudio(deviceId_);
}

bool AudioManager::LoadMusic(const std::string& name, const std::string& path) {
    return LoadSound(name, path); // Same loading mechanism
}

void AudioManager::PlayMusic(const std::string& name, bool loop, float volume) {
    PlaySound(name, loop ? -1 : 0, volume);
    musicPlaying_ = true;
}

void AudioManager::StopMusic() {
    StopAllSounds();
    musicPlaying_ = false;
}

void AudioManager::PauseMusic() {
    if (deviceId_) SDL_PauseAudioDevice(deviceId_, 1);
}

void AudioManager::ResumeMusic() {
    if (deviceId_) SDL_PauseAudioDevice(deviceId_, 0);
}

void AudioManager::SetMusicVolume(float volume) {
    masterVolume_ = volume;
}

} // namespace Nova
