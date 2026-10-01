#include "Ball.h"
#include <algorithm>
#include <cmath>

namespace Breakout {

Ball::Ball(Vec2 pos, Vec2 vel)
    : m_pos(pos)
    , m_vel(vel)
    , m_stuck(vel.lengthSquared() < 0.001f)
{
}

void Ball::resetOnPaddle(float paddleCenterX, float paddleTopY) {
    m_stuck = true;
    m_alive = true;
    m_paddleOffset = 0.0f;
    m_pos = Vec2{paddleCenterX, paddleTopY - m_radius - 1.0f};
    m_vel = Vec2{0.0f, 0.0f};
    m_currentSpeed = m_baseSpeed;
    m_isFireball = false;
    m_fireballTimer = 0.0f;
    m_trail.clear();
}

void Ball::stickToPaddle(float offsetFromPaddleCenter) {
    m_stuck = true;
    m_paddleOffset = offsetFromPaddleCenter;
    m_vel = Vec2{0.0f, 0.0f};
}

void Ball::launch(float angleRad) {
    if (!m_stuck) return;
    m_stuck = false;

    // Launch upward with angleRad
    m_vel = Vec2{std::cos(angleRad) * m_currentSpeed, std::sin(angleRad) * m_currentSpeed};

    // Guarantee upward motion
    if (m_vel.y > -50.0f) {
        m_vel.y = -m_currentSpeed * 0.7f;
        m_vel = m_vel.normalized() * m_currentSpeed;
    }
}

void Ball::update(float dt) {
    if (!m_alive || m_stuck) return;

    m_animTimer += dt;

    if (m_isFireball) {
        m_fireballTimer -= dt;
        if (m_fireballTimer <= 0.0f) {
            m_isFireball = false;
        }
    }

    // Motion trail record
    m_trail.push_front(m_pos);
    if (m_trail.size() > 5) {
        m_trail.pop_back();
    }

    // Integrate motion with speed scaling
    m_pos += m_vel * (m_speedScale * dt);
}

void Ball::deflectPaddle(float paddleCenterX, float paddleWidth, float paddleVx) {
    // Relative impact ratio from center [-1.0 to 1.0]
    float halfWidth = paddleWidth * 0.5f;
    float offset = std::clamp((m_pos.x - paddleCenterX) / halfWidth, -1.0f, 1.0f);

    // Max reflection angle of 70 degrees from normal
    constexpr float MAX_BOUNCE_ANGLE = 1.2217305f; // ~70 deg
    float bounceAngle = -1.5707963f + offset * MAX_BOUNCE_ANGLE; // -90 deg is straight up

    // Calculate new velocity vector
    float spd = m_currentSpeed;
    m_vel.x = std::cos(bounceAngle) * spd;
    m_vel.y = std::sin(bounceAngle) * spd;

    // Add tangential momentum from paddle movement (spin)
    m_vel.x += paddleVx * 0.2f;

    // Re-normalize to current speed
    m_vel = m_vel.normalized() * spd;

    // Guarantee minimum vertical velocity to avoid horizontal bouncing loops
    if (std::abs(m_vel.y) < 45.0f) {
        m_vel.y = (m_vel.y < 0.0f ? -45.0f : 45.0f);
        m_vel = m_vel.normalized() * spd;
    }

    boostSpeed(1.5f);
}

void Ball::deflectNormal(Vec2 normal) {
    if (m_isFireball) {
        // Fireballs pass through bricks without deflecting
        return;
    }

    m_vel = m_vel.reflected(normal);

    // Re-normalize to current speed
    float spd = m_currentSpeed;
    m_vel = m_vel.normalized() * spd;

    // Avoid horizontal bouncing traps
    if (std::abs(m_vel.y) < 35.0f) {
        m_vel.y = (m_vel.y < 0.0f ? -35.0f : 35.0f);
        m_vel = m_vel.normalized() * spd;
    }
}

void Ball::boostSpeed(float amount) {
    m_currentSpeed = std::min(m_currentSpeed + amount, BALL_MAX_SPEED);
}

void Ball::setFireball(bool active, float duration) {
    m_isFireball = active;
    if (active) {
        m_fireballTimer = duration;
    }
}

void Ball::setSlow(bool active, float factor) {
    m_speedScale = active ? factor : 1.0f;
}

int Ball::checkArenaBoundaries() {
    int hitType = 0;

    // Left wall
    if (m_pos.x - m_radius < PLAYFIELD_LEFT) {
        m_pos.x = PLAYFIELD_LEFT + m_radius;
        if (m_vel.x < 0.0f) m_vel.x = -m_vel.x;
        hitType = 1;
    }
    // Right wall
    if (m_pos.x + m_radius > PLAYFIELD_RIGHT) {
        m_pos.x = PLAYFIELD_RIGHT - m_radius;
        if (m_vel.x > 0.0f) m_vel.x = -m_vel.x;
        hitType = 1;
    }
    // Ceiling
    if (m_pos.y - m_radius < PLAYFIELD_TOP) {
        m_pos.y = PLAYFIELD_TOP + m_radius;
        if (m_vel.y < 0.0f) m_vel.y = -m_vel.y;
        hitType = 2;
    }
    // Bottom pit
    if (m_pos.y - m_radius > PLAYFIELD_BOTTOM) {
        m_alive = false;
        return -1;
    }

    return hitType;
}

void Ball::render(RasterBuffer& buffer) const {
    if (!m_alive) return;

    // Motion trail rendering
    float trailAlpha = 0.5f;
    for (const auto& trailPos : m_trail) {
        uint32_t trailCol = m_isFireball ? Colors::NeonOrange : Colors::NeonCyan;
        if (m_skin == BallSkin::PlasmaCore) trailCol = 0xFFFF00D4;
        else if (m_skin == BallSkin::NeonDiamond) trailCol = 0xFF8CF8F7;
        else if (m_skin == BallSkin::CyberCube) trailCol = 0xFF00E5FF;

        buffer.setPixelBlend(static_cast<int>(trailPos.x), static_cast<int>(trailPos.y), trailCol, trailAlpha);
        trailAlpha *= 0.6f;
    }

    int cx = static_cast<int>(m_pos.x);
    int cy = static_cast<int>(m_pos.y);
    int r = static_cast<int>(m_radius);

    if (m_isFireball) {
        // Blazing Fireball mode override
        buffer.fillCircle(cx, cy, r, Colors::NeonOrange);
        buffer.drawCircle(cx, cy, r, Colors::NeonYellow);
        buffer.setPixel(cx, cy, Colors::White);
        return;
    }

    if (m_skin == BallSkin::PlasmaCore) {
        // Plasma Orb with orbiting energy sparks
        buffer.fillCircle(cx, cy, r, 0xFF7928CA); // Deep neon violet
        buffer.fillCircle(cx, cy, r - 1, 0xFF00F0FF); // Cyan plasma core
        buffer.setPixel(cx, cy, Colors::White);
        // Orbiting sparks
        int s1x = cx + static_cast<int>(std::round(std::cos(m_animTimer * 12.0f) * (r + 1)));
        int s1y = cy + static_cast<int>(std::round(std::sin(m_animTimer * 12.0f) * (r + 1)));
        buffer.setPixel(s1x, s1y, 0xFFFF00D4);
        int s2x = cx - static_cast<int>(std::round(std::cos(m_animTimer * 12.0f) * (r + 1)));
        int s2y = cy - static_cast<int>(std::round(std::sin(m_animTimer * 12.0f) * (r + 1)));
        buffer.setPixel(s2x, s2y, 0xFF00FFCC);
    } else if (m_skin == BallSkin::NeonDiamond) {
        // 4-point rotating star diamond
        buffer.drawFastHLine(cx - r - 1, cx + r + 1, cy, 0xFF8CF8F7);
        buffer.drawFastVLine(cx, cy - r - 1, cy + r + 1, 0xFF8CF8F7);
        buffer.fillRect(cx - 1, cy - 1, 3, 3, 0xFFB3F6C0);
        buffer.setPixel(cx, cy, Colors::White);
    } else if (m_skin == BallSkin::CyberCube) {
        // 3D Isometric micro cube
        buffer.fillRect(cx - 2, cy - 2, 5, 5, 0xFF003366);
        buffer.drawFastHLine(cx - 2, cx + 2, cy - 2, 0xFF00FFFF);
        buffer.drawFastVLine(cx - 2, cy - 2, cy + 2, 0xFF00FFFF);
        buffer.fillRect(cx - 1, cy - 1, 3, 3, 0xFF0077EE);
        buffer.setPixel(cx, cy, Colors::White);
    } else {
        // Classic Arcade Energy Orb
        buffer.fillCircle(cx, cy, r, Colors::White);
        buffer.drawCircle(cx, cy, r, Colors::NeonCyan);
        buffer.setPixel(cx, cy, Colors::White);
    }
}

} // namespace Breakout
