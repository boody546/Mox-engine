#pragma once
// ═══════════════════════════════════════════════════════════════════
//  Mox Engine — Node2D.h
//  Base 2D spatial node + all specialized 2D runtime node types
//  loaded from scenes/main.json by SceneLoader.
// ═══════════════════════════════════════════════════════════════════

#include "scene/Node.h"
#include "scene/ScriptNode.h"
#include "rendering/ParticleSystem.h"
#include "physics/PhysicsWorld.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace Nova {

class Engine;
class InputManager;

// ─────────────────────────────────────────────────────────────────
// Node2D  — Base 2D Spatial Node
// ─────────────────────────────────────────────────────────────────
class Node2D : public Node {
public:
    Node2D(const std::string& name = "Node2D") : Node(name) {}
    ~Node2D() override = default;
    std::string GetType() const override { return "Node2D"; }
};

// ─────────────────────────────────────────────────────────────────
// Sprite2D  — Textured 2D sprite
// ─────────────────────────────────────────────────────────────────
class Sprite2D : public Node2D {
public:
    Sprite2D(const std::string& name = "Sprite2D") : Node2D(name) {}
    ~Sprite2D() override = default;

    void SetTexturePath(const std::string& path) { texturePath_ = path; }
    const std::string& GetTexturePath() const { return texturePath_; }

    void SetModulate(const Color& col) { modulate_ = col; }
    const Color& GetModulate() const { return modulate_; }

    void SetFlipH(bool flip) { flipH_ = flip; }
    bool GetFlipH() const { return flipH_; }

    void SetFlipV(bool flip) { flipV_ = flip; }
    bool GetFlipV() const { return flipV_; }

    void SetSize(const Vec2& sz) { size_ = sz; }
    Vec2 GetSize() const { return size_; }

    void _Draw(Renderer2D* renderer) override {
        if (!visible_) return;
        Vec2 pos = GetGlobalPosition();
        // If we have a texture loaded, renderer draws it; otherwise draw a tinted rect
        renderer->DrawRect(Rect2(pos - size_ * 0.5f, size_), modulate_, true);
    }

    std::string GetType() const override { return "Sprite2D"; }

private:
    std::string texturePath_;
    Color modulate_ = Color::White();
    bool flipH_ = false;
    bool flipV_ = false;
    Vec2 size_{64.0f, 64.0f};
};

// ─────────────────────────────────────────────────────────────────
// Light2D  — 2D Point / radial light
// ─────────────────────────────────────────────────────────────────
class Light2D : public Node2D {
public:
    Light2D(const std::string& name = "Light2D") : Node2D(name) {}

    void SetLightColor(const Color& col) { color_ = col; }
    const Color& GetLightColor() const { return color_; }

    void SetRadius(float r) { radius_ = r; }
    float GetRadius() const { return radius_; }

    void SetEnergy(float e) { energy_ = e; }
    float GetEnergy() const { return energy_; }

    void _Draw(Renderer2D* renderer) override {
        if (!visible_) return;
        Vec2 pos = GetGlobalPosition();
        // Render several concentric circles to fake a light bloom
        for (int i = 4; i >= 1; --i) {
            float r = radius_ * ((float)i / 4.0f);
            float alpha = 0.06f * energy_ * ((float)(5 - i) / 4.0f);
            renderer->DrawCircle(pos, r, color_.WithAlpha(alpha), true);
        }
        // Bright core
        renderer->DrawCircle(pos, radius_ * 0.08f, color_.WithAlpha(0.5f * energy_), true);
    }

    std::string GetType() const override { return "Light2D"; }

private:
    Color color_ = Color::FromRGBA8(255, 255, 170);
    float radius_ = 140.0f;
    float energy_ = 1.0f;
};

// ─────────────────────────────────────────────────────────────────
// Camera2DNode  — 2D camera: zoom + position, synced to Engine camera
// ─────────────────────────────────────────────────────────────────
class Camera2DNode : public Node2D {
public:
    Camera2DNode(const std::string& name = "Camera2D") : Node2D(name) {}

    void SetZoom(float z) { zoom_ = z; }
    float GetZoom() const { return zoom_; }

    std::string GetType() const override { return "Camera2D"; }

private:
    float zoom_ = 1.0f;
};

// ─────────────────────────────────────────────────────────────────
// CharacterBody2DNode — Kinematic character with gravity + WASD input
// ─────────────────────────────────────────────────────────────────
class CharacterBody2DNode : public ScriptNode {
public:
    float speed       = 200.0f;
    float jumpImpulse = -400.0f;
    float gravity     = 980.0f;

