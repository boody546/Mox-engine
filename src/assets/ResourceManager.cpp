#include "assets/ResourceManager.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Nova {

ResourceManager::ResourceManager(SDL_Renderer* renderer) : renderer_(renderer) {
    // Try to detect base path
    char* basePath = SDL_GetBasePath();
    if (basePath) {
        assetBasePath_ = std::string(basePath) + "assets/";
        SDL_free(basePath);
    }
}

ResourceManager::~ResourceManager() { UnloadAll(); }

Texture* ResourceManager::LoadTexture(const std::string& path) {
    std::string fullPath = ResolvePath(path);

    // Check cache
    auto it = textures_.find(fullPath);
    if (it != textures_.end()) return it->second.get();

    // Load new texture
    auto tex = std::make_unique<Texture>();
    if (!tex->LoadFromFile(renderer_, fullPath)) {
        NOVA_ERROR("Failed to load texture: ", fullPath);
        return nullptr;
    }

    Texture* ptr = tex.get();
    textures_[fullPath] = std::move(tex);
    NOVA_TRACE("Texture cached: ", fullPath);
    return ptr;
}

Texture* ResourceManager::GetTexture(const std::string& path) const {
    std::string fullPath = ResolvePath(path);
    auto it = textures_.find(fullPath);
    return it != textures_.end() ? it->second.get() : nullptr;
}

void ResourceManager::UnloadTexture(const std::string& path) {
    std::string fullPath = ResolvePath(path);
    textures_.erase(fullPath);
}

std::string ResourceManager::LoadTextFile(const std::string& path) const {
    std::string fullPath = ResolvePath(path);
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        NOVA_ERROR("Cannot open file: ", fullPath);
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool ResourceManager::FileExists(const std::string& path) const {
    std::string fullPath = ResolvePath(path);
    return std::filesystem::exists(fullPath);
}

void ResourceManager::UnloadAll() {
    textures_.clear();
    NOVA_LOG("All resources unloaded");
}

std::string ResourceManager::ResolvePath(const std::string& relativePath) const {
    // If absolute path, return as-is
    if (relativePath.size() > 1 && (relativePath[0] == '/' || relativePath[1] == ':'))
        return relativePath;
    return assetBasePath_ + relativePath;
}

} // namespace Nova
