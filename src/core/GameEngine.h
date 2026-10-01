#pragma once

#include "core/Config.h"
#include "raster/RasterBuffer.h"
#include "raster/ParticleSystem.h"
#include "entities/Paddle.h"
#include "entities/Ball.h"
#include "entities/Brick.h"
#include "entities/PowerUp.h"
#include "entities/Laser.h"
#include "core/LevelManager.h"
#include <vector>
#include <string>

namespace Breakout {

class GameEngine {
public:
    GameEngine();

    void update(float dt);
    void render();

    // In-game Menu Navigation
    void menuUp();
    void menuDown();
    void menuLeft();
    void menuRight();
    void menuConfirm(); // Enter / Space
    void menuBack();    // Esc

    // Mouse interactions in virtual [0, 320] x [0, 240] coordinates
    void handleMouseMove(float vx, float vy);
    void handleMouseClick(float vx, float vy);

    // Game Actions
    void setMoveLeft(bool active);
    void setMoveRight(bool active);
    void setMouseTargetX(float x);
    void actionLaunchOrShoot();
    void togglePause();
    void restartCurrentLevel();
    void resetGame();
    void startNewGame();
    void selectAndStartLevel(int levelIdx);

    // Dynamic settings getters & setters
    void setDifficultyMultiplier(float mult);
    [[nodiscard]] float getDifficultyMultiplier() const { return m_difficultyMultiplier; }

    void setStartingLives(int lives);
    [[nodiscard]] int getStartingLives() const { return m_startingLives; }

    void setCrtScanlineMode(CrtScanlineMode mode) { m_crtMode = mode; }
    [[nodiscard]] CrtScanlineMode getCrtScanlineMode() const { return m_crtMode; }

    void setPaletteTheme(PaletteTheme theme) { m_paletteTheme = theme; }
    [[nodiscard]] PaletteTheme getPaletteTheme() const { return m_paletteTheme; }

    void setMouseControlEnabled(bool enabled) { m_mouseControlEnabled = enabled; }
    [[nodiscard]] bool isMouseControlEnabled() const { return m_mouseControlEnabled; }

    // State queries
    [[nodiscard]] GameState getState() const { return m_state; }
    [[nodiscard]] int getScore() const { return m_score; }
    [[nodiscard]] int getHighScore() const { return m_highScore; }
    [[nodiscard]] int getLives() const { return m_lives; }
    [[nodiscard]] float getLevelTime() const { return m_levelTime; }
    [[nodiscard]] const RasterBuffer& getBuffer() const { return m_buffer; }
    [[nodiscard]] RasterBuffer& getBuffer() { return m_buffer; }

    [[nodiscard]] int getTotalLevels() const { return m_levelManager.getTotalLevels(); }
    [[nodiscard]] int getCurrentLevelIndex() const { return m_levelManager.getCurrentLevelIndex(); }

private:
    void updatePhysicsSubSteps(float dt);
    void updateLasers(float dt);
    void updatePowerUps(float dt);
    void handleBallLost();
    void onBrickHit(Brick& brick, Vec2 hitPoint, bool destroyed);
    void triggerExplosiveChain(Vec2 center, float radius);
    void spawnRandomPowerUp(Vec2 pos);
    void activatePowerUp(PowerUpType type);

    // In-game screen renderers
    void renderHUD();
    void renderOverlays();
    void renderMainMenu();
    void renderLevelSelect();
    void renderOptionsMenu();
    void renderHelpMenu();
    void renderPauseMenu();

    void loadHighScore();
    void saveHighScore();

    RasterBuffer m_buffer;
    ParticleSystem m_particles;
    Paddle m_paddle;
    std::vector<Ball> m_balls;
    std::vector<Laser> m_lasers;
    std::vector<PowerUp> m_powerUps;
    LevelManager m_levelManager;

    GameState m_state = GameState::MainMenu;
    GameState m_previousState = GameState::MainMenu;
    int m_score = 0;
    int m_highScore = 0;
    int m_lives = 3;
    int m_startingLives = 3;
    int m_combo = 0;
    float m_levelTime = 0.0f;
    float m_stateTimer = 0.0f;
    float m_titleAnimTimer = 0.0f;

    // Menu selection indices
    int m_mainMenuIndex = 0;
    int m_pauseMenuIndex = 0;
    int m_optionsMenuIndex = 0;
    int m_levelSelectIndex = 0;

    // Modifiers & settings
    float m_difficultyMultiplier = 1.0f;
    bool m_hasShield = false;
    bool m_mouseControlEnabled = true;
    CrtScanlineMode m_crtMode = CrtScanlineMode::Subtle;
    PaletteTheme m_paletteTheme = PaletteTheme::NeonArcade;
};

} // namespace Breakout
