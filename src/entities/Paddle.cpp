#include "Paddle.h"
#include <algorithm>
#include <cmath>

namespace Breakout {

Paddle::Paddle() {
    reset();
}

void Paddle::reset() {
    m_width = DEFAULT_PADDLE_WIDTH;
    m_targetWidth = DEFAULT_PADDLE_WIDTH;
    m_height = PADDLE_HEIGHT;
    m_pos = Vec2{(VIRTUAL_WIDTH - m_width) * 0.5f, PADDLE_Y};
    m_vx = 0.0f;
    m_moveLeft = false;
    m_moveRight = false;
    m_mouseControlActive = false;

    m_hasLaser = false;
    m_laserTimer = 0.0f;
    m_laserFireCooldown = 0.0f;

    m_isSticky = false;
    m_stickyTimer = 0.0f;

    m_widthTimer = 0.0f;
}

void Paddle::setTargetX(float x) {
    m_mouseControlActive = true;
    m_mouseTargetX = x;
}

void Paddle::applyPowerUp(PowerUpType type, float duration) {
    switch (type) {
        case PowerUpType::Elongate:
            m_targetWidth = ELONGATED_PADDLE_WIDTH;
            m_widthTimer = duration;
            break;
        case PowerUpType::Shrink:
            m_targetWidth = SHRUNK_PADDLE_WIDTH;
            m_widthTimer = duration;
            break;
        case PowerUpType::Laser:
            m_hasLaser = true;
            m_laserTimer = duration;
            break;
        case PowerUpType::StickyCatch:
            m_isSticky = true;
            m_stickyTimer = duration;
            break;
        default:
            break;
    }
}

void Paddle::updatePowerUps(float dt) {
    // Width buff decay
    if (m_widthTimer > 0.0f) {
        m_widthTimer -= dt;
        if (m_widthTimer <= 0.0f) {
            m_targetWidth = DEFAULT_PADDLE_WIDTH;
        }
    }

    // Laser buff decay
    if (m_laserTimer > 0.0f) {
        m_laserTimer -= dt;
        if (m_laserTimer <= 0.0f) {
            m_hasLaser = false;
        }
    }
    if (m_laserFireCooldown > 0.0f) {
        m_laserFireCooldown -= dt;
    }

    // Sticky buff decay
    if (m_stickyTimer > 0.0f) {
        m_stickyTimer -= dt;
        if (m_stickyTimer <= 0.0f) {
            m_isSticky = false;
        }
    }

    // Smoothly animate paddle width change
    if (std::abs(m_width - m_targetWidth) > 0.5f) {
        float oldWidth = m_width;
        m_width += (m_targetWidth - m_width) * (dt * 10.0f);
        // Keep paddle center fixed while width changes
        m_pos.x -= (m_width - oldWidth) * 0.5f;
    } else {
        m_width = m_targetWidth;
    }
}

void Paddle::update(float dt) {
    updatePowerUps(dt);

    float prevX = m_pos.x;

    if (m_mouseControlActive) {
        // Directly align paddle center to mouse cursor with 1:1 responsive tracking
        m_pos.x = m_mouseTargetX - m_width * 0.5f;
    } else {
        // Keyboard arrow/AD inputs
        float moveDir = 0.0f;
        if (m_moveLeft) moveDir -= 1.0f;
        if (m_moveRight) moveDir += 1.0f;

        m_pos.x += moveDir * PADDLE_SPEED * dt;
    }

    // Clamp paddle to arena boundaries
    float minX = static_cast<float>(PLAYFIELD_LEFT);
    float maxX = static_cast<float>(PLAYFIELD_RIGHT) - m_width;
    m_pos.x = std::clamp(m_pos.x, minX, maxX);

    // Calculate actual horizontal velocity to transfer spin to ball
    if (dt > 0.0001f) {
        m_vx = (m_pos.x - prevX) / dt;
    } else {
        m_vx = 0.0f;
    }
}

std::vector<Laser> Paddle::tryFireLasers() {
    std::vector<Laser> fired;
    if (!m_hasLaser || m_laserFireCooldown > 0.0f) return fired;

    m_laserFireCooldown = 0.22f; // Cooldown between bursts

    // Twin laser bolts launched from left and right wings
    fired.emplace_back(Vec2{m_pos.x + 2.0f, m_pos.y - LASER_HEIGHT});
    fired.emplace_back(Vec2{m_pos.x + m_width - 4.0f, m_pos.y - LASER_HEIGHT});
    return fired;
}

void Paddle::render(RasterBuffer& buffer) const {
    int px = static_cast<int>(m_pos.x);
    int py = static_cast<int>(m_pos.y);
    int pw = static_cast<int>(m_width);
    int ph = static_cast<int>(m_height);

    // Paddle body colors
    uint32_t bodyColor = m_isSticky ? Colors::NeonPurple : Colors::NeonCyan;
    uint32_t coreColor = Colors::White;
    uint32_t bevelColor = Colors::BorderGlow;

    // Base body
    buffer.fillRect(px + 2, py, pw - 4, ph, bodyColor);
    buffer.fillRect(px, py + 1, pw, ph - 2, bodyColor);

    // Center metallic grip
    int gripWidth = std::min(pw - 12, 16);
    int gripX = px + (pw - gripWidth) / 2;
    buffer.fillRect(gripX, py + 2, gripWidth, ph - 4, Colors::GrayDark);
    for (int gx = gripX + 2; gx < gripX + gripWidth - 1; gx += 3) {
        buffer.drawFastVLine(gx, py + 2, py + ph - 3, Colors::White);
    }

    // Top highlight bevel
    buffer.drawFastHLine(px + 2, px + pw - 3, py, coreColor);
    buffer.drawFastHLine(px + 1, px + pw - 2, py + 1, bevelColor);

    // Bottom dark shadow
    buffer.drawFastHLine(px + 2, px + pw - 3, py + ph - 1, Colors::Black);

    // Laser cannon attachments on wings if laser powerup is active
    if (m_hasLaser) {
        // Left cannon barrel
        buffer.fillRect(px - 1, py - 3, 3, 5, Colors::NeonYellow);
        buffer.drawFastHLine(px - 1, px + 1, py - 3, Colors::LaserRed);

        // Right cannon barrel
        buffer.fillRect(px + pw - 2, py - 3, 3, 5, Colors::NeonYellow);
        buffer.drawFastHLine(px + pw - 2, px + pw, py - 3, Colors::LaserRed);
    }
}

AABB Paddle::getBounds() const {
    return AABB(m_pos.x, m_pos.y, m_width, m_height);
}

} // namespace Breakout
