#pragma once

#include "core/Config.h"
#include "physics/Vec2.h"
#include "physics/Collision.h"
#include "raster/RasterBuffer.h"

namespace Breakout {

class PowerUp {
public:
    PowerUp(Vec2 pos, PowerUpType type);

    void update(float dt);
    void render(RasterBuffer& buffer) const;

    [[nodiscard]] bool isAlive() const { return m_alive; }
    void kill() { m_alive = false; }
    [[nodiscard]] PowerUpType getType() const { return m_type; }
    [[nodiscard]] AABB getBounds() const;

    static uint32_t getColor(PowerUpType type);
    static char getIcon(PowerUpType type);
    static const char* getName(PowerUpType type);

private:
    Vec2 m_pos;
    Vec2 m_vel;
    PowerUpType m_type;
    bool m_alive = true;
    float m_animTimer = 0.0f;
};

} // namespace Breakout
