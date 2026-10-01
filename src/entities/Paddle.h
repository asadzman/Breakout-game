#pragma once

#include "core/Config.h"
#include "physics/Vec2.h"
#include "physics/Collision.h"
#include "raster/RasterBuffer.h"

namespace Breakout {

class Paddle {
public:
    Paddle();

    void reset();
    void update(float dt);
    void render(RasterBuffer& buffer) const;

    // Movement inputs
    void setMoveLeft(bool active) { m_moveLeft = active; }
    void setMoveRight(bool active) { m_moveRight = active; }
    void setTargetX(float x); // For mouse tracking

    [[nodiscard]] Vec2 getPosition() const { return m_pos; }
    void setPosition(const Vec2& pos) { m_pos = pos; }
    [[nodiscard]] float getWidth() const { return m_width; }
    [[nodiscard]] float getHeight() const { return m_height; }
    [[nodiscard]] float getVelocityX() const { return m_vx; }
    [[nodiscard]] AABB getBounds() const;

private:
    Vec2 m_pos;
    float m_width = DEFAULT_PADDLE_WIDTH;
    float m_height = PADDLE_HEIGHT;
    float m_vx = 0.0f;

    bool m_moveLeft = false;
    bool m_moveRight = false;
    bool m_mouseControlActive = false;
    float m_mouseTargetX = 0.0f;
};

} // namespace Breakout