    CharacterBody2DNode(const std::string& name = "CharacterBody2D")
        : ScriptNode(name) {}
    ~CharacterBody2DNode() override {
        // body is owned by PhysicsWorld — do NOT delete here
    }

    void SetPhysicsBody(RigidBody2D* body, PhysicsWorld* world) {
        body_ = body;
        world_ = world;
    }

    void _Update(float dt) override {
        // Let script run its _process hook first
        ScriptNode::_Update(dt);

        // Apply gravity
        velocity_.y += gravity * dt;

        // Read WASD / Arrow input from Engine singleton
        if (auto* eng = Engine::Instance()) {
            auto* input = eng->GetInput();
            float dx = 0.0f;
            if (input->IsKeyDown(SDL_SCANCODE_A) || input->IsKeyDown(SDL_SCANCODE_LEFT))  dx -= 1.0f;
            if (input->IsKeyDown(SDL_SCANCODE_D) || input->IsKeyDown(SDL_SCANCODE_RIGHT)) dx += 1.0f;
            velocity_.x = dx * speed;

            // Jump (only on ground — simplified: on floor when velocity.y is near 0)
            bool onFloor = onFloor_;
            if (onFloor && (input->IsKeyDown(SDL_SCANCODE_W)    ||
                            input->IsKeyDown(SDL_SCANCODE_UP)    ||
                            input->IsKeyDown(SDL_SCANCODE_SPACE))) {
                velocity_.y = jumpImpulse;
                onFloor_ = false;
            }
        }

        // Move position
        Vec2 pos = GetPosition() + velocity_ * dt;

        // Simple floor clamp (y > 700 → floor at y=700, adjust as needed)
        if (pos.y > 680.0f) {
            pos.y = 680.0f;
            velocity_.y = 0.0f;
            onFloor_ = true;
        }

        SetPosition(pos);

        // Sync physics body position
        if (body_) body_->position = pos;
    }

    void _Draw(Renderer2D* renderer) override {
        if (!visible_) return;
        Vec2 pos = GetGlobalPosition();
        // Draw character as a teal rectangle
        renderer->DrawRect(Rect2(pos - Vec2(20, 30), Vec2(40, 60)),
                           Color::FromRGBA8(100, 220, 255), true);
        // Wireframe collision box (green)
        renderer->DrawRect(Rect2(pos - Vec2(20, 30), Vec2(40, 60)),
                           Color::FromRGBA8(46, 204, 113, 160), false);
    }

    std::string GetType() const override { return "CharacterBody2D"; }

private:
    RigidBody2D* body_  = nullptr;
    PhysicsWorld* world_ = nullptr;
    Vec2 velocity_{0.0f, 0.0f};
    bool onFloor_ = false;
};

// ─────────────────────────────────────────────────────────────────
// RigidBody2DNode — Dynamic physics body, syncs from PhysicsWorld
// ─────────────────────────────────────────────────────────────────
class RigidBody2DNode : public ScriptNode {
public:
    RigidBody2DNode(const std::string& name = "RigidBody2D")
        : ScriptNode(name) {}
    ~RigidBody2DNode() override {}

    void SetPhysicsBody(RigidBody2D* body, PhysicsWorld* world) {
        body_ = body;
        world_ = world;
    }

    void _Update(float dt) override {
        ScriptNode::_Update(dt);
        // Pull position from the physics simulation
        if (body_) SetPosition(body_->position);
    }

    void _Draw(Renderer2D* renderer) override {
        if (!visible_) return;
        Vec2 pos = GetGlobalPosition();
        Vec2 sz{40.0f, 40.0f};
        if (body_ && body_->collider) {
            Rect2 bounds = body_->collider->GetBounds(Vec2::Zero());
            sz = bounds.size;
        }
        renderer->DrawRect(Rect2(pos - sz * 0.5f, sz),
                           Color::FromRGBA8(76, 127, 190), true);
        renderer->DrawRect(Rect2(pos - sz * 0.5f, sz),
                           Color::FromRGBA8(46, 204, 113, 160), false);
    }

    std::string GetType() const override { return "RigidBody2D"; }

private:
    RigidBody2D* body_   = nullptr;
    PhysicsWorld* world_ = nullptr;
};

// ─────────────────────────────────────────────────────────────────
// StaticBody2DNode — Static environment collider (walls, floors)
// ─────────────────────────────────────────────────────────────────
class StaticBody2DNode : public Node2D {
public:
    StaticBody2DNode(const std::string& name = "StaticBody2D")
        : Node2D(name) {}
    ~StaticBody2DNode() override {}

    void SetPhysicsBody(RigidBody2D* body, PhysicsWorld* world) {
        body_ = body;
        world_ = world;
    }

