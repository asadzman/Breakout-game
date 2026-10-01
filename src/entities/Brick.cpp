#include "Brick.h"
#include <algorithm>

namespace Breakout {

Brick::Brick(float x, float y, float w, float h, BrickType type, int hp, uint32_t baseColor)
    : m_x(x)
    , m_y(y)
    , m_w(w)
    , m_h(h)
    , m_type(type)
    , m_hp(hp)
    , m_maxHp(hp)
    , m_baseColor(baseColor)
{
    if (m_type == BrickType::Indestructible) {
        m_hp = 9999;
        m_maxHp = 9999;
    }
}

bool Brick::hit(int damage) {
    if (m_destroyed || m_type == BrickType::Indestructible) {
        return false;
    }

    m_hp -= damage;
    if (m_hp <= 0) {
        m_hp = 0;
        m_destroyed = true;
        return true; // Destroyed
    }
    return false;
}

void Brick::render(RasterBuffer& buffer) const {
    if (m_destroyed) return;

    int ix = static_cast<int>(m_x);
    int iy = static_cast<int>(m_y);
    int iw = static_cast<int>(m_w);
    int ih = static_cast<int>(m_h);

    if (m_type == BrickType::Indestructible) {
        // Metallic steel look
        buffer.fillRect(ix, iy, iw, ih, Colors::BorderWall);
        // Bevel highlights
        buffer.drawFastHLine(ix, ix + iw - 1, iy, Colors::BorderGlow);
        buffer.drawFastVLine(ix, iy, iy + ih - 1, Colors::BorderGlow);
        buffer.drawFastHLine(ix, ix + iw - 1, iy + ih - 1, Colors::Black);
        buffer.drawFastVLine(ix + iw - 1, iy, iy + ih - 1, Colors::Black);
        // Rivet corner dots
        buffer.setPixel(ix + 2, iy + 2, Colors::White);
        buffer.setPixel(ix + iw - 3, iy + 2, Colors::White);
        buffer.setPixel(ix + 2, iy + ih - 3, Colors::White);
        buffer.setPixel(ix + iw - 3, iy + ih - 3, Colors::White);
        return;
    }

    if (m_type == BrickType::Explosive) {
        // Warning hazard styling
        buffer.fillRect(ix, iy, iw, ih, Colors::NeonOrange);
        buffer.drawFastHLine(ix, ix + iw - 1, iy, Colors::NeonYellow);
        buffer.drawFastVLine(ix, iy, iy + ih - 1, Colors::NeonYellow);
        buffer.drawFastHLine(ix, ix + iw - 1, iy + ih - 1, Colors::Black);
        buffer.drawFastVLine(ix + iw - 1, iy, iy + ih - 1, Colors::Black);
        // Hazard exclamation mark in center
        int midX = ix + iw / 2;
        int midY = iy + ih / 2;
        buffer.drawFastVLine(midX, midY - 2, midY, Colors::Black);
        buffer.setPixel(midX, midY + 2, Colors::Black);
        return;
    }

    // Standard & Armored & PowerUp bricks
    uint32_t fillCol = m_baseColor;
    if (m_maxHp > 1 && m_hp < m_maxHp) {
        // Darken color slightly as damage accumulates
        float healthRatio = static_cast<float>(m_hp) / static_cast<float>(m_maxHp);
        uint32_t r = static_cast<uint32_t>(((fillCol >> 16) & 0xFF) * (0.6f + 0.4f * healthRatio));
        uint32_t g = static_cast<uint32_t>(((fillCol >> 8) & 0xFF) * (0.6f + 0.4f * healthRatio));
        uint32_t b = static_cast<uint32_t>((fillCol & 0xFF) * (0.6f + 0.4f * healthRatio));
        fillCol = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    // Fill brick body
    buffer.fillRect(ix, iy, iw, ih, fillCol);

    // Bevel edges for 3D retro arcade pop
    uint32_t r = (fillCol >> 16) & 0xFF;
    uint32_t g = (fillCol >> 8) & 0xFF;
    uint32_t b = fillCol & 0xFF;

    uint32_t brightBevel = 0xFF000000 |
        (std::min(255u, r + 60) << 16) |
        (std::min(255u, g + 60) << 8) |
        std::min(255u, b + 60);

    uint32_t darkBevel = 0xFF000000 |
        (static_cast<uint32_t>(r * 0.45f) << 16) |
        (static_cast<uint32_t>(g * 0.45f) << 8) |
        static_cast<uint32_t>(b * 0.45f);

    buffer.drawFastHLine(ix, ix + iw - 1, iy, brightBevel);
    buffer.drawFastVLine(ix, iy, iy + ih - 1, brightBevel);
    buffer.drawFastHLine(ix, ix + iw - 1, iy + ih - 1, darkBevel);
    buffer.drawFastVLine(ix + iw - 1, iy, iy + ih - 1, darkBevel);

    // PowerUp brick visual indicator (glowing diamond mark)
    if (m_type == BrickType::PowerUp) {
        int midX = ix + iw / 2;
        int midY = iy + ih / 2;
        buffer.setPixel(midX, midY - 2, Colors::White);
        buffer.drawFastHLine(midX - 1, midX + 1, midY - 1, Colors::White);
        buffer.drawFastHLine(midX - 2, midX + 2, midY, Colors::White);
        buffer.drawFastHLine(midX - 1, midX + 1, midY + 1, Colors::White);
        buffer.setPixel(midX, midY + 2, Colors::White);
    }

    // Damage crack overlays
    if (m_maxHp > 1 && m_hp < m_maxHp) {
        int midX = ix + iw / 2;
        int midY = iy + ih / 2;
        // First crack stage
        buffer.drawLine(midX - 3, iy + 2, midX, midY, Colors::Black);
        buffer.drawLine(midX, midY, midX + 4, iy + ih - 3, Colors::Black);

        // Second crack stage if heavily damaged
        if (m_hp == 1 && m_maxHp >= 3) {
            buffer.drawLine(midX, midY, ix + 2, midY + 1, Colors::Black);
            buffer.drawLine(midX + 2, midY - 1, ix + iw - 3, midY - 2, Colors::Black);
        }
    }
}

AABB Brick::getBounds() const {
    return AABB(m_x, m_y, m_w, m_h);
}

Vec2 Brick::getCenter() const {
    return Vec2{m_x + m_w * 0.5f, m_y + m_h * 0.5f};
}

} // namespace Breakout
