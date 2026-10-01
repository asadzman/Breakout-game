#pragma once

#include "RasterBuffer.h"
#include "physics/Vec2.h"
#include <vector>

namespace Breakout {

struct Particle {
    Vec2 pos;
    Vec2 vel;
    uint32_t color;
    float life;      // Current life remaining in seconds
    float maxLife;   // Total lifetime in seconds
    int size;        // Pixel size (1 or 2)
};

class ParticleSystem {
public:
    ParticleSystem() = default;

    void update(float dt);
    void render(RasterBuffer& buffer) const;
    void clear();

    void spawnSparks(Vec2 pos, uint32_t color, int count = 12, float speed = 90.0f);
    void spawnBrickExplosion(Vec2 center, float width, float height, uint32_t color, int count = 20);

private:
    std::vector<Particle> m_particles;
};

} // namespace Breakout
