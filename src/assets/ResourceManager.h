#pragma once
#include "rendering/Texture.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <SDL.h>

namespace Nova {

class ResourceManager {
public:
    explicit ResourceManager(SDL_Renderer* renderer);
    ~ResourceManager();

    // Textures
    Texture* LoadTexture(const std::string& path);
    Texture* GetTexture(const std::string& path) const;
    void UnloadTexture(const std::string& path);

    // File loading
    std::string LoadTextFile(const std::string& path) const;
    bool FileExists(const std::string& path) const;

    // Cleanup
    void UnloadAll();

    // Stats
    int GetTextureCount() const { return (int)textures_.size(); }

    // Base paths
    void SetAssetBasePath(const std::string& path) { assetBasePath_ = path; }
    std::string ResolvePath(const std::string& relativePath) const;

private:
    SDL_Renderer* renderer_;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures_;
    std::string assetBasePath_ = "assets/";
};

} // namespace Nova
