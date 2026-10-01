#include "PowerUp.h"
#include <string>

namespace Breakout {

PowerUp::PowerUp(Vec2 pos, PowerUpType type)
    : m_pos(pos)
    , m_vel(0.0f, POWERUP_FALL_SPEED)
    , m_type(type)
{
}

void PowerUp::update(float dt) {
    if (!m_alive) return;
    m_pos += m_vel * dt;
    m_animTimer += dt;

    if (m_pos.y > VIRTUAL_HEIGHT + 10.0f) {
        m_alive = false;
    }
}

void PowerUp::render(RasterBuffer& buffer) const {
    if (!m_alive) return;

    int px = static_cast<int>(m_pos.x);
    int py = static_cast<int>(m_pos.y);
    int w = static_cast<int>(POWERUP_WIDTH);
    int h = static_cast<int>(POWERUP_HEIGHT);

    uint32_t col = getColor(m_type);

    // Pill/Capsule body
    buffer.fillRect(px + 1, py, w - 2, h, col);
    buffer.fillRect(px, py + 1, w, h - 2, col);

    // Highlight line
    buffer.drawFastHLine(px + 2, px + w - 3, py + 1, Colors::White);

    // Inner dark center
    buffer.fillRect(px + 3, py + 2, w - 6, h - 4, Colors::Black);

    // Single-char letter icon
    char icon = getIcon(m_type);
    char str[2] = {icon, '\0'};
    buffer.drawBitmapText(px + (w - 8) / 2, py + (h - 8) / 2, str, Colors::White, 1);
}

AABB PowerUp::getBounds() const {
    return AABB(m_pos.x, m_pos.y, POWERUP_WIDTH, POWERUP_HEIGHT);
}

uint32_t PowerUp::getColor(PowerUpType type) {
    switch (type) {
        case PowerUpType::Elongate:    return Colors::NeonGreen;
        case PowerUpType::Shrink:      return Colors::NeonPink;
        case PowerUpType::MultiBall:   return Colors::NeonCyan;
        case PowerUpType::Fireball:    return Colors::NeonOrange;
        case PowerUpType::Laser:       return Colors::NeonYellow;
        case PowerUpType::StickyCatch: return Colors::NeonPurple;
        case PowerUpType::SlowBall:    return Colors::NeonBlue;
        case PowerUpType::Shield:      return Colors::BrickCyan;
        case PowerUpType::ExtraLife:   return Colors::BrickRed;
        default:                       return Colors::White;
    }
}

char PowerUp::getIcon(PowerUpType type) {
    switch (type) {
        case PowerUpType::Elongate:    return 'E';
        case PowerUpType::Shrink:      return 'S';
        case PowerUpType::MultiBall:   return 'M';
        case PowerUpType::Fireball:    return 'F';
        case PowerUpType::Laser:       return 'L';
        case PowerUpType::StickyCatch: return 'C';
        case PowerUpType::SlowBall:    return 'Z';
        case PowerUpType::Shield:      return 'B';
        case PowerUpType::ExtraLife:   return '+';
        default:                       return '?';
    }
}

const char* PowerUp::getName(PowerUpType type) {
    switch (type) {
        case PowerUpType::Elongate:    return "WIDE PADDLE";
        case PowerUpType::Shrink:      return "TINY PADDLE";
        case PowerUpType::MultiBall:   return "MULTI-BALL";
        case PowerUpType::Fireball:    return "FIREBALL";
        case PowerUpType::Laser:       return "LASER CANNONS";
        case PowerUpType::StickyCatch: return "STICKY CATCH";
        case PowerUpType::SlowBall:    return "SLOW MOTION";
        case PowerUpType::Shield:      return "SAFETY SHIELD";
        case PowerUpType::ExtraLife:   return "+1 EXTRA LIFE";
        default:                       return "UNKNOWN";
    }
}

} // namespace Breakout
