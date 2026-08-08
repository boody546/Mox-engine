#include "physics/PhysicsWorld.h"
#include "rendering/Renderer2D.h"
#include "core/Logger.h"
#include <algorithm>
#include <cmath>

namespace Nova {

PhysicsWorld::PhysicsWorld() {}
PhysicsWorld::~PhysicsWorld() {
    for (auto* b : bodies_) delete b;
    bodies_.clear();
}

RigidBody2D* PhysicsWorld::CreateBody() {
    auto* body = new RigidBody2D();
    body->id = nextId_++;
    bodies_.push_back(body);
    return body;
}

void PhysicsWorld::DestroyBody(RigidBody2D* body) {
    auto it = std::find(bodies_.begin(), bodies_.end(), body);
    if (it != bodies_.end()) {
        bodies_.erase(it);
        delete body;
    }
}

void PhysicsWorld::Step(float dt) {
    IntegrateBodies(dt);
    DetectCollisions();
}

void PhysicsWorld::IntegrateBodies(float dt) {
    for (auto* body : bodies_) {
        if (body->type == BodyType::Static) continue;
        if (body->type == BodyType::Dynamic) {
            // Apply gravity
            body->acceleration = body->force * body->inverseMass + gravity_ * body->gravityScale;
            body->velocity += body->acceleration * dt;
            body->velocity *= (1.0f - body->linearDamping);
            body->position += body->velocity * dt;
            body->force = Vec2::Zero();
        } else if (body->type == BodyType::Kinematic) {
            body->position += body->velocity * dt;
        }
    }
}

void PhysicsWorld::DetectCollisions() {
    for (size_t i = 0; i < bodies_.size(); i++) {
        for (size_t j = i + 1; j < bodies_.size(); j++) {
            auto* a = bodies_[i];
            auto* b = bodies_[j];
            if (!a->collider || !b->collider) continue;
            if (a->type == BodyType::Static && b->type == BodyType::Static) continue;

            // Check layer masks
            if (!(a->collider->layer & b->collider->mask) &&
                !(b->collider->layer & a->collider->mask)) continue;

            CollisionInfo info;
            auto aType = a->collider->type;
            auto bType = b->collider->type;

            if (aType == ColliderType::AABB && bType == ColliderType::AABB) {
                info = CheckAABBvsAABB(a->collider->GetBounds(a->position),
                                        b->collider->GetBounds(b->position));
            } else if (aType == ColliderType::Circle && bType == ColliderType::Circle) {
                auto* ca = static_cast<CircleCollider*>(a->collider);
                auto* cb = static_cast<CircleCollider*>(b->collider);
                info = CheckCirclevsCircle(a->position + ca->offset, ca->radius,
                                            b->position + cb->offset, cb->radius);
            } else if (aType == ColliderType::AABB && bType == ColliderType::Circle) {
                auto* cb = static_cast<CircleCollider*>(b->collider);
                info = CheckAABBvsCircle(a->collider->GetBounds(a->position),
                                          b->position + cb->offset, cb->radius);
            } else if (aType == ColliderType::Circle && bType == ColliderType::AABB) {
                auto* ca = static_cast<CircleCollider*>(a->collider);
                info = CheckAABBvsCircle(b->collider->GetBounds(b->position),
                                          a->position + ca->offset, ca->radius);
                info.normal = -info.normal;
            }

            if (info.collided) {
                if (collisionCallback_) collisionCallback_(a, b, info);
                if (!a->collider->isTrigger && !b->collider->isTrigger) {
                    ResolveCollision(a, b, info);
                }
            }
        }
    }
}

void PhysicsWorld::ResolveCollision(RigidBody2D* a, RigidBody2D* b, const CollisionInfo& info) {
    // Positional correction
    float totalInvMass = a->inverseMass + b->inverseMass;
    if (totalInvMass <= 0) return;

    Vec2 correction = info.normal * (info.depth / totalInvMass) * 0.8f;
    if (a->type != BodyType::Static) a->position -= correction * a->inverseMass;
    if (b->type != BodyType::Static) b->position += correction * b->inverseMass;

    // Velocity resolution
    Vec2 relVel = b->velocity - a->velocity;
    float normalVel = relVel.Dot(info.normal);
    if (normalVel > 0) return; // Separating

    float restitution = Min(a->bounce, b->bounce);
    float j = -(1.0f + restitution) * normalVel / totalInvMass;

    Vec2 impulse = info.normal * j;
    if (a->type != BodyType::Static) a->velocity -= impulse * a->inverseMass;
    if (b->type != BodyType::Static) b->velocity += impulse * b->inverseMass;

    // Friction
    Vec2 tangent = relVel - info.normal * normalVel;
    if (tangent.LengthSquared() > EPSILON) {
        tangent.Normalize();
        float jt = -relVel.Dot(tangent) / totalInvMass;
        float mu = (a->friction + b->friction) * 0.5f;
        Vec2 frictionImpulse = Abs(jt) < j * mu ? tangent * jt : tangent * (-j * mu);
        if (a->type != BodyType::Static) a->velocity -= frictionImpulse * a->inverseMass;
        if (b->type != BodyType::Static) b->velocity += frictionImpulse * b->inverseMass;
    }
}

CollisionInfo PhysicsWorld::CheckAABBvsAABB(const Rect2& a, const Rect2& b) {
    CollisionInfo info;
    if (!a.Intersects(b)) return info;

    float overlapX = Min(a.Right() - b.Left(), b.Right() - a.Left());
    float overlapY = Min(a.Bottom() - b.Top(), b.Bottom() - a.Top());

    info.collided = true;
    if (overlapX < overlapY) {
        info.depth = overlapX;
        info.normal = a.Center().x < b.Center().x ? Vec2(-1, 0) : Vec2(1, 0);
    } else {
        info.depth = overlapY;
        info.normal = a.Center().y < b.Center().y ? Vec2(0, -1) : Vec2(0, 1);
    }
    info.contactPoint = (a.Center() + b.Center()) * 0.5f;
    return info;
}

CollisionInfo PhysicsWorld::CheckCirclevsCircle(const Vec2& posA, float rA,
                                                  const Vec2& posB, float rB) {
    CollisionInfo info;
    Vec2 diff = posB - posA;
    float dist = diff.Length();
    float totalR = rA + rB;
    if (dist >= totalR) return info;

    info.collided = true;
    info.depth = totalR - dist;
    info.normal = dist > EPSILON ? diff / dist : Vec2(1, 0);
    info.contactPoint = posA + info.normal * rA;
    return info;
}

CollisionInfo PhysicsWorld::CheckAABBvsCircle(const Rect2& aabb, const Vec2& circPos, float circR) {
    CollisionInfo info;
    Vec2 closest;
    closest.x = Clamp(circPos.x, aabb.Left(), aabb.Right());
    closest.y = Clamp(circPos.y, aabb.Top(), aabb.Bottom());

    Vec2 diff = circPos - closest;
    float distSq = diff.LengthSquared();
    if (distSq >= circR * circR) return info;

    float dist = Sqrt(distSq);
    info.collided = true;
    info.depth = circR - dist;
    info.normal = dist > EPSILON ? diff / dist : Vec2(0, -1);
    info.contactPoint = closest;
    return info;
}

RaycastHit PhysicsWorld::Raycast(const Vec2& origin, const Vec2& dir, float maxDist,
                                  uint32_t mask) const {
    RaycastHit closest;
    closest.distance = maxDist;
    Vec2 normDir = dir.Normalized();

    for (auto* body : bodies_) {
        if (!body->collider || !(body->collider->layer & mask)) continue;

        // Simple AABB ray intersection
        Rect2 bounds = body->collider->GetBounds(body->position);
        float tmin = 0, tmax = maxDist;

        if (Abs(normDir.x) > EPSILON) {
            float t1 = (bounds.Left() - origin.x) / normDir.x;
            float t2 = (bounds.Right() - origin.x) / normDir.x;
            if (t1 > t2) std::swap(t1, t2);
            tmin = Max(tmin, t1);
            tmax = Min(tmax, t2);
        } else if (origin.x < bounds.Left() || origin.x > bounds.Right()) continue;

        if (Abs(normDir.y) > EPSILON) {
            float t1 = (bounds.Top() - origin.y) / normDir.y;
            float t2 = (bounds.Bottom() - origin.y) / normDir.y;
            if (t1 > t2) std::swap(t1, t2);
            tmin = Max(tmin, t1);
            tmax = Min(tmax, t2);
        } else if (origin.y < bounds.Top() || origin.y > bounds.Bottom()) continue;

        if (tmin <= tmax && tmin < closest.distance && tmin >= 0) {
            closest.hit = true;
            closest.distance = tmin;
            closest.point = origin + normDir * tmin;
            closest.body = body;
            // Approximate normal
            Vec2 center = bounds.Center();
            Vec2 diff = closest.point - center;
            if (Abs(diff.x) > Abs(diff.y))
                closest.normal = Vec2(Sign(diff.x), 0);
            else
                closest.normal = Vec2(0, Sign(diff.y));
        }
    }
    return closest;
}

std::vector<RigidBody2D*> PhysicsWorld::QueryRect(const Rect2& rect, uint32_t mask) const {
    std::vector<RigidBody2D*> result;
    for (auto* body : bodies_) {
        if (!body->collider || !(body->collider->layer & mask)) continue;
        if (rect.Intersects(body->collider->GetBounds(body->position)))
            result.push_back(body);
    }
    return result;
}

std::vector<RigidBody2D*> PhysicsWorld::QueryCircle(const Vec2& center, float radius,
                                                      uint32_t mask) const {
    std::vector<RigidBody2D*> result;
    for (auto* body : bodies_) {
        if (!body->collider || !(body->collider->layer & mask)) continue;
        Rect2 bounds = body->collider->GetBounds(body->position);
        Vec2 closest;
        closest.x = Clamp(center.x, bounds.Left(), bounds.Right());
        closest.y = Clamp(center.y, bounds.Top(), bounds.Bottom());
        if (center.DistanceSquaredTo(closest) <= radius * radius)
            result.push_back(body);
    }
    return result;
}

void PhysicsWorld::DebugDraw(Renderer2D* renderer) const {
    for (auto* body : bodies_) {
        if (!body->collider) continue;
        Color c = body->type == BodyType::Static ? Color::Green() :
                  body->type == BodyType::Kinematic ? Color::Cyan() : Color::Yellow();
        c = c.WithAlpha(0.5f);

        if (body->collider->type == ColliderType::AABB) {
            renderer->DrawRect(body->collider->GetBounds(body->position), c, false);
        } else if (body->collider->type == ColliderType::Circle) {
            auto* cc = static_cast<CircleCollider*>(body->collider);
            renderer->DrawCircle(body->position + cc->offset, cc->radius, c, false);
        }
    }
}

} // namespace Nova
