#include "Paddle.h"
#include <algorithm>

namespace Breakout {

Paddle::Paddle() {
    reset();
}

void Paddle::reset() {
    m_width = DEFAULT_PADDLE_WIDTH;
    m_height = PADDLE_HEIGHT;
    m_pos = Vec2{(VIRTUAL_WIDTH - m_width) * 0.5f, PADDLE_Y};
    m_vx = 0.0f;
    m_moveLeft = false;
    m_moveRight = false;
    m_mouseControlActive = false;
}

void Paddle::setTargetX(float x) {
    m_mouseControlActive = true;
    m_mouseTargetX = x;
}

void Paddle::update(float dt) {
    float prevX = m_pos.x;

    if (m_mouseControlActive) {
        // Directly align paddle center to mouse cursor
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

    // Calculate horizontal velocity to transfer spin to ball
    if (dt > 0.0001f) {
        m_vx = (m_pos.x - prevX) / dt;
    } else {
        m_vx = 0.0f;
    }
}

void Paddle::render(RasterBuffer& buffer) const {
    int px = static_cast<int>(m_pos.x);
    int py = static_cast<int>(m_pos.y);
    int pw = static_cast<int>(m_width);
    int ph = static_cast<int>(m_height);

    // Classic arcade beveled paddle
    uint32_t bodyColor = Colors::NeonCyan;
    buffer.fillRect(px + 2, py, pw - 4, ph, bodyColor);
    buffer.fillRect(px, py + 1, pw, ph - 2, bodyColor);

    // Central grip texture
    int gripWidth = std::min(pw - 12, 16);
    int gripX = px + (pw - gripWidth) / 2;
    buffer.fillRect(gripX, py + 2, gripWidth, ph - 4, Colors::GrayDark);
    for (int gx = gripX + 2; gx < gripX + gripWidth - 1; gx += 3) {
        buffer.drawFastVLine(gx, py + 2, py + ph - 3, Colors::White);
    }
    // Top highlight & bottom shadow
    buffer.drawFastHLine(px + 2, px + pw - 3, py, Colors::White);
    buffer.drawFastHLine(px + 1, px + pw - 2, py + 1, Colors::BorderGlow);
    buffer.drawFastHLine(px + 2, px + pw - 3, py + ph - 1, Colors::Black);
}

AABB Paddle::getBounds() const {
    return AABB(m_pos.x, m_pos.y, m_width, m_height);
}

} // namespace Breakout
