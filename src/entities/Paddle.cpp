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
    m_flashTimer = 0.0f;
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
    if (m_flashTimer > 0.0f) {
        m_flashTimer = std::max(0.0f, m_flashTimer - dt);
    }

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

    if (m_skin == PaddleSkin::Skateboard) {
        // --- AUTHENTIC STREET SKATEBOARD ---
        // 1. Dual Urethane Wheels & Trucks underneath
        int wheelY = py + ph - 1;
        uint32_t wheelColor = 0xFF00FFCC; // Vivid lime-cyan urethane
        uint32_t truckColor = 0xFF606874; // Steel axle trucks
        // Left wheel set
        buffer.fillRect(px + 4, wheelY, 4, 3, wheelColor);
        buffer.setPixel(px + 5, wheelY + 1, truckColor);
        // Right wheel set
        buffer.fillRect(px + pw - 8, wheelY, 4, 3, wheelColor);
        buffer.setPixel(px + pw - 7, wheelY + 1, truckColor);

        // 2. Skateboard Deck Body
        // Kicktails (raised lips on left and right ends)
        buffer.fillRect(px, py - 1, 2, ph - 1, 0xFF39FF14); // Neon green deck rail
        buffer.fillRect(px + pw - 2, py - 1, 2, ph - 1, 0xFF39FF14);

        // Deck base board (dark grip tape surface)
        buffer.fillRect(px + 2, py, pw - 4, ph - 1, 0xFF181A20);
        buffer.fillRect(px + 1, py + 1, pw - 2, ph - 2, 0xFF181A20);

        // Dual Racing Deck Stripes running down the middle
        int stripeY = py + 2;
        buffer.drawFastHLine(px + 4, px + pw - 5, stripeY, 0xFF00E5FF);     // Neon cyan stripe
        buffer.drawFastHLine(px + 4, px + pw - 5, stripeY + 1, 0xFFFF007F); // Hot pink stripe

        // Grip tape speckled traction texture
        for (int gx = px + 6; gx < px + pw - 6; gx += 4) {
            buffer.setPixel(gx, py + 1, 0xFF454D5A);
        }

        // Bottom deck glow rail
        buffer.drawFastHLine(px + 2, px + pw - 3, py + ph - 2, 0xFF39FF14);
    } else if (m_skin == PaddleSkin::RetroWood) {
        // --- RETRO MAHOGANY WOODEN SKATEBOARD ---
        int wheelY = py + ph - 1;
        buffer.fillRect(px + 4, wheelY, 4, 3, 0xFFF0E6D2); // Cream vintage urethane wheels
        buffer.setPixel(px + 5, wheelY + 1, 0xFF5C4033);
        buffer.fillRect(px + pw - 8, wheelY, 4, 3, 0xFFF0E6D2);
        buffer.setPixel(px + pw - 7, wheelY + 1, 0xFF5C4033);

        // Wood kicktails
        buffer.fillRect(px, py - 1, 2, ph - 1, 0xFFD2691E);
        buffer.fillRect(px + pw - 2, py - 1, 2, ph - 1, 0xFFD2691E);

        // Rich mahogany deck
        buffer.fillRect(px + 2, py, pw - 4, ph - 1, 0xFF8B4513);
        buffer.fillRect(px + 1, py + 1, pw - 2, ph - 2, 0xFFA0522D);

        // Maple wood stringer stripe
        buffer.drawFastHLine(px + 3, px + pw - 4, py + 2, 0xFFDEB887);
        buffer.drawFastHLine(px + 3, px + pw - 4, py + 3, 0xFFF5DEB3);
    } else if (m_skin == PaddleSkin::CyberHover) {
        // --- CYBER HOVERCRAFT ---
        buffer.fillRect(px + 4, py, pw - 8, ph, 0xFF1A2332);
        buffer.fillRect(px + 2, py + 1, pw - 4, ph - 2, 0xFF0D1B2A);

        // Side thruster pods
        buffer.fillRect(px, py + 2, 3, ph - 3, 0xFF00E5FF);
        buffer.fillRect(px + pw - 3, py + 2, 3, ph - 3, 0xFF00E5FF);
        buffer.setPixel(px - 1, py + 3, 0xFF70D6FF);
        buffer.setPixel(px + pw, py + 3, 0xFF70D6FF);

        // Glowing cyan energy line & central plasma reactor
        buffer.drawFastHLine(px + 5, px + pw - 6, py + 2, 0xFF00F0FF);
        int coreX = px + pw / 2;
        buffer.fillRect(coreX - 2, py + 1, 4, 3, Colors::White);
    } else {
        // --- CLASSIC ARCADE ---
        uint32_t bodyColor = m_isSticky ? Colors::NeonPurple : Colors::NeonCyan;
        buffer.fillRect(px + 2, py, pw - 4, ph, bodyColor);
        buffer.fillRect(px, py + 1, pw, ph - 2, bodyColor);

        int gripWidth = std::min(pw - 12, 16);
        int gripX = px + (pw - gripWidth) / 2;
        buffer.fillRect(gripX, py + 2, gripWidth, ph - 4, Colors::GrayDark);
        for (int gx = gripX + 2; gx < gripX + gripWidth - 1; gx += 3) {
            buffer.drawFastVLine(gx, py + 2, py + ph - 3, Colors::White);
        }
        buffer.drawFastHLine(px + 2, px + pw - 3, py, Colors::White);
        buffer.drawFastHLine(px + 1, px + pw - 2, py + 1, Colors::BorderGlow);
        buffer.drawFastHLine(px + 2, px + pw - 3, py + ph - 1, Colors::Black);
    }

    // Sticky catch aura overlay if active
    if (m_isSticky) {
        buffer.drawFastHLine(px, px + pw - 1, py - 1, Colors::NeonPurple);
        for (int sx = px + 2; sx < px + pw - 2; sx += 4) {
            buffer.setPixel(sx, py - 2, Colors::White);
        }
    }

    // Laser cannon attachments on wings if laser powerup is active
    if (m_hasLaser) {
        buffer.fillRect(px - 1, py - 3, 3, 5, Colors::NeonYellow);
        buffer.drawFastHLine(px - 1, px + 1, py - 3, Colors::LaserRed);
        buffer.fillRect(px + pw - 2, py - 3, 3, 5, Colors::NeonYellow);
        buffer.drawFastHLine(px + pw - 2, px + pw, py - 3, Colors::LaserRed);
    }

    // Impact flash feedback
    if (m_flashTimer > 0.0f) {
        buffer.drawFastHLine(px, px + pw - 1, py, Colors::White);
    }
}

AABB Paddle::getBounds() const {
    return AABB(m_pos.x, m_pos.y, m_width, m_height);
}

} // namespace Breakout
