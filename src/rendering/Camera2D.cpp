#include "rendering/Camera2D.h"
#include <cmath>

namespace Nova {

Camera2D::Camera2D(const Vec2& position, const Vec2& viewportSize)
    : position_(position), viewportSize_(viewportSize) {}

void Camera2D::Update(float dt) {
    // Follow target
    if (followTarget_) {
        Vec2 targetPos = *followTarget_ + followOffset_;

        if (useDeadzone_) {
            Vec2 diff = targetPos - position_;
            if (diff.x > deadzone_.size.x * 0.5f)
                position_.x = targetPos.x - deadzone_.size.x * 0.5f;
            else if (diff.x < -deadzone_.size.x * 0.5f)
                position_.x = targetPos.x + deadzone_.size.x * 0.5f;
            if (diff.y > deadzone_.size.y * 0.5f)
                position_.y = targetPos.y - deadzone_.size.y * 0.5f;
            else if (diff.y < -deadzone_.size.y * 0.5f)
                position_.y = targetPos.y + deadzone_.size.y * 0.5f;
        } else {
            position_ = position_.LerpTo(targetPos, 1.0f - std::exp(-followSmoothing_ * dt));
        }
    }

    // Apply bounds
    if (useBounds_) {
        float halfW = (viewportSize_.x / zoom_) * 0.5f;
        float halfH = (viewportSize_.y / zoom_) * 0.5f;
        position_.x = Clamp(position_.x, bounds_.Left() + halfW, bounds_.Right() - halfW);
        position_.y = Clamp(position_.y, bounds_.Top() + halfH, bounds_.Bottom() - halfH);
    }

    // Screen shake
    if (shakeTimer_ > 0.0f) {
        shakeTimer_ -= dt;
        float progress = 1.0f - (shakeTimer_ / shakeDuration_);
        float decay = 1.0f - progress;
        float t = shakeTimer_ * shakeFrequency_;
        shakeOffset_ = Vec2(
            std::sin(t * 1.1f) * shakeIntensity_ * decay,
            std::cos(t * 1.3f) * shakeIntensity_ * decay
        );
    } else {
        shakeOffset_ = Vec2::Zero();
    }
}

Rect2 Camera2D::GetViewRect() const {
    float w = viewportSize_.x / zoom_;
    float h = viewportSize_.y / zoom_;
    return Rect2(
        position_.x - w * 0.5f + shakeOffset_.x,
        position_.y - h * 0.5f + shakeOffset_.y,
        w, h
    );
}

void Camera2D::SetFollowTarget(const Vec2* target, float smoothing) {
    followTarget_ = target;
    followSmoothing_ = smoothing;
}

void Camera2D::Shake(float intensity, float duration, float frequency) {
    shakeIntensity_ = intensity;
    shakeDuration_ = duration;
    shakeTimer_ = duration;
    shakeFrequency_ = frequency;
}

} // namespace Nova
