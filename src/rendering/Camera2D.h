#pragma once
#include "core/Math.h"

namespace Nova {

class Camera2D {
public:
    Camera2D(const Vec2& position, const Vec2& viewportSize);
    ~Camera2D() = default;

    void Update(float dt);

    // Position
    Vec2 GetPosition() const { return position_; }
    void SetPosition(const Vec2& pos) { position_ = pos; }
    void Move(const Vec2& offset) { position_ += offset; }

    // Zoom
    float GetZoom() const { return zoom_; }
    void SetZoom(float z) { zoom_ = Clamp(z, minZoom_, maxZoom_); }
    void ZoomBy(float delta) { SetZoom(zoom_ + delta); }
    void SetZoomLimits(float min, float max) { minZoom_ = min; maxZoom_ = max; }

    // Rotation
    float GetRotation() const { return rotation_; }
    void SetRotation(float r) { rotation_ = r; }

    // Viewport
    Vec2 GetViewportSize() const { return viewportSize_; }
    void SetViewportSize(const Vec2& size) { viewportSize_ = size; }
    Rect2 GetViewRect() const;

    // Follow target
    void SetFollowTarget(const Vec2* target, float smoothing = 5.0f);
    void ClearFollowTarget() { followTarget_ = nullptr; }
    void SetFollowOffset(const Vec2& offset) { followOffset_ = offset; }
    void SetFollowDeadzone(const Rect2& deadzone) { deadzone_ = deadzone; useDeadzone_ = true; }

    // Screen shake
    void Shake(float intensity, float duration, float frequency = 30.0f);
    bool IsShaking() const { return shakeTimer_ > 0.0f; }

    // Bounds (limits camera movement)
    void SetBounds(const Rect2& bounds) { bounds_ = bounds; useBounds_ = true; }
    void ClearBounds() { useBounds_ = false; }

private:
    Vec2  position_;
    Vec2  viewportSize_;
    float zoom_ = 1.0f;
    float rotation_ = 0.0f;
    float minZoom_ = 0.1f;
    float maxZoom_ = 10.0f;

    // Follow
    const Vec2* followTarget_ = nullptr;
    float followSmoothing_ = 5.0f;
    Vec2 followOffset_;
    Rect2 deadzone_;
    bool useDeadzone_ = false;

    // Shake
    float shakeIntensity_ = 0.0f;
    float shakeTimer_ = 0.0f;
    float shakeDuration_ = 0.0f;
    float shakeFrequency_ = 30.0f;
    Vec2  shakeOffset_;

    // Bounds
    Rect2 bounds_;
    bool useBounds_ = false;
};

} // namespace Nova
