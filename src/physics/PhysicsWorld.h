#pragma once
#include "core/Math.h"
#include <vector>
#include <functional>

namespace Nova {

// Collision shapes
enum class ColliderType { AABB, Circle, Polygon };

struct CollisionInfo {
    bool collided = false;
    Vec2 normal;
    float depth = 0.0f;
    Vec2 contactPoint;
};

class Collider {
public:
    ColliderType type;
    Vec2 offset;            // Offset from body position
    uint32_t layer = 1;     // Collision layer bitmask
    uint32_t mask = 0xFFFFFFFF;  // Which layers this collides with
    bool isTrigger = false;      // Triggers don't resolve collision

    virtual ~Collider() = default;
    virtual Rect2 GetBounds(const Vec2& bodyPos) const = 0;
};

class AABBCollider : public Collider {
public:
    Vec2 size;
    AABBCollider(const Vec2& sz) : size(sz) { type = ColliderType::AABB; }
    Rect2 GetBounds(const Vec2& bodyPos) const override {
        Vec2 pos = bodyPos + offset - size * 0.5f;
        return Rect2(pos, size);
    }
};

class CircleCollider : public Collider {
public:
    float radius;
    CircleCollider(float r) : radius(r) { type = ColliderType::Circle; }
    Rect2 GetBounds(const Vec2& bodyPos) const override {
        Vec2 pos = bodyPos + offset;
        return Rect2(pos.x - radius, pos.y - radius, radius * 2, radius * 2);
    }
};

// Rigid body types
enum class BodyType { Static, Dynamic, Kinematic };

struct RigidBody2D {
    int id = -1;
    BodyType type = BodyType::Dynamic;
    Vec2 position;
    Vec2 velocity;
    Vec2 acceleration;
    Vec2 force;
    float mass = 1.0f;
    float inverseMass = 1.0f;
    float bounce = 0.3f;
    float friction = 0.5f;
    float linearDamping = 0.01f;
    float gravityScale = 1.0f;
    bool fixedRotation = true;
    Collider* collider = nullptr;
    void* userData = nullptr;   // Pointer to owning Node

    void SetMass(float m) {
        mass = m;
        inverseMass = m > 0 ? 1.0f / m : 0.0f;
    }
    void ApplyForce(const Vec2& f) { force += f; }
    void ApplyImpulse(const Vec2& imp) { velocity += imp * inverseMass; }
};

// Raycast result
struct RaycastHit {
    bool hit = false;
    Vec2 point;
    Vec2 normal;
    float distance = 0.0f;
    RigidBody2D* body = nullptr;
};

// Collision callback
using CollisionCallback = std::function<void(RigidBody2D* a, RigidBody2D* b, const CollisionInfo& info)>;

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    // Bodies
    RigidBody2D* CreateBody();
    void DestroyBody(RigidBody2D* body);

    // Simulation
    void Step(float dt);
    void SetGravity(const Vec2& g) { gravity_ = g; }
    Vec2 GetGravity() const { return gravity_; }

    // Queries
    RaycastHit Raycast(const Vec2& origin, const Vec2& direction, float maxDist,
                       uint32_t mask = 0xFFFFFFFF) const;
    std::vector<RigidBody2D*> QueryRect(const Rect2& rect, uint32_t mask = 0xFFFFFFFF) const;
    std::vector<RigidBody2D*> QueryCircle(const Vec2& center, float radius,
                                           uint32_t mask = 0xFFFFFFFF) const;

    // Callbacks
    void SetCollisionCallback(CollisionCallback cb) { collisionCallback_ = std::move(cb); }

    // Debug
    void DebugDraw(class Renderer2D* renderer) const;

private:
    void IntegrateBodies(float dt);
    void DetectCollisions();
    void ResolveCollision(RigidBody2D* a, RigidBody2D* b, const CollisionInfo& info);

    // Collision detection
    static CollisionInfo CheckAABBvsAABB(const Rect2& a, const Rect2& b);
    static CollisionInfo CheckCirclevsCircle(const Vec2& posA, float rA, const Vec2& posB, float rB);
    static CollisionInfo CheckAABBvsCircle(const Rect2& aabb, const Vec2& circPos, float circR);

    Vec2 gravity_{0, 980.0f};
    std::vector<RigidBody2D*> bodies_;
    int nextId_ = 0;
    CollisionCallback collisionCallback_;
};

} // namespace Nova
