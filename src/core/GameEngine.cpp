#include "GameEngine.h"
#include <algorithm>
#include <cstdio>

namespace Breakout {

GameEngine::GameEngine() {
    startNewGame();
}

void GameEngine::initBricks() {
    m_bricks.clear();

    constexpr int ROWS = 5;
    constexpr int COLS = 10;
    constexpr float BRICK_W = 28.0f;
    constexpr float BRICK_H = 9.0f;
    constexpr float SPACING_X = 2.0f;
    constexpr float SPACING_Y = 2.0f;
    constexpr float START_X = 14.0f;
    constexpr float START_Y = 36.0f;

    const uint32_t rowColors[ROWS] = {
        Colors::BrickRed,
        Colors::BrickOrange,
        Colors::BrickYellow,
        Colors::BrickGreen,
        Colors::BrickCyan
    };

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            float x = START_X + static_cast<float>(c) * (BRICK_W + SPACING_X);
            float y = START_Y + static_cast<float>(r) * (BRICK_H + SPACING_Y);
            m_bricks.emplace_back(x, y, BRICK_W, BRICK_H, BrickType::Normal, 1, rowColors[r]);
        }
    }
}

void GameEngine::startNewGame() {
    m_score = 0;
    m_lives = 3;
    initBricks();
    restartCurrentLevel();
}

void GameEngine::restartCurrentLevel() {
    m_paddle.reset();
    m_ball.resetOnPaddle(m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f, m_paddle.getPosition().y);
    m_state = GameState::Ready;
    m_paused = false;
}

void GameEngine::setMoveLeft(bool active) {
    m_paddle.setMoveLeft(active);
}

void GameEngine::setMoveRight(bool active) {
    m_paddle.setMoveRight(active);
}

void GameEngine::handleMouseMove(float vx, float /*vy*/) {
    m_paddle.setTargetX(vx);
}

void GameEngine::handleMouseClick(float /*vx*/, float /*vy*/) {
    actionLaunchOrShoot();
}

void GameEngine::menuConfirm() {
    actionLaunchOrShoot();
}

void GameEngine::menuBack() {
    togglePause();
}

void GameEngine::actionLaunchOrShoot() {
    if (m_state == GameState::Ready) {
        m_ball.launch(-1.5707963f); // Launch straight up
        m_state = GameState::Playing;
    } else if (m_state == GameState::GameOver || m_state == GameState::LevelWon) {
        startNewGame();
    }
}

void GameEngine::togglePause() {
    if (m_state == GameState::Playing) {
        m_paused = !m_paused;
    }
}

void GameEngine::update(float dt) {
    if (m_paused) return;

    m_paddle.update(dt);

    if (m_state == GameState::Ready) {
        m_ball.resetOnPaddle(m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f, m_paddle.getPosition().y);
    } else if (m_state == GameState::Playing) {
        updatePhysics(dt);

        // Check victory condition
        bool anyBricksLeft = false;
        for (const auto& brick : m_bricks) {
            if (!brick.isDestroyed()) {
                anyBricksLeft = true;
                break;
            }
        }
        if (!anyBricksLeft) {
            m_state = GameState::LevelWon;
        }
    }
}

void GameEngine::updatePhysics(float dt) {
    constexpr int SUB_STEPS = 4;
    float subDt = dt / static_cast<float>(SUB_STEPS);

    for (int step = 0; step < SUB_STEPS; ++step) {
        m_ball.update(subDt);

        // Arena boundary collision
        int boundary = m_ball.checkArenaBoundaries();
        if (boundary == -1) {
            // Ball fell in floor pit
            m_lives--;
            if (m_lives <= 0) {
                m_state = GameState::GameOver;
            } else {
                m_state = GameState::Ready;
                m_ball.resetOnPaddle(m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f, m_paddle.getPosition().y);
            }
            return;
        }

        // Paddle collision
        if (m_ball.getVelocity().y > 0.0f) {
            CollisionResult pCol = Collision::testCircleAABB(m_ball.getPosition(), m_ball.getRadius(), m_paddle.getBounds());
            if (pCol.collided) {
                m_ball.deflectPaddle(
                    m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f,
                    m_paddle.getWidth(),
                    m_paddle.getVelocityX()
                );
            }
        }

        // Brick collision
        for (auto& brick : m_bricks) {
            if (brick.isDestroyed()) continue;

            CollisionResult bCol = Collision::testCircleAABB(m_ball.getPosition(), m_ball.getRadius(), brick.getBounds());
            if (bCol.collided) {
                m_ball.deflectNormal(bCol.normal);
                brick.hit(1);
                m_score += 100;
                break; // One collision per sub-step
            }
        }
    }
}

