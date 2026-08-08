#pragma once
// ═══════════════════════════════════════════════════════════════════
//  Nova2D Math Library — Optimized for 2D
// ═══════════════════════════════════════════════════════════════════

#include <cmath>
#include <algorithm>
#include <random>
#include <cstdint>

namespace Nova {

// ─── Constants ───────────────────────────────────────────────────
constexpr float PI        = 3.14159265358979323846f;
constexpr float TAU       = PI * 2.0f;
constexpr float DEG2RAD   = PI / 180.0f;
constexpr float RAD2DEG   = 180.0f / PI;
constexpr float EPSILON   = 0.00001f;

// ─── Utility Functions ──────────────────────────────────────────
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float Clamp(float val, float min, float max) { return std::fmin(std::fmax(val, min), max); }
inline float Clamp01(float val) { return Clamp(val, 0.0f, 1.0f); }
inline float Sign(float val) { return val < 0 ? -1.0f : (val > 0 ? 1.0f : 0.0f); }
inline float Abs(float val) { return std::fabs(val); }
inline float Sqrt(float val) { return std::sqrtf(val); }
inline float Floor(float val) { return std::floorf(val); }
inline float Ceil(float val) { return std::ceilf(val); }
inline float Round(float val) { return std::roundf(val); }
inline float Min(float a, float b) { return std::fmin(a, b); }
inline float Max(float a, float b) { return std::fmax(a, b); }
inline float SmoothStep(float edge0, float edge1, float x) {
    float t = Clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}
inline float InverseLerp(float a, float b, float value) {
    return (b - a) != 0.0f ? (value - a) / (b - a) : 0.0f;
}
inline float MoveTowards(float current, float target, float maxDelta) {
    if (Abs(target - current) <= maxDelta) return target;
    return current + Sign(target - current) * maxDelta;
}
inline float Wrap(float value, float min, float max) {
    float range = max - min;
    return min + std::fmod(std::fmod(value - min, range) + range, range);
}

// ─── Random ─────────────────────────────────────────────────────
class Random {
public:
    static void Seed(uint32_t seed) { engine_.seed(seed); }
    
    // Random float [0, 1)
    static float Value() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(engine_);
    }
    
    // Random float [min, max)
    static float Range(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(engine_);
    }
    
    // Random int [min, max]
    static int RangeInt(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(engine_);
    }

private:
    static inline std::mt19937 engine_{std::random_device{}()};
};

