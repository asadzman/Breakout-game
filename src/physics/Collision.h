#pragma once

#include "Vec2.h"
#include <algorithm>

namespace Breakout {

struct AABB {
    Vec2 min;
    Vec2 max;

    AABB() = default;
    constexpr AABB(Vec2 pMin, Vec2 pMax) : min(pMin), max(pMax) {}
    constexpr AABB(float x, float y, float w, float h)
        : min(x, y), max(x + w, y + h) {}

    [[nodiscard]] float width() const { return max.x - min.x; }
    [[nodiscard]] float height() const { return max.y - min.y; }
    [[nodiscard]] Vec2 center() const { return (min + max) * 0.5f; }

    [[nodiscard]] bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x &&
                min.y <= other.max.y && max.y >= other.min.y);
    }
};

struct CollisionResult {
    bool collided = false;
    Vec2 normal{0.0f, 0.0f};      // Direction to deflect ball
    float penetration = 0.0f;     // Overlap depth
    Vec2 contactPoint{0.0f, 0.0f};// Exact contact point
};

class Collision {
public:
    // Resolves circle vs AABB (used for Ball vs Brick and Ball vs Paddle)
    static CollisionResult testCircleAABB(Vec2 circleCenter, float radius, const AABB& box);

    // Resolves AABB vs AABB (used for Laser vs Brick and PowerUp vs Paddle)
    static bool testAABBAABB(const AABB& a, const AABB& b);
};

} // namespace Breakout
