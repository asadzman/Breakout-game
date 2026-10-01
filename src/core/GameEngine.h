#pragma once

#include "core/Config.h"
#include "raster/RasterBuffer.h"
#include "entities/Paddle.h"
#include "entities/Ball.h"
#include "entities/Brick.h"
#include <vector>

namespace Breakout {

class GameEngine {
public:
    GameEngine();

    void update(float dt);
    void render();

    // Menu / Control Actions
    void menuUp() {}
    void menuDown() {}
    void menuLeft() {}
    void menuRight() {}
    void menuConfirm(); // Space / Enter
    void menuBack();    // Esc

    // Mouse interactions
    void handleMouseMove(float vx, float vy);
    void handleMouseClick(float vx, float vy);

    // Player inputs
    void setMoveLeft(bool active);
    void setMoveRight(bool active);
    void actionLaunchOrShoot();
    void togglePause();
    void restartCurrentLevel();
    void startNewGame();

    // State queries
    [[nodiscard]] GameState getState() const { return m_state; }
    [[nodiscard]] int getScore() const { return m_score; }
    [[nodiscard]] int getLives() const { return m_lives; }
    [[nodiscard]] const RasterBuffer& getBuffer() const { return m_buffer; }
    [[nodiscard]] RasterBuffer& getBuffer() { return m_buffer; }

private:
    void initBricks();
    void updatePhysics(float dt);
    void renderHUD();
    void renderOverlays();

    RasterBuffer m_buffer;
    Paddle m_paddle;
    Ball m_ball;
    std::vector<Brick> m_bricks;

    GameState m_state = GameState::Ready;
    int m_score = 0;
    int m_lives = 3;
    bool m_paused = false;
};

} // namespace Breakout
