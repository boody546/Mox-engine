#include "rendering/Texture.h"
#include "core/Logger.h"

// stb_image for loading PNG/JPG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Nova {

Texture::~Texture() { Free(); }

bool Texture::LoadFromFile(SDL_Renderer* renderer, const std::string& path) {
    Free();
    path_ = path;

    // Load with stb_image
    int channels;
    unsigned char* pixels = stbi_load(path.c_str(), &width_, &height_, &channels, 4);
    if (!pixels) {
        NOVA_ERROR("Failed to load texture: ", path, " — ", stbi_failure_reason());
        return false;
    }

    // Create SDL surface from pixel data
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
        pixels, width_, height_, 32, width_ * 4,
        0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
    );

    if (!surface) {
        NOVA_ERROR("Failed to create surface: ", SDL_GetError());
        stbi_image_free(pixels);
        return false;
    }

    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    stbi_image_free(pixels);

    if (!texture_) {
        NOVA_ERROR("Failed to create texture: ", SDL_GetError());
        return false;
    }

    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    NOVA_TRACE("Texture loaded: ", path, " (", width_, "x", height_, ")");
    return true;
}

bool Texture::LoadFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
    Free();
    if (!surface) return false;

    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture_) {
        NOVA_ERROR("Failed to create texture from surface: ", SDL_GetError());
        return false;
    }
    width_ = surface->w;
    height_ = surface->h;
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    return true;
}

bool Texture::CreateSolid(SDL_Renderer* renderer, int w, int h, const Color& color) {
    Free();
    texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_TARGET, w, h);
    if (!texture_) return false;

    SDL_SetRenderTarget(renderer, texture_);
    SDL_SetRenderDrawColor(renderer, color.R8(), color.G8(), color.B8(), color.A8());
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, nullptr);

    width_ = w;
    height_ = h;
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    return true;
}

void Texture::Free() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    width_ = height_ = 0;
}

void Texture::SetBlendMode(SDL_BlendMode mode) {
    if (texture_) SDL_SetTextureBlendMode(texture_, mode);
}

} // namespace Nova
