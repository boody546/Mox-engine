#include "rendering/Renderer2D.h"
#include "rendering/Texture.h"
#include "rendering/Camera2D.h"
#include "core/Logger.h"
#include <cmath>

namespace Nova {

Renderer2D::Renderer2D(SDL_Renderer* renderer) : renderer_(renderer) {}

Vec2 Renderer2D::WorldToScreen(const Vec2& worldPos) const {
    if (!camera_) return worldPos;
    Vec2 camPos = camera_->GetPosition();
    float zoom = camera_->GetZoom();
    Vec2 viewport = camera_->GetViewportSize();
    return (worldPos - camPos) * zoom + viewport * 0.5f;
}

Vec2 Renderer2D::ScreenToWorld(const Vec2& screenPos) const {
    if (!camera_) return screenPos;
    Vec2 camPos = camera_->GetPosition();
    float zoom = camera_->GetZoom();
    Vec2 viewport = camera_->GetViewportSize();
    return (screenPos - viewport * 0.5f) / zoom + camPos;
}

void Renderer2D::DrawSprite(Texture* texture, const Vec2& position,
                             float rotation, const Vec2& scale,
                             const Color& tint, FlipMode flip,
                             const Vec2& origin) {
    if (!texture || !texture->GetSDLTexture()) return;

    Vec2 screenPos = WorldToScreen(position);
    float zoom = camera_ ? camera_->GetZoom() : 1.0f;

    int w = texture->GetWidth();
    int h = texture->GetHeight();
    float sw = w * scale.x * zoom;
    float sh = h * scale.y * zoom;

    SDL_FRect dst;
    dst.x = screenPos.x - sw * origin.x;
    dst.y = screenPos.y - sh * origin.y;
    dst.w = sw;
    dst.h = sh;

    SDL_FPoint center = {sw * origin.x, sh * origin.y};

    SDL_RendererFlip sdlFlip = SDL_FLIP_NONE;
    if (flip == FlipMode::Horizontal) sdlFlip = SDL_FLIP_HORIZONTAL;
    else if (flip == FlipMode::Vertical) sdlFlip = SDL_FLIP_VERTICAL;
    else if (flip == FlipMode::Both)
        sdlFlip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);

    SDL_SetTextureColorMod(texture->GetSDLTexture(), tint.R8(), tint.G8(), tint.B8());
    SDL_SetTextureAlphaMod(texture->GetSDLTexture(), tint.A8());

    SDL_RenderCopyExF(renderer_, texture->GetSDLTexture(), nullptr, &dst,
                      rotation * RAD2DEG, &center, sdlFlip);
    drawCalls_++;
}

void Renderer2D::DrawSpriteRegion(Texture* texture, const Rect2& srcRect,
                                   const Vec2& position, float rotation,
                                   const Vec2& scale, const Color& tint,
                                   FlipMode flip, const Vec2& origin) {
    if (!texture || !texture->GetSDLTexture()) return;

    Vec2 screenPos = WorldToScreen(position);
    float zoom = camera_ ? camera_->GetZoom() : 1.0f;

    SDL_Rect src;
    src.x = (int)srcRect.position.x;
    src.y = (int)srcRect.position.y;
    src.w = (int)srcRect.size.x;
    src.h = (int)srcRect.size.y;

    float sw = srcRect.size.x * scale.x * zoom;
    float sh = srcRect.size.y * scale.y * zoom;

    SDL_FRect dst;
    dst.x = screenPos.x - sw * origin.x;
    dst.y = screenPos.y - sh * origin.y;
    dst.w = sw;
    dst.h = sh;

    SDL_FPoint center = {sw * origin.x, sh * origin.y};
    SDL_RendererFlip sdlFlip = SDL_FLIP_NONE;
    if (flip == FlipMode::Horizontal) sdlFlip = SDL_FLIP_HORIZONTAL;
    else if (flip == FlipMode::Vertical) sdlFlip = SDL_FLIP_VERTICAL;
    else if (flip == FlipMode::Both)
        sdlFlip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);

    SDL_SetTextureColorMod(texture->GetSDLTexture(), tint.R8(), tint.G8(), tint.B8());
    SDL_SetTextureAlphaMod(texture->GetSDLTexture(), tint.A8());

    SDL_RenderCopyExF(renderer_, texture->GetSDLTexture(), &src, &dst,
                      rotation * RAD2DEG, &center, sdlFlip);
    drawCalls_++;
}

