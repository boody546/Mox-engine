#pragma once
#include "core/Math.h"
#include <SDL.h>
#include <string>

namespace Nova {

class Texture {
public:
    Texture() = default;
    ~Texture();

    // Load from file (PNG, JPG, BMP via stb_image or SDL)
    bool LoadFromFile(SDL_Renderer* renderer, const std::string& path);
    // Create from SDL_Surface
    bool LoadFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
    // Create a solid color texture
    bool CreateSolid(SDL_Renderer* renderer, int w, int h, const Color& color);
    
    void Free();

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    Vec2 GetSize() const { return Vec2((float)width_, (float)height_); }
    SDL_Texture* GetSDLTexture() const { return texture_; }
    const std::string& GetPath() const { return path_; }
    bool IsValid() const { return texture_ != nullptr; }

    // Blend mode
    void SetBlendMode(SDL_BlendMode mode);

private:
    SDL_Texture* texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::string path_;
};

} // namespace Nova
