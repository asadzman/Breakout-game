#include "Laser.h"
#include "core/Config.h"

namespace Breakout {

Laser::Laser(Vec2 pos)
    : m_pos(pos)
    , m_vel(0.0f, -LASER_SPEED)
{
}

void Laser::update(float dt) {
    if (!m_alive) return;
    m_pos += m_vel * dt;

    if (m_pos.y < PLAYFIELD_TOP) {
        m_alive = false;
    }
}

void Laser::render(RasterBuffer& buffer) const {
    if (!m_alive) return;
    int px = static_cast<int>(m_pos.x);
    int py = static_cast<int>(m_pos.y);

    // Twin-tone laser bolt
    buffer.fillRect(px, py, static_cast<int>(LASER_WIDTH), static_cast<int>(LASER_HEIGHT), Colors::LaserRed);
    buffer.fillRect(px, py + 1, static_cast<int>(LASER_WIDTH), static_cast<int>(LASER_HEIGHT - 2), Colors::White);
}

AABB Laser::getBounds() const {
    return AABB(m_pos.x, m_pos.y, LASER_WIDTH, LASER_HEIGHT);
}

} // namespace Breakout