    void _Draw(Renderer2D* renderer) override {
        if (!visible_) return;
        Vec2 pos = GetGlobalPosition();
        Vec2 sz{100.0f, 20.0f};
        if (body_ && body_->collider) {
            Rect2 bounds = body_->collider->GetBounds(Vec2::Zero());
            sz = bounds.size;
        }
        // Dark grey filled rect with a green wireframe border
        renderer->DrawRect(Rect2(pos - sz * 0.5f, sz),
                           Color::FromRGBA8(76, 76, 76), true);
        renderer->DrawRect(Rect2(pos - sz * 0.5f, sz),
                           Color::FromRGBA8(46, 204, 113, 200), false);
    }

    std::string GetType() const override { return "StaticBody2D"; }

private:
    RigidBody2D* body_   = nullptr;
    PhysicsWorld* world_ = nullptr;
};

// ─────────────────────────────────────────────────────────────────
// TileMapNode — Grid-based tilemap renderer
// ─────────────────────────────────────────────────────────────────
class TileMapNode : public Node2D {
public:
    TileMapNode(const std::string& name = "TileMap") : Node2D(name) {}

    void SetCellSize(float size) { cellSize_ = size; }
    float GetCellSize() const { return cellSize_; }

    void SetTile(int col, int row, const std::string& tileId) {
        tiles_[{col, row}] = tileId;
    }
    void ClearTiles() { tiles_.clear(); }

    void _Draw(Renderer2D* renderer) override {
        if (!visible_) return;
        Vec2 origin = GetGlobalPosition();

        for (auto& [coord, tileId] : tiles_) {
            if (tileId.empty()) continue;
            float x = origin.x + coord.first  * cellSize_;
            float y = origin.y + coord.second * cellSize_;
            Color col = _tileColor(tileId);
            renderer->DrawRect(Rect2(x, y, cellSize_ - 1, cellSize_ - 1), col, true);
        }
    }

    std::string GetType() const override { return "TileMap"; }

private:
    float cellSize_ = 32.0f;

    struct PairHash {
        size_t operator()(const std::pair<int,int>& p) const {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 16);
        }
    };
    std::unordered_map<std::pair<int,int>, std::string, PairHash> tiles_;

    static Color _tileColor(const std::string& id) {
        if (id == "grass")  return Color::FromRGBA8(76,  175, 80);
        if (id == "dirt")   return Color::FromRGBA8(121, 85,  72);
        if (id == "stone")  return Color::FromRGBA8(96,  125, 139);
        if (id == "water")  return Color::FromRGBA8(33,  150, 243);
        if (id == "brick")  return Color::FromRGBA8(233, 30,  99);
        if (id == "wood")   return Color::FromRGBA8(141, 110, 99);
        return Color::FromRGBA8(150, 150, 150);
    }
};

// ─────────────────────────────────────────────────────────────────
// CPUParticles2DNode — Owns and drives a ParticleSystem
// ─────────────────────────────────────────────────────────────────
class CPUParticles2DNode : public Node2D {
public:
    CPUParticles2DNode(const std::string& name = "CPUParticles2D")
        : Node2D(name) {}

    void SetParticleConfig(const ParticleConfig& cfg) {
        particles_.SetConfig(cfg);
    }
    void StartEmitting() { particles_.Start(); }
    void StopEmitting()  { particles_.Stop();  }

    void _Update(float dt) override {
        particles_.SetPosition(GetGlobalPosition());
        particles_.Update(dt);
    }

    void _Draw(Renderer2D* renderer) override {
        if (!visible_) return;
        particles_.Render(renderer);
    }

    std::string GetType() const override { return "CPUParticles2D"; }

private:
    ParticleSystem particles_;
};

// ─────────────────────────────────────────────────────────────────
// AudioNode — Audio stream player (logs path; no SDL_mixer dependency)
// ─────────────────────────────────────────────────────────────────
class AudioNode : public Node2D {
public:
    AudioNode(const std::string& name = "AudioNode") : Node2D(name) {}

    void SetStreamPath(const std::string& path) { streamPath_ = path; }
    const std::string& GetStreamPath() const { return streamPath_; }

    void SetVolumeDb(float db) { volumeDb_ = db; }
    float GetVolumeDb() const { return volumeDb_; }

    void _Ready() override {
        if (!streamPath_.empty())
            NOVA_LOG("AudioNode '", GetName(), "': stream=", streamPath_,
                     " vol=", volumeDb_, "dB (SDL_mixer not linked)");
    }

    std::string GetType() const override { return "AudioNode"; }

private:
    std::string streamPath_;
    float volumeDb_ = 0.0f;
};

} // namespace Nova
