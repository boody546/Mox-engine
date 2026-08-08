#pragma once
#include "core/Math.h"
#include <SDL.h>

namespace Nova {

class Camera2D;
class Texture;

// Flip modes for sprite rendering
enum class FlipMode { None, Horizontal, Vertical, Both };

class Renderer2D {
public:
    explicit Renderer2D(SDL_Renderer* renderer);
    ~Renderer2D() = default;

    void SetCamera(Camera2D* camera) { camera_ = camera; }
    Camera2D* GetCamera() const { return camera_; }

    // Draw textured sprite
    void DrawSprite(Texture* texture, const Vec2& position,
                    float rotation = 0.0f, const Vec2& scale = Vec2::One(),
                    const Color& tint = Color::White(), FlipMode flip = FlipMode::None,
                    const Vec2& origin = Vec2(0.5f, 0.5f));

    // Draw sub-region of texture (for sprite sheets)
    void DrawSpriteRegion(Texture* texture, const Rect2& srcRect,
                          const Vec2& position, float rotation = 0.0f,
                          const Vec2& scale = Vec2::One(),
                          const Color& tint = Color::White(),
                          FlipMode flip = FlipMode::None,
                          const Vec2& origin = Vec2(0.5f, 0.5f));

    // Draw primitives
    void DrawRect(const Rect2& rect, const Color& color, bool filled = true);
    void DrawRectEx(const Vec2& pos, const Vec2& size, float rotation,
                    const Color& color, bool filled = true);
    void DrawLine(const Vec2& from, const Vec2& to, const Color& color);
    void DrawCircle(const Vec2& center, float radius, const Color& color,
                    bool filled = true, int segments = 32);
    void DrawPoint(const Vec2& pos, const Color& color);
    void DrawPolygon(const Vec2* points, int count, const Color& color,
                     bool filled = false);

    // World-to-screen transform (applies camera)
    Vec2 WorldToScreen(const Vec2& worldPos) const;
    Vec2 ScreenToWorld(const Vec2& screenPos) const;

    // Stats
    int GetDrawCallCount() const { return drawCalls_; }
    void ResetStats() { drawCalls_ = 0; }

private:
    SDL_Renderer* renderer_;
    Camera2D* camera_ = nullptr;
    int drawCalls_ = 0;
};

} // namespace Nova
