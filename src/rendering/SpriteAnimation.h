#pragma once
#include "core/Math.h"
#include "rendering/Texture.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>


namespace Nova {

struct AnimationFrame {
    Rect2 region;      // Source rect in the spritesheet
    float duration;    // Duration of this frame in seconds
};

struct AnimationTrack {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool loop = true;
    float speed = 1.0f;
};

class SpriteAnimation {
public:
    SpriteAnimation() = default;
    ~SpriteAnimation() = default;

    // Setup from uniform sprite sheet grid
    void SetupFromGrid(Texture* sheet, int cols, int rows, float frameDuration = 0.1f);

    // Add named animation tracks
    void AddTrack(const std::string& name, const std::vector<int>& frameIndices,
                  float frameDuration = 0.1f, bool loop = true);
    void AddTrackFromRow(const std::string& name, int row, int startCol,
                         int numFrames, float frameDuration = 0.1f, bool loop = true);

    // Playback
    void Play(const std::string& trackName);
    void Stop();
    void Pause() { paused_ = true; }
    void Resume() { paused_ = false; }
    void Update(float dt);

    // State
    bool IsPlaying() const { return playing_ && !paused_; }
    bool IsFinished() const { return finished_; }
    const std::string& GetCurrentTrack() const { return currentTrack_; }
    int GetCurrentFrameIndex() const { return currentFrame_; }
    Rect2 GetCurrentFrameRect() const;
    Texture* GetTexture() const { return texture_; }

    // Speed
    void SetSpeed(float s) { playbackSpeed_ = s; }
    float GetSpeed() const { return playbackSpeed_; }

    // Callbacks
    using FinishCallback = std::function<void()>;
    void SetOnFinish(FinishCallback cb) { onFinish_ = std::move(cb); }

private:
    Texture* texture_ = nullptr;
    int gridCols_ = 1, gridRows_ = 1;
    int frameWidth_ = 0, frameHeight_ = 0;

    std::unordered_map<std::string, AnimationTrack> tracks_;
    std::string currentTrack_;
    int currentFrame_ = 0;
    float frameTimer_ = 0.0f;
    float playbackSpeed_ = 1.0f;
    bool playing_ = false;
    bool paused_ = false;
    bool finished_ = false;

    FinishCallback onFinish_;

    Rect2 GetFrameRect(int frameIndex) const;
};

} // namespace Nova