void GameEngine::render() {
    m_buffer.clear(Colors::DarkBg);

    // Draw arena walls
    // Top wall
    m_buffer.fillRect(PLAYFIELD_LEFT - 2, PLAYFIELD_TOP - 2, (PLAYFIELD_RIGHT - PLAYFIELD_LEFT) + 4, 2, Colors::BorderGlow);
    // Left wall
    m_buffer.fillRect(PLAYFIELD_LEFT - 2, PLAYFIELD_TOP - 2, 2, (PLAYFIELD_BOTTOM - PLAYFIELD_TOP) + 4, Colors::BorderGlow);
    // Right wall
    m_buffer.fillRect(PLAYFIELD_RIGHT, PLAYFIELD_TOP - 2, 2, (PLAYFIELD_BOTTOM - PLAYFIELD_TOP) + 4, Colors::BorderGlow);

    // Render bricks
    for (const auto& brick : m_bricks) {
        brick.render(m_buffer);
    }

    // Render paddle & ball
    m_paddle.render(m_buffer);
    m_ball.render(m_buffer);

    // Render UI HUD & Overlays
    renderHUD();
    renderOverlays();
}

void GameEngine::renderHUD() {
    // Header HUD banner
    m_buffer.drawFastHLine(0, VIRTUAL_WIDTH - 1, 16, Colors::BorderWall);

    char scoreStr[32];
    std::snprintf(scoreStr, sizeof(scoreStr), "SCORE: %06d", m_score);
    m_buffer.drawBitmapText(8, 5, scoreStr, Colors::White);

    char livesStr[32];
    std::snprintf(livesStr, sizeof(livesStr), "LIVES: %d", m_lives);
    m_buffer.drawBitmapText(VIRTUAL_WIDTH - 76, 5, livesStr, Colors::NeonYellow);
}

void GameEngine::renderOverlays() {
    if (m_state == GameState::Ready) {
        m_buffer.drawBitmapTextCentered(140, "PRESS SPACE OR CLICK TO LAUNCH", Colors::White);
        m_buffer.drawBitmapTextCentered(152, "A/D OR ARROWS OR MOUSE TO MOVE", Colors::GrayMid);
    } else if (m_state == GameState::GameOver) {
        m_buffer.fillRect(40, 90, 240, 56, 0xCC000000);
        m_buffer.drawRect(40, 90, 240, 56, Colors::BrickRed);
        m_buffer.drawBitmapTextCentered(102, "GAME OVER", Colors::BrickRed);
        m_buffer.drawBitmapTextCentered(124, "PRESS SPACE TO PLAY AGAIN", Colors::White);
    } else if (m_state == GameState::LevelWon) {
        m_buffer.fillRect(40, 90, 240, 56, 0xCC000000);
        m_buffer.drawRect(40, 90, 240, 56, Colors::NeonGreen);
        m_buffer.drawBitmapTextCentered(102, "STAGE CLEARED! VICTORY!", Colors::NeonGreen);
        m_buffer.drawBitmapTextCentered(124, "PRESS SPACE TO RESTART", Colors::White);
    } else if (m_paused) {
        m_buffer.fillRect(60, 100, 200, 40, 0xDD000000);
        m_buffer.drawRect(60, 100, 200, 40, Colors::NeonCyan);
        m_buffer.drawBitmapTextCentered(110, "PAUSED", Colors::NeonCyan);
        m_buffer.drawBitmapTextCentered(124, "PRESS P OR ESC TO RESUME", Colors::White);
    }
}

} // namespace Breakout