void Renderer2D::DrawRect(const Rect2& rect, const Color& color, bool filled) {
    Vec2 screenPos = WorldToScreen(rect.position);
    float zoom = camera_ ? camera_->GetZoom() : 1.0f;

    SDL_FRect r;
    r.x = screenPos.x;
    r.y = screenPos.y;
    r.w = rect.size.x * zoom;
    r.h = rect.size.y * zoom;

    SDL_SetRenderDrawColor(renderer_, color.R8(), color.G8(), color.B8(), color.A8());
    if (filled)
        SDL_RenderFillRectF(renderer_, &r);
    else
        SDL_RenderDrawRectF(renderer_, &r);
    drawCalls_++;
}

void Renderer2D::DrawRectEx(const Vec2& pos, const Vec2& size, float rotation,
                             const Color& color, bool filled) {
    // For rotated rects, draw as polygon
    if (Abs(rotation) > EPSILON) {
        Vec2 half = size * 0.5f;
        Vec2 corners[4] = {
            Vec2(-half.x, -half.y).Rotated(rotation) + pos,
            Vec2(half.x, -half.y).Rotated(rotation) + pos,
            Vec2(half.x, half.y).Rotated(rotation) + pos,
            Vec2(-half.x, half.y).Rotated(rotation) + pos
        };
        DrawPolygon(corners, 4, color, filled);
    } else {
        DrawRect(Rect2(pos - size * 0.5f, size), color, filled);
    }
}

void Renderer2D::DrawLine(const Vec2& from, const Vec2& to, const Color& color) {
    Vec2 sf = WorldToScreen(from);
    Vec2 st = WorldToScreen(to);
    SDL_SetRenderDrawColor(renderer_, color.R8(), color.G8(), color.B8(), color.A8());
    SDL_RenderDrawLineF(renderer_, sf.x, sf.y, st.x, st.y);
    drawCalls_++;
}

void Renderer2D::DrawCircle(const Vec2& center, float radius, const Color& color,
                             bool filled, int segments) {
    Vec2 sc = WorldToScreen(center);
    float zoom = camera_ ? camera_->GetZoom() : 1.0f;
    float r = radius * zoom;

    SDL_SetRenderDrawColor(renderer_, color.R8(), color.G8(), color.B8(), color.A8());

    if (filled) {
        for (int y = (int)-r; y <= (int)r; y++) {
            int dx = (int)std::sqrt(r * r - y * y);
            SDL_RenderDrawLineF(renderer_, sc.x - dx, sc.y + y, sc.x + dx, sc.y + y);
        }
    } else {
        float step = TAU / segments;
        for (int i = 0; i < segments; i++) {
            float a1 = i * step, a2 = (i + 1) * step;
            SDL_RenderDrawLineF(renderer_,
                sc.x + std::cos(a1) * r, sc.y + std::sin(a1) * r,
                sc.x + std::cos(a2) * r, sc.y + std::sin(a2) * r);
        }
    }
    drawCalls_++;
}

void Renderer2D::DrawPoint(const Vec2& pos, const Color& color) {
    Vec2 sp = WorldToScreen(pos);
    SDL_SetRenderDrawColor(renderer_, color.R8(), color.G8(), color.B8(), color.A8());
    SDL_RenderDrawPointF(renderer_, sp.x, sp.y);
    drawCalls_++;
}

void Renderer2D::DrawPolygon(const Vec2* points, int count, const Color& color, bool filled) {
    if (count < 3) return;
    SDL_SetRenderDrawColor(renderer_, color.R8(), color.G8(), color.B8(), color.A8());

    if (filled) {
        // Simple triangle fan fill
        Vec2 a = WorldToScreen(points[0]);
        for (int i = 1; i < count - 1; i++) {
            Vec2 b = WorldToScreen(points[i]);
            Vec2 c = WorldToScreen(points[i + 1]);
            // Rasterize triangle with scanlines would be complex,
            // so draw as outline for now
            SDL_RenderDrawLineF(renderer_, a.x, a.y, b.x, b.y);
            SDL_RenderDrawLineF(renderer_, b.x, b.y, c.x, c.y);
            SDL_RenderDrawLineF(renderer_, c.x, c.y, a.x, a.y);
        }
    } else {
        for (int i = 0; i < count; i++) {
            Vec2 a = WorldToScreen(points[i]);
            Vec2 b = WorldToScreen(points[(i + 1) % count]);
            SDL_RenderDrawLineF(renderer_, a.x, a.y, b.x, b.y);
        }
    }
    drawCalls_++;
}

} // namespace Nova
