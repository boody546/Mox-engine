#pragma once
#include "core/Math.h"
#include <vector>

namespace Nova {

class Renderer2D;
class Texture;

struct Particle {
    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    Color startColor, endColor;
    float startSize, endSize;
    float lifetime;
    float maxLifetime;
    float rotation;
    float rotationSpeed;
    bool alive = false;
};

struct ParticleConfig {
    Vec2 emitPosition = Vec2::Zero();
    Vec2 emitDirection = Vec2::Up();
    float emitAngle = 45.0f;         // Spread angle in degrees
    float emitSpeed = 100.0f;
    float emitSpeedVariation = 20.0f;
    float lifetime = 1.0f;
    float lifetimeVariation = 0.3f;
    float startSize = 8.0f;
    float endSize = 0.0f;
    Color startColor = Color::White();
    Color endColor = Color::Transparent();
    Vec2 gravity = Vec2(0, 98.0f);
    float emitRate = 50.0f;           // Particles per second
    int maxParticles = 500;
    bool oneShot = false;
    float rotationSpeed = 0.0f;
    float damping = 0.98f;
    Texture* texture = nullptr;
};

class ParticleSystem {
public:
    ParticleSystem();
    explicit ParticleSystem(const ParticleConfig& config);
    ~ParticleSystem() = default;

    void SetConfig(const ParticleConfig& config);
    ParticleConfig& GetConfig() { return config_; }

    void Start();
    void Stop();
    void Reset();
    void Update(float dt);
    void Render(Renderer2D* renderer);

    bool IsEmitting() const { return emitting_; }
    int GetAliveCount() const;
    void SetPosition(const Vec2& pos) { config_.emitPosition = pos; }

private:
    void EmitParticle();

    ParticleConfig config_;
    std::vector<Particle> particles_;
    float emitAccum_ = 0.0f;
    bool emitting_ = false;
};

} // namespace Nova