// ═══════════════════════════════════════════════════════════════════
//  Vec2 — 2D Vector
// ═══════════════════════════════════════════════════════════════════
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x, float y) : x(x), y(y) {}
    constexpr explicit Vec2(float v) : x(v), y(v) {}

    // ─── Arithmetic Operators ────────────────────────────────
    constexpr Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    constexpr Vec2 operator*(const Vec2& o) const { return {x * o.x, y * o.y}; }
    constexpr Vec2 operator/(const Vec2& o) const { return {x / o.x, y / o.y}; }
    constexpr Vec2 operator*(float s) const { return {x * s, y * s}; }
    constexpr Vec2 operator/(float s) const { return {x / s, y / s}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { x /= s; y /= s; return *this; }

    constexpr bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    constexpr bool operator!=(const Vec2& o) const { return !(*this == o); }

    // ─── Vector Operations ───────────────────────────────────
    float Length() const { return Sqrt(x * x + y * y); }
    float LengthSquared() const { return x * x + y * y; }
    
    Vec2 Normalized() const {
        float len = Length();
        return len > EPSILON ? Vec2(x / len, y / len) : Vec2(0, 0);
    }
    
    void Normalize() {
        float len = Length();
        if (len > EPSILON) { x /= len; y /= len; }
    }

    float Dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float Cross(const Vec2& o) const { return x * o.y - y * o.x; }
    
    float DistanceTo(const Vec2& o) const { return (*this - o).Length(); }
    float DistanceSquaredTo(const Vec2& o) const { return (*this - o).LengthSquared(); }
    
    float Angle() const { return std::atan2(y, x); }
    float AngleTo(const Vec2& o) const { return std::atan2(Cross(o), Dot(o)); }
    
    Vec2 Rotated(float angle) const {
        float c = std::cos(angle), s = std::sin(angle);
        return {x * c - y * s, x * s + y * c};
    }
    
    Vec2 Perpendicular() const { return {-y, x}; }
    
    Vec2 Reflect(const Vec2& normal) const {
        return *this - normal * (2.0f * Dot(normal));
    }
    
    Vec2 LerpTo(const Vec2& to, float t) const {
        return {Nova::Lerp(x, to.x, t), Nova::Lerp(y, to.y, t)};
    }
    
    Vec2 MoveToward(const Vec2& to, float delta) const {
        Vec2 diff = to - *this;
        float dist = diff.Length();
        if (dist <= delta || dist < EPSILON) return to;
        return *this + diff / dist * delta;
    }
    
    Vec2 Clamped(float maxLength) const {
        float len = Length();
        if (len > maxLength && len > EPSILON) {
            return *this * (maxLength / len);
        }
        return *this;
    }
    
    Vec2 Abs() const { return {std::fabs(x), std::fabs(y)}; }
    Vec2 Floor() const { return {std::floor(x), std::floor(y)}; }
    Vec2 Ceil() const { return {std::ceil(x), std::ceil(y)}; }
    Vec2 Round() const { return {std::round(x), std::round(y)}; }
    Vec2 Sign() const { return {Nova::Sign(x), Nova::Sign(y)}; }

    bool IsZero() const { return LengthSquared() < EPSILON * EPSILON; }

    // ─── Static Constructors ────────────────────────────────
    static constexpr Vec2 Zero()  { return {0, 0}; }
    static constexpr Vec2 One()   { return {1, 1}; }
    static constexpr Vec2 Up()    { return {0, -1}; }
    static constexpr Vec2 Down()  { return {0, 1}; }
    static constexpr Vec2 Left()  { return {-1, 0}; }
    static constexpr Vec2 Right() { return {1, 0}; }
    
    static Vec2 FromAngle(float angle) {
        return {std::cos(angle), std::sin(angle)};
    }
    
    static Vec2 RandomUnit() {
        float angle = Random::Range(0.0f, TAU);
        return FromAngle(angle);
    }
    
    static Vec2 RandomInCircle(float radius = 1.0f) {
        return RandomUnit() * Sqrt(Random::Value()) * radius;
    }
};

inline Vec2 operator*(float s, const Vec2& v) { return {s * v.x, s * v.y}; }

// ═══════════════════════════════════════════════════════════════════
//  Color — RGBA Color
// ═══════════════════════════════════════════════════════════════════
struct Color {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;

    constexpr Color() = default;
    constexpr Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    // Convert 0-255 to 0-1 range
    static constexpr Color FromRGBA8(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }

    // Hex color: 0xRRGGBBAA
    static constexpr Color FromHex(uint32_t hex) {
        return {
            ((hex >> 24) & 0xFF) / 255.0f,
            ((hex >> 16) & 0xFF) / 255.0f,
            ((hex >> 8) & 0xFF) / 255.0f,
            (hex & 0xFF) / 255.0f
        };
    }

    uint8_t R8() const { return static_cast<uint8_t>(Clamp01(r) * 255); }
    uint8_t G8() const { return static_cast<uint8_t>(Clamp01(g) * 255); }
    uint8_t B8() const { return static_cast<uint8_t>(Clamp01(b) * 255); }
    uint8_t A8() const { return static_cast<uint8_t>(Clamp01(a) * 255); }

    Color WithAlpha(float alpha) const { return {r, g, b, alpha}; }
    
    Color LerpTo(const Color& to, float t) const {
        return {Lerp(r, to.r, t), Lerp(g, to.g, t), Lerp(b, to.b, t), Lerp(a, to.a, t)};
    }

    Color operator*(float s) const { return {r * s, g * s, b * s, a}; }

