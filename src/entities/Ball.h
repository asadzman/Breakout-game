#pragma once

#include "core/Config.h"
#include "physics/Vec2.h"
#include "physics/Collision.h"
#include "raster/RasterBuffer.h"
#include <deque>

namespace Breakout {

class Ball {
public:
    Ball(Vec2 pos = {0.0f, 0.0f}, Vec2 vel = {0.0f, 0.0f});

    void resetOnPaddle(float paddleCenterX, float paddleTopY);
    void launch(float angleRad = -1.5707963f); // Default straight up (-90 deg)

    void update(float dt);
    void render(RasterBuffer& buffer) const;

    // Deflection mechanics
    void deflectPaddle(float paddleCenterX, float paddleWidth, float paddleVx);
    void deflectNormal(Vec2 normal);
    void boostSpeed(float amount = BALL_SPEED_INCREMENT);

    [[nodiscard]] bool isStuck() const { return m_stuck; }
    [[nodiscard]] bool isAlive() const { return m_alive; }
    [[nodiscard]] Vec2 getPosition() const { return m_pos; }
    [[nodiscard]] Vec2 getVelocity() const { return m_vel; }
    [[nodiscard]] float getRadius() const { return m_radius; }

    void setPosition(Vec2 p) { m_pos = p; }
    void setVelocity(Vec2 v) { m_vel = v; }
    void kill() { m_alive = false; }

    // Check boundary collisions against arena borders
    // Returns 1 if hit left/right, 2 if hit ceiling, -1 if fell in bottom pit
    int checkArenaBoundaries();

private:
    Vec2 m_pos;
    Vec2 m_vel;
    float m_radius = BALL_RADIUS;
    float m_baseSpeed = BALL_BASE_SPEED;
    float m_currentSpeed = BALL_BASE_SPEED;

    bool m_stuck = true;
    bool m_alive = true;

    std::deque<Vec2> m_trail; // Motion trail history
};

} // namespace Breakout
