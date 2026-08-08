#pragma once
#include "core/Math.h"
#include "rendering/Texture.h"
#include "rendering/Renderer2D.h"
#include <vector>
#include <string>

namespace Nova {

class TileMap {
public:
    TileMap() = default;
    ~TileMap() = default;

    // Setup
    void SetTileset(Texture* tileset, int tileWidth, int tileHeight);
    void Resize(int width, int height, int layers = 1);

    // Tile manipulation
    void SetTile(int layer, int x, int y, int tileId);
    int  GetTile(int layer, int x, int y) const;
    void ClearTile(int layer, int x, int y) { SetTile(layer, x, y, -1); }
    void Fill(int layer, int tileId);
    void ClearLayer(int layer) { Fill(layer, -1); }

    // Info
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetTileWidth() const { return tileWidth_; }
    int GetTileHeight() const { return tileHeight_; }
    int GetLayerCount() const { return numLayers_; }
    Vec2 GetMapPixelSize() const { return Vec2((float)(width_ * tileWidth_), (float)(height_ * tileHeight_)); }

    // Convert between world and tile coordinates
    Vec2 WorldToTile(const Vec2& worldPos) const;
    Vec2 TileToWorld(int tileX, int tileY) const;

    // Render (only visible tiles)
    void Render(Renderer2D* renderer, const Rect2& viewport);

    // Collision helpers
    bool IsSolid(int x, int y) const;
    void SetSolidTile(int tileId, bool solid = true);

    // Position offset
    void SetPosition(const Vec2& pos) { position_ = pos; }
    Vec2 GetPosition() const { return position_; }

private:
    Texture* tileset_ = nullptr;
    int tileWidth_ = 16, tileHeight_ = 16;
    int tilesetCols_ = 1;
    int width_ = 0, height_ = 0;
    int numLayers_ = 1;
    Vec2 position_;

    // Tile data: layers[layer][y * width + x] = tileId (-1 = empty)
    std::vector<std::vector<int>> layers_;
    std::vector<int> solidTiles_;

    Rect2 GetTileSourceRect(int tileId) const;
};

} // namespace Nova
