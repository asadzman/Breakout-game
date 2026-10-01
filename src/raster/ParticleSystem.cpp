#include "ParticleSystem.h"
#include <random>
#include <algorithm>

namespace Breakout {

static std::mt19937& getRng() {
    static std::mt19937 rng(1337);
    return rng;
}

void ParticleSystem::update(float dt) {
    constexpr float GRAVITY = 120.0f; // Soft downward gravity for debris

    size_t activeCount = 0;
    for (size_t i = 0; i < m_particles.size(); ++i) {
        m_particles[i].life -= dt;
        if (m_particles[i].life > 0.0f) {
            m_particles[i].pos += m_particles[i].vel * dt;
            m_particles[i].vel.y += GRAVITY * dt;
            m_particles[i].vel *= 0.98f; // Air friction
            if (activeCount != i) {
                m_particles[activeCount] = m_particles[i];
            }
            ++activeCount;
        }
    }
    m_particles.resize(activeCount);
}

void ParticleSystem::render(RasterBuffer& buffer) const {
    for (const auto& p : m_particles) {
        float alpha = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        int px = static_cast<int>(p.pos.x);
        int py = static_cast<int>(p.pos.y);

        if (p.size <= 1) {
            buffer.setPixelBlend(px, py, p.color, alpha);
        } else {
            for (int dy = 0; dy < p.size; ++dy) {
                for (int dx = 0; dx < p.size; ++dx) {
                    buffer.setPixelBlend(px + dx, py + dy, p.color, alpha);
                }
            }
        }
    }
}

void ParticleSystem::clear() {
    m_particles.clear();
}

void ParticleSystem::spawnSparks(Vec2 pos, uint32_t color, int count, float speed) {
    std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> speedDist(speed * 0.4f, speed);
    std::uniform_real_distribution<float> lifeDist(0.2f, 0.5f);

    for (int i = 0; i < count; ++i) {
        float angle = angleDist(getRng());
        float spd = speedDist(getRng());
        float life = lifeDist(getRng());

        m_particles.push_back(Particle{
            .pos = pos,
            .vel = Vec2{std::cos(angle) * spd, std::sin(angle) * spd},
            .color = color,
            .life = life,
            .maxLife = life,
            .size = 1
        });
    }
}

void ParticleSystem::spawnBrickExplosion(Vec2 center, float width, float height, uint32_t color, int count) {
    std::uniform_real_distribution<float> xDist(-width * 0.5f, width * 0.5f);
    std::uniform_real_distribution<float> yDist(-height * 0.5f, height * 0.5f);
    std::uniform_real_distribution<float> vxDist(-80.0f, 80.0f);
    std::uniform_real_distribution<float> vyDist(-120.0f, 30.0f);
    std::uniform_real_distribution<float> lifeDist(0.35f, 0.75f);
    std::uniform_int_distribution<int> sizeDist(1, 2);

    for (int i = 0; i < count; ++i) {
        float life = lifeDist(getRng());
        Vec2 spawnPos{center.x + xDist(getRng()), center.y + yDist(getRng())};

        m_particles.push_back(Particle{
            .pos = spawnPos,
            .vel = Vec2{vxDist(getRng()), vyDist(getRng())},
            .color = color,
            .life = life,
            .maxLife = life,
            .size = sizeDist(getRng())
        });
    }
}

} // namespace Breakout
