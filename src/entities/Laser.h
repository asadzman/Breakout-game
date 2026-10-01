#pragma once

#include "physics/Vec2.h"
#include "physics/Collision.h"
#include "raster/RasterBuffer.h"

namespace Breakout {

class Laser {
public:
    Laser(Vec2 pos);

    void update(float dt);
    void render(RasterBuffer& buffer) const;

    [[nodiscard]] bool isAlive() const { return m_alive; }
    void kill() { m_alive = false; }
    [[nodiscard]] AABB getBounds() const;

private:
    Vec2 m_pos;
    Vec2 m_vel;
    bool m_alive = true;
};

} // namespace Breakout
