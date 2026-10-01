#include "Collision.h"
#include <cmath>

namespace Breakout {

CollisionResult Collision::testCircleAABB(Vec2 circleCenter, float radius, const AABB& box) {
    CollisionResult result;

    // Find closest point on AABB to circle center
    float closestX = std::clamp(circleCenter.x, box.min.x, box.max.x);
    float closestY = std::clamp(circleCenter.y, box.min.y, box.max.y);

    Vec2 diff = circleCenter - Vec2{closestX, closestY};
    float distSq = diff.lengthSquared();

    if (distSq <= radius * radius) {
        result.collided = true;
        result.contactPoint = Vec2{closestX, closestY};
        float dist = std::sqrt(distSq);

        if (dist > 0.0001f) {
            result.normal = diff / dist;
            result.penetration = radius - dist;
        } else {
            // Circle center inside or exactly on box edge: resolve along smallest penetration axis
            float leftPen = circleCenter.x - box.min.x;
            float rightPen = box.max.x - circleCenter.x;
            float topPen = circleCenter.y - box.min.y;
            float bottomPen = box.max.y - circleCenter.y;

            float minPen = leftPen;
            result.normal = Vec2{-1.0f, 0.0f};

            if (rightPen < minPen) {
                minPen = rightPen;
                result.normal = Vec2{1.0f, 0.0f};
            }
            if (topPen < minPen) {
                minPen = topPen;
                result.normal = Vec2{0.0f, -1.0f};
            }
            if (bottomPen < minPen) {
                minPen = bottomPen;
                result.normal = Vec2{0.0f, 1.0f};
            }
            result.penetration = radius + minPen;
        }
    }

    return result;
}

bool Collision::testAABBAABB(const AABB& a, const AABB& b) {
    return a.intersects(b);
}

} // namespace Breakout
