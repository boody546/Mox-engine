#include "rendering/TileMap.h"
#include "core/Logger.h"
#include <algorithm>

namespace Nova {

void TileMap::SetTileset(Texture* tileset, int tw, int th) {
    tileset_ = tileset;
    tileWidth_ = tw;
    tileHeight_ = th;
    if (tileset) tilesetCols_ = tileset->GetWidth() / tw;
}

void TileMap::Resize(int w, int h, int numLayers) {
    width_ = w; height_ = h; numLayers_ = numLayers;
    layers_.resize(numLayers);
    for (auto& layer : layers_) {
        layer.resize(w * h, -1);
    }
}

void TileMap::SetTile(int layer, int x, int y, int tileId) {
    if (layer < 0 || layer >= numLayers_ || x < 0 || x >= width_ || y < 0 || y >= height_) return;
    layers_[layer][y * width_ + x] = tileId;
}

int TileMap::GetTile(int layer, int x, int y) const {
    if (layer < 0 || layer >= numLayers_ || x < 0 || x >= width_ || y < 0 || y >= height_) return -1;
    return layers_[layer][y * width_ + x];
}

void TileMap::Fill(int layer, int tileId) {
    if (layer < 0 || layer >= numLayers_) return;
    std::fill(layers_[layer].begin(), layers_[layer].end(), tileId);
}

Vec2 TileMap::WorldToTile(const Vec2& worldPos) const {
    Vec2 local = worldPos - position_;
    return Vec2(Floor(local.x / tileWidth_), Floor(local.y / tileHeight_));
}

Vec2 TileMap::TileToWorld(int tx, int ty) const {
    return position_ + Vec2((float)(tx * tileWidth_), (float)(ty * tileHeight_));
}

Rect2 TileMap::GetTileSourceRect(int tileId) const {
    if (tileId < 0) return {};
    int col = tileId % tilesetCols_;
    int row = tileId / tilesetCols_;
    return Rect2((float)(col * tileWidth_), (float)(row * tileHeight_),
                 (float)tileWidth_, (float)tileHeight_);
}

void TileMap::Render(Renderer2D* renderer, const Rect2& viewport) {
    if (!tileset_) return;

    // Calculate visible tile range
    Vec2 topLeft = WorldToTile(Vec2(viewport.Left(), viewport.Top()));
    Vec2 bottomRight = WorldToTile(Vec2(viewport.Right(), viewport.Bottom()));

    int startX = std::max(0, (int)topLeft.x - 1);
    int startY = std::max(0, (int)topLeft.y - 1);
    int endX = std::min(width_ - 1, (int)bottomRight.x + 1);
    int endY = std::min(height_ - 1, (int)bottomRight.y + 1);

    for (int layer = 0; layer < numLayers_; layer++) {
        for (int y = startY; y <= endY; y++) {
            for (int x = startX; x <= endX; x++) {
                int tileId = layers_[layer][y * width_ + x];
                if (tileId < 0) continue;

                Rect2 src = GetTileSourceRect(tileId);
                Vec2 worldPos = TileToWorld(x, y);
                renderer->DrawSpriteRegion(tileset_, src, worldPos, 0.0f,
                                            Vec2::One(), Color::White(),
                                            FlipMode::None, Vec2::Zero());
            }
        }
    }
}

bool TileMap::IsSolid(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return true;
    for (int layer = 0; layer < numLayers_; layer++) {
        int tid = layers_[layer][y * width_ + x];
        if (std::find(solidTiles_.begin(), solidTiles_.end(), tid) != solidTiles_.end())
            return true;
    }
    return false;
}

void TileMap::SetSolidTile(int tileId, bool solid) {
    auto it = std::find(solidTiles_.begin(), solidTiles_.end(), tileId);
    if (solid && it == solidTiles_.end()) solidTiles_.push_back(tileId);
    else if (!solid && it != solidTiles_.end()) solidTiles_.erase(it);
}

} // namespace Nova
