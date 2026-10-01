#pragma once

#include <memory>
#include <string>

namespace Breakout {

enum class SoundEffect {
    PaddleHit,
    BrickHit,
    WallBounce,
    LaserFire,
    PowerUpCollect,
    Explosion,
    BallLost,
    LevelWon,
    GameOver,
    MenuSelect,
    MenuChange
};

class SoundManager {
public:
    static SoundManager& instance();

    void play(SoundEffect effect);
    void playBrickTing(int comboStep = 0);
    void setMuted(bool muted);
    [[nodiscard]] bool isMuted() const { return m_muted; }
    void setVolume(float vol); // 0.0f to 1.0f
    [[nodiscard]] float getVolume() const { return m_volume; }

private:
    SoundManager();
    ~SoundManager();

    class Impl;
    std::unique_ptr<Impl> m_impl;

    bool m_muted = false;
    float m_volume = 0.8f;
};

} // namespace Breakout
