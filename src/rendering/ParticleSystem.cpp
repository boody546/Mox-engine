#include "rendering/ParticleSystem.h"
#include "rendering/Renderer2D.h"
#include "rendering/Texture.h"

namespace Nova {

ParticleSystem::ParticleSystem() { particles_.resize(500); }
ParticleSystem::ParticleSystem(const ParticleConfig& cfg) : config_(cfg) {
    particles_.resize(cfg.maxParticles);
}

void ParticleSystem::SetConfig(const ParticleConfig& cfg) {
    config_ = cfg;
    particles_.resize(cfg.maxParticles);
}

void ParticleSystem::Start() { emitting_ = true; emitAccum_ = 0; }
void ParticleSystem::Stop() { emitting_ = false; }
void ParticleSystem::Reset() { 
    for (auto& p : particles_) p.alive = false;
    emitAccum_ = 0; 
}

int ParticleSystem::GetAliveCount() const {
    int c = 0;
    for (auto& p : particles_) if (p.alive) c++;
    return c;
}

void ParticleSystem::EmitParticle() {
    for (auto& p : particles_) {
        if (p.alive) continue;

        p.alive = true;
        p.position = config_.emitPosition;

        float angle = config_.emitDirection.Angle();
        float spread = config_.emitAngle * DEG2RAD;
        float a = angle + Random::Range(-spread * 0.5f, spread * 0.5f);
        float speed = config_.emitSpeed + Random::Range(-config_.emitSpeedVariation, config_.emitSpeedVariation);

        p.velocity = Vec2::FromAngle(a) * speed;
        p.acceleration = config_.gravity;
        p.startColor = config_.startColor;
        p.endColor = config_.endColor;
        p.startSize = config_.startSize;
        p.endSize = config_.endSize;
        p.maxLifetime = config_.lifetime + Random::Range(-config_.lifetimeVariation, config_.lifetimeVariation);
        p.maxLifetime = Max(p.maxLifetime, 0.05f);
        p.lifetime = p.maxLifetime;
        p.rotation = Random::Range(0.0f, TAU);
        p.rotationSpeed = config_.rotationSpeed * Random::Range(-1.0f, 1.0f);
        return;
    }
}

void ParticleSystem::Update(float dt) {
    // Emit new particles
    if (emitting_) {
        emitAccum_ += config_.emitRate * dt;
        while (emitAccum_ >= 1.0f) {
            EmitParticle();
            emitAccum_ -= 1.0f;
        }
        if (config_.oneShot) emitting_ = false;
    }

    // Update existing particles
    for (auto& p : particles_) {
        if (!p.alive) continue;
        p.lifetime -= dt;
        if (p.lifetime <= 0) { p.alive = false; continue; }

        p.velocity += p.acceleration * dt;
        p.velocity *= config_.damping;
        p.position += p.velocity * dt;
        p.rotation += p.rotationSpeed * dt;
    }
}

void ParticleSystem::Render(Renderer2D* renderer) {
    for (auto& p : particles_) {
        if (!p.alive) continue;
        float t = 1.0f - (p.lifetime / p.maxLifetime);
        Color color = p.startColor.LerpTo(p.endColor, t);
        float size = Lerp(p.startSize, p.endSize, t);

        if (config_.texture) {
            renderer->DrawSprite(config_.texture, p.position, p.rotation,
                                  Vec2(size / config_.texture->GetWidth()),
                                  color);
        } else {
            renderer->DrawRect(Rect2(p.position - Vec2(size * 0.5f), Vec2(size)),
                               color, true);
        }
    }
}

} // namespace Nova
