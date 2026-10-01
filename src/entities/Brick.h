#pragma once

#include "core/Config.h"
#include "physics/Vec2.h"
#include "physics/Collision.h"
#include "raster/RasterBuffer.h"

namespace Breakout {

enum class BrickType {
    Normal,
    Armored,
    Indestructible,
    Explosive,
    PowerUp
};

class Brick {
public:
    Brick(float x, float y, float w, float h, BrickType type, int hp, uint32_t baseColor);

    void render(RasterBuffer& buffer) const;

    // Hits brick for damage. Returns true if destroyed.
    bool hit(int damage = 1);

    [[nodiscard]] bool isDestroyed() const { return m_destroyed; }
    [[nodiscard]] bool isIndestructible() const { return m_type == BrickType::Indestructible; }
    [[nodiscard]] bool isExplosive() const { return m_type == BrickType::Explosive; }
    [[nodiscard]] BrickType getType() const { return m_type; }
    [[nodiscard]] int getHp() const { return m_hp; }
    [[nodiscard]] int getMaxHp() const { return m_maxHp; }
    [[nodiscard]] uint32_t getColor() const { return m_baseColor; }
    [[nodiscard]] AABB getBounds() const;
    [[nodiscard]] Vec2 getCenter() const;

private:
    float m_x;
    float m_y;
    float m_w;
    float m_h;
    BrickType m_type;
    int m_hp;
    int m_maxHp;
    uint32_t m_baseColor;
    bool m_destroyed = false;
};

} // namespace Breakout
