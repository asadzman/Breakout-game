#pragma once

#include "core/Config.h"
#include "physics/Vec2.h"
#include "physics/Collision.h"
#include "raster/RasterBuffer.h"
#include "entities/Laser.h"
#include <vector>

namespace Breakout {

class Paddle {
public:
    Paddle();

    void reset();
    void update(float dt);
    void render(RasterBuffer& buffer) const;

    // Movement inputs
    void setMoveLeft(bool active) {
        m_moveLeft = active;
        if (active) m_mouseControlActive = false;
    }
    void setMoveRight(bool active) {
        m_moveRight = active;
        if (active) m_mouseControlActive = false;
    }
    void setTargetX(float x); // For mouse tracking
    void triggerHitFlash() { m_flashTimer = 0.08f; }

    // Power-up state management
    void applyPowerUp(PowerUpType type, float duration = POWERUP_DURATION_SEC);
    void updatePowerUps(float dt);

    // Laser firing (returns spawned lasers if ready)
    std::vector<Laser> tryFireLasers();

    [[nodiscard]] Vec2 getPosition() const { return m_pos; }
    void setPosition(const Vec2& pos) { m_pos = pos; }
    [[nodiscard]] float getWidth() const { return m_width; }
    [[nodiscard]] float getHeight() const { return m_height; }
    [[nodiscard]] float getVelocityX() const { return m_vx; }
    [[nodiscard]] bool isSticky() const { return m_isSticky; }
    [[nodiscard]] bool hasLaser() const { return m_hasLaser; }
    [[nodiscard]] AABB getBounds() const;

    void setSkin(PaddleSkin skin) { m_skin = skin; }
    [[nodiscard]] PaddleSkin getSkin() const { return m_skin; }

    // Active power-up remaining time queries for HUD
    [[nodiscard]] float getLaserTimeRemaining() const { return m_laserTimer; }
    [[nodiscard]] float getWidthTimeRemaining() const { return m_widthTimer; }
    [[nodiscard]] float getStickyTimeRemaining() const { return m_stickyTimer; }

private:
    Vec2 m_pos;
    float m_width = DEFAULT_PADDLE_WIDTH;
    float m_targetWidth = DEFAULT_PADDLE_WIDTH;
    float m_height = PADDLE_HEIGHT;
    float m_vx = 0.0f;

    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_mouseControlActive = false;
    float m_mouseTargetX = 0.0f;

    // Power-up buff timers
    bool m_hasLaser = false;
    float m_laserTimer = 0.0f;
    float m_laserFireCooldown = 0.0f;

    bool m_isSticky = false;
    float m_stickyTimer = 0.0f;

    float m_widthTimer = 0.0f;
    float m_flashTimer = 0.0f;
    PaddleSkin m_skin = PaddleSkin::Skateboard;
};

} // namespace Breakout