    // ─── Preset Colors ──────────────────────────────────────
    static constexpr Color White()       { return {1, 1, 1, 1}; }
    static constexpr Color Black()       { return {0, 0, 0, 1}; }
    static constexpr Color Red()         { return {1, 0, 0, 1}; }
    static constexpr Color Green()       { return {0, 1, 0, 1}; }
    static constexpr Color Blue()        { return {0, 0, 1, 1}; }
    static constexpr Color Yellow()      { return {1, 1, 0, 1}; }
    static constexpr Color Cyan()        { return {0, 1, 1, 1}; }
    static constexpr Color Magenta()     { return {1, 0, 1, 1}; }
    static constexpr Color Orange()      { return {1, 0.647f, 0, 1}; }
    static constexpr Color Purple()      { return {0.5f, 0, 0.5f, 1}; }
    static constexpr Color Transparent() { return {0, 0, 0, 0}; }
    static constexpr Color CornflowerBlue() { return {0.392f, 0.584f, 0.929f, 1}; }
};

// ═══════════════════════════════════════════════════════════════════
//  Rect2 — 2D Rectangle (position + size)
// ═══════════════════════════════════════════════════════════════════
struct Rect2 {
    Vec2 position;   // Top-left corner
    Vec2 size;       // Width, Height

    constexpr Rect2() = default;
    constexpr Rect2(float x, float y, float w, float h) : position(x, y), size(w, h) {}
    constexpr Rect2(Vec2 pos, Vec2 sz) : position(pos), size(sz) {}

    float Left()   const { return position.x; }
    float Right()  const { return position.x + size.x; }
    float Top()    const { return position.y; }
    float Bottom() const { return position.y + size.y; }
    Vec2  Center() const { return position + size * 0.5f; }
    float Area()   const { return size.x * size.y; }

    bool Contains(const Vec2& point) const {
        return point.x >= Left() && point.x <= Right() &&
               point.y >= Top()  && point.y <= Bottom();
    }

    bool Intersects(const Rect2& other) const {
        return Left() < other.Right() && Right() > other.Left() &&
               Top() < other.Bottom() && Bottom() > other.Top();
    }

    Rect2 Intersection(const Rect2& other) const {
        float l = Max(Left(), other.Left());
        float t = Max(Top(), other.Top());
        float r = Min(Right(), other.Right());
        float b = Min(Bottom(), other.Bottom());
        if (r <= l || b <= t) return {};
        return {l, t, r - l, b - t};
    }

    Rect2 Merged(const Rect2& other) const {
        float l = Min(Left(), other.Left());
        float t = Min(Top(), other.Top());
        float r = Max(Right(), other.Right());
        float b = Max(Bottom(), other.Bottom());
        return {l, t, r - l, b - t};
    }

    Rect2 Expanded(float amount) const {
        return {position.x - amount, position.y - amount,
                size.x + amount * 2, size.y + amount * 2};
    }

    Rect2 Moved(const Vec2& offset) const {
        return {position + offset, size};
    }
};

// ═══════════════════════════════════════════════════════════════════
//  Transform2D — 2D Transform (position, rotation, scale)
// ═══════════════════════════════════════════════════════════════════
struct Transform2D {
    Vec2  position{0, 0};
    float rotation = 0.0f;    // Radians
    Vec2  scale{1, 1};
    Vec2  origin{0, 0};       // Pivot point (0-1 range)

    Transform2D() = default;
    Transform2D(Vec2 pos) : position(pos) {}
    Transform2D(Vec2 pos, float rot, Vec2 scl = Vec2::One()) 
        : position(pos), rotation(rot), scale(scl) {}

    // Transform a local point to world space
    Vec2 TransformPoint(const Vec2& point) const {
        Vec2 scaled = (point - origin) * scale;
        Vec2 rotated = scaled.Rotated(rotation);
        return rotated + position;
    }
    
    // Inverse transform: world to local
    Vec2 InverseTransformPoint(const Vec2& point) const {
        Vec2 local = point - position;
        Vec2 unrotated = local.Rotated(-rotation);
        return unrotated / scale + origin;
    }

    Vec2 Forward() const { return Vec2::FromAngle(rotation); }
    Vec2 Right() const { return Vec2::FromAngle(rotation + PI * 0.5f); }
};

} // namespace Nova
