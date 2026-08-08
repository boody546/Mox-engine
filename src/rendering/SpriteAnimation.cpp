#include "rendering/SpriteAnimation.h"
#include "core/Logger.h"

namespace Nova {

void SpriteAnimation::SetupFromGrid(Texture* sheet, int cols, int rows, float frameDuration) {
    texture_ = sheet;
    gridCols_ = cols;
    gridRows_ = rows;
    if (sheet) {
        frameWidth_ = sheet->GetWidth() / cols;
        frameHeight_ = sheet->GetHeight() / rows;
    }

    // Create default "all" track
    std::vector<int> allFrames;
    for (int i = 0; i < cols * rows; i++) allFrames.push_back(i);
    AddTrack("default", allFrames, frameDuration);
}

void SpriteAnimation::AddTrack(const std::string& name, const std::vector<int>& frameIndices,
                                float frameDuration, bool loop) {
    AnimationTrack track;
    track.name = name;
    track.loop = loop;
    for (int idx : frameIndices) {
        track.frames.push_back({GetFrameRect(idx), frameDuration});
    }
    tracks_[name] = std::move(track);
}

void SpriteAnimation::AddTrackFromRow(const std::string& name, int row, int startCol,
                                       int numFrames, float frameDuration, bool loop) {
    std::vector<int> indices;
    for (int i = 0; i < numFrames; i++) {
        indices.push_back(row * gridCols_ + startCol + i);
    }
    AddTrack(name, indices, frameDuration, loop);
}

void SpriteAnimation::Play(const std::string& trackName) {
    if (currentTrack_ == trackName && playing_ && !finished_) return;
    auto it = tracks_.find(trackName);
    if (it == tracks_.end()) {
        NOVA_WARN("Animation track not found: ", trackName);
        return;
    }
    currentTrack_ = trackName;
    currentFrame_ = 0;
    frameTimer_ = 0.0f;
    playing_ = true;
    paused_ = false;
    finished_ = false;
}

void SpriteAnimation::Stop() {
    playing_ = false;
    paused_ = false;
    currentFrame_ = 0;
    frameTimer_ = 0.0f;
}

void SpriteAnimation::Update(float dt) {
    if (!playing_ || paused_ || finished_) return;

    auto it = tracks_.find(currentTrack_);
    if (it == tracks_.end() || it->second.frames.empty()) return;

    auto& track = it->second;
    frameTimer_ += dt * playbackSpeed_ * track.speed;

    float frameDur = track.frames[currentFrame_].duration;
    while (frameTimer_ >= frameDur) {
        frameTimer_ -= frameDur;
        currentFrame_++;

        if (currentFrame_ >= (int)track.frames.size()) {
            if (track.loop) {
                currentFrame_ = 0;
            } else {
                currentFrame_ = (int)track.frames.size() - 1;
                finished_ = true;
                playing_ = false;
                if (onFinish_) onFinish_();
                return;
            }
        }
        frameDur = track.frames[currentFrame_].duration;
    }
}

Rect2 SpriteAnimation::GetCurrentFrameRect() const {
    auto it = tracks_.find(currentTrack_);
    if (it == tracks_.end() || it->second.frames.empty())
        return Rect2(0, 0, (float)frameWidth_, (float)frameHeight_);
    return it->second.frames[currentFrame_].region;
}

Rect2 SpriteAnimation::GetFrameRect(int frameIndex) const {
    int col = frameIndex % gridCols_;
    int row = frameIndex / gridCols_;
    return Rect2((float)(col * frameWidth_), (float)(row * frameHeight_),
                 (float)frameWidth_, (float)frameHeight_);
}

} // namespace Nova
