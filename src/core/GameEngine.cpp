#include "GameEngine.h"
#include "audio/SoundManager.h"
#include "raster/BitmapFont.h"
#include <QSettings>
#include <QCoreApplication>
#include <random>
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace Breakout {

static std::mt19937& getEngineRng() {
    static std::mt19937 rng(4242);
    return rng;
}

GameEngine::GameEngine()
    : m_buffer(VIRTUAL_WIDTH, VIRTUAL_HEIGHT)
{
    loadSettings();
    loadHighScore();
    m_levelManager.loadLevel(0);
    m_state = GameState::MainMenu;
}

void GameEngine::loadHighScore() {
    QSettings settings("BreakoutGame", "RetroBreakout");
    m_highScore = settings.value("highScore", 0).toInt();
}

void GameEngine::saveHighScore() {
    if (m_score > m_highScore) {
        m_highScore = m_score;
        QSettings settings("BreakoutGame", "RetroBreakout");
        settings.setValue("highScore", m_highScore);
    }
}

void GameEngine::loadSettings() {
    QSettings settings("BreakoutGame", "RetroBreakout");
    m_difficultyMultiplier = settings.value("speedMultiplier", 1.0f).toFloat();
    m_startingLives = settings.value("startingLives", 3).toInt();
    m_crtMode = static_cast<CrtScanlineMode>(settings.value("crtMode", static_cast<int>(CrtScanlineMode::Subtle)).toInt());
    m_paletteTheme = static_cast<PaletteTheme>(settings.value("paletteTheme", static_cast<int>(PaletteTheme::NeonArcade)).toInt());
    m_paddleSkin = static_cast<PaddleSkin>(settings.value("paddleSkin", static_cast<int>(PaddleSkin::Skateboard)).toInt());
    m_ballSkin = static_cast<BallSkin>(settings.value("ballSkin", static_cast<int>(BallSkin::EnergyOrb)).toInt());
    m_mouseControlEnabled = settings.value("mouseControl", true).toBool();

    float vol = settings.value("volume", 0.8f).toFloat();
    bool muted = settings.value("muted", false).toBool();
    SoundManager::instance().setVolume(vol);
    SoundManager::instance().setMuted(muted);
    m_paddle.setSkin(m_paddleSkin);
}

void GameEngine::saveSettings() {
    QSettings settings("BreakoutGame", "RetroBreakout");
    settings.setValue("speedMultiplier", m_difficultyMultiplier);
    settings.setValue("startingLives", m_startingLives);
    settings.setValue("crtMode", static_cast<int>(m_crtMode));
    settings.setValue("paletteTheme", static_cast<int>(m_paletteTheme));
    settings.setValue("paddleSkin", static_cast<int>(m_paddleSkin));
    settings.setValue("ballSkin", static_cast<int>(m_ballSkin));
    settings.setValue("mouseControl", m_mouseControlEnabled);
    settings.setValue("volume", SoundManager::instance().getVolume());
    settings.setValue("muted", SoundManager::instance().isMuted());
}

void GameEngine::openMainMenu() {
    m_state = GameState::MainMenu;
    m_mainMenuIndex = 0;
}

void GameEngine::openOptionsMenu() {
    if (m_state != GameState::OptionsMenu) {
        m_previousState = m_state;
        m_optionsMenuIndex = 0;
        m_state = GameState::OptionsMenu;
    }
}

void GameEngine::openLevelSelect() {
    if (m_state != GameState::LevelSelect) {
        m_previousState = m_state;
        m_levelSelectIndex = m_levelManager.getCurrentLevelIndex();
        m_state = GameState::LevelSelect;
    }
}

void GameEngine::openHelpMenu() {
    if (m_state != GameState::HelpMenu) {
        m_previousState = m_state;
        m_state = GameState::HelpMenu;
    }
}

void GameEngine::openAboutMenu() {
    if (m_state != GameState::AboutMenu) {
        m_previousState = m_state;
        m_state = GameState::AboutMenu;
    }
}

void GameEngine::triggerScreenShake(float duration, float intensity) {
    m_shakeTimer = duration;
    m_shakeIntensity = intensity;
}

void GameEngine::spawnFloatingScore(Vec2 pos, std::string text, uint32_t color) {
    m_floatingTexts.push_back(FloatingText{
        .pos = pos,
        .text = std::move(text),
        .color = color,
        .life = 0.8f,
        .maxLife = 0.8f
    });
}

void GameEngine::updateFloatingTexts(float dt) {
    for (auto it = m_floatingTexts.begin(); it != m_floatingTexts.end();) {
        it->life -= dt;
        if (it->life <= 0.0f) {
            it = m_floatingTexts.erase(it);
        } else {
            it->pos.y -= 20.0f * dt;
            ++it;
        }
    }
}

void GameEngine::renderFloatingTexts() {
    for (const auto& ft : m_floatingTexts) {
        int tx = static_cast<int>(ft.pos.x) - (BitmapFont::measureText(ft.text, 1) / 2);
        int ty = static_cast<int>(ft.pos.y);
        m_buffer.drawBitmapText(tx, ty, ft.text, ft.color, 1);
    }
}

void GameEngine::startNewGame() {
    m_score = 0;
    m_lives = m_startingLives;
    m_combo = 0;
    m_levelTime = 0.0f;
    m_hasShield = false;
    m_floatingTexts.clear();
    m_levelManager.loadLevel(0);
    restartCurrentLevel();
}

void GameEngine::selectAndStartLevel(int levelIdx) {
    m_score = 0;
    m_lives = m_startingLives;
    m_combo = 0;
    m_levelTime = 0.0f;
    m_hasShield = false;
    m_floatingTexts.clear();
    m_levelManager.loadLevel(levelIdx);
    restartCurrentLevel();
}

void GameEngine::resetGame() {
    startNewGame();
}

void GameEngine::restartCurrentLevel() {
    m_lasers.clear();
    m_powerUps.clear();
    m_particles.clear();
    m_floatingTexts.clear();
    m_paddle.reset();
    m_paddle.setSkin(m_paddleSkin);
    m_hasShield = false;

    m_balls.clear();
    Ball mainBall;
    mainBall.setSkin(m_ballSkin);
    mainBall.resetOnPaddle(m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f, m_paddle.getPosition().y);
    mainBall.setSlow(m_difficultyMultiplier < 0.95f, m_difficultyMultiplier);
    m_balls.push_back(mainBall);

    m_state = GameState::Ready;
    m_stateTimer = 0.0f;
}

void GameEngine::setDifficultyMultiplier(float mult) {
    m_difficultyMultiplier = std::clamp(mult, 0.6f, 1.8f);
    for (auto& b : m_balls) {
        b.setSlow(m_difficultyMultiplier < 0.95f, m_difficultyMultiplier);
    }
    saveSettings();
}

void GameEngine::setStartingLives(int lives) {
    m_startingLives = std::clamp(lives, 1, 9);
    if (m_state == GameState::Ready && m_score == 0) {
        m_lives = m_startingLives;
    }
    saveSettings();
}

void GameEngine::setPaddleSkin(PaddleSkin skin) {
    m_paddleSkin = skin;
    m_paddle.setSkin(skin);
    saveSettings();
}

void GameEngine::setBallSkin(BallSkin skin) {
    m_ballSkin = skin;
    for (auto& b : m_balls) {
        b.setSkin(skin);
    }
    saveSettings();
}

void GameEngine::setMoveLeft(bool active) {
    m_paddle.setMoveLeft(active);
}

void GameEngine::setMoveRight(bool active) {
    m_paddle.setMoveRight(active);
}

void GameEngine::setMouseTargetX(float x) {
    if (m_mouseControlEnabled) {
        m_paddle.setTargetX(x);
    }
}

// In-Game Menu Navigation
void GameEngine::menuUp() {
    SoundManager::instance().play(SoundEffect::MenuSelect);
    if (m_state == GameState::MainMenu) {
        m_mainMenuIndex = (m_mainMenuIndex + 4) % 5;
    } else if (m_state == GameState::Paused) {
        m_pauseMenuIndex = (m_pauseMenuIndex + 4) % 5;
    } else if (m_state == GameState::OptionsMenu) {
        m_optionsMenuIndex = (m_optionsMenuIndex + 8) % 9;
    } else if (m_state == GameState::LevelSelect) {
        m_levelSelectIndex = (m_levelSelectIndex + m_levelManager.getTotalLevels() - 1) % m_levelManager.getTotalLevels();
    }
}

void GameEngine::menuDown() {
    SoundManager::instance().play(SoundEffect::MenuSelect);
    if (m_state == GameState::MainMenu) {
        m_mainMenuIndex = (m_mainMenuIndex + 1) % 5;
    } else if (m_state == GameState::Paused) {
        m_pauseMenuIndex = (m_pauseMenuIndex + 1) % 5;
    } else if (m_state == GameState::OptionsMenu) {
        m_optionsMenuIndex = (m_optionsMenuIndex + 1) % 9;
    } else if (m_state == GameState::LevelSelect) {
        m_levelSelectIndex = (m_levelSelectIndex + 1) % m_levelManager.getTotalLevels();
    }
}

void GameEngine::menuLeft() {
    if (m_state == GameState::LevelSelect) {
        SoundManager::instance().play(SoundEffect::MenuChange);
        m_levelSelectIndex = (m_levelSelectIndex + m_levelManager.getTotalLevels() - 1) % m_levelManager.getTotalLevels();
        return;
    }

    if (m_state == GameState::OptionsMenu) {
        SoundManager::instance().play(SoundEffect::MenuChange);
        switch (m_optionsMenuIndex) {
            case 0: // Speed
                setDifficultyMultiplier(m_difficultyMultiplier - 0.15f);
                break;
            case 1: // Lives
                setStartingLives(m_startingLives - 1);
                break;
            case 2: // CRT
                m_crtMode = static_cast<CrtScanlineMode>((static_cast<int>(m_crtMode) + 2) % 3);
                break;
            case 3: // Theme
                m_paletteTheme = static_cast<PaletteTheme>((static_cast<int>(m_paletteTheme) + 5) % 6);
                break;
            case 4: // Paddle / Board Preset
                setPaddleSkin(static_cast<PaddleSkin>((static_cast<int>(m_paddleSkin) + 3) % 4));
                break;
            case 5: // Ball Preset
                setBallSkin(static_cast<BallSkin>((static_cast<int>(m_ballSkin) + 3) % 4));
                break;
            case 6: { // Audio
                float cur = SoundManager::instance().getVolume();
                if (SoundManager::instance().isMuted()) {
                    SoundManager::instance().setMuted(false);
                } else if (cur <= 0.25f) {
                    SoundManager::instance().setMuted(true);
                } else {
                    SoundManager::instance().setVolume(cur - 0.25f);
                }
                break;
            }
            case 7: // Mouse
                m_mouseControlEnabled = !m_mouseControlEnabled;
                break;
            default:
                break;
        }
        saveSettings();
    }
}

void GameEngine::menuRight() {
    if (m_state == GameState::LevelSelect) {
        SoundManager::instance().play(SoundEffect::MenuChange);
        m_levelSelectIndex = (m_levelSelectIndex + 1) % m_levelManager.getTotalLevels();
        return;
    }

    if (m_state == GameState::OptionsMenu) {
        SoundManager::instance().play(SoundEffect::MenuChange);
        switch (m_optionsMenuIndex) {
            case 0: // Speed
                setDifficultyMultiplier(m_difficultyMultiplier + 0.15f);
                break;
            case 1: // Lives
                setStartingLives(m_startingLives + 1);
                break;
            case 2: // CRT
                m_crtMode = static_cast<CrtScanlineMode>((static_cast<int>(m_crtMode) + 1) % 3);
                break;
            case 3: // Theme
                m_paletteTheme = static_cast<PaletteTheme>((static_cast<int>(m_paletteTheme) + 1) % 6);
                break;
            case 4: // Paddle Skin
                setPaddleSkin(static_cast<PaddleSkin>((static_cast<int>(m_paddleSkin) + 1) % 4));
                break;
            case 5: // Ball Skin
                setBallSkin(static_cast<BallSkin>((static_cast<int>(m_ballSkin) + 1) % 4));
                break;
            case 6: { // Audio
                if (SoundManager::instance().isMuted()) {
                    SoundManager::instance().setMuted(false);
                    SoundManager::instance().setVolume(0.25f);
                } else {
                    float cur = SoundManager::instance().getVolume();
                    if (cur >= 0.95f) {
                        SoundManager::instance().setMuted(true);
                    } else {
                        SoundManager::instance().setVolume(cur + 0.25f);
                    }
                }
                break;
            }
            case 7: // Mouse
                m_mouseControlEnabled = !m_mouseControlEnabled;
                break;
            default:
                break;
        }
        saveSettings();
    }
}

void GameEngine::menuConfirm() {
    SoundManager::instance().play(SoundEffect::MenuSelect);

    if (m_state == GameState::MainMenu) {
        switch (m_mainMenuIndex) {
            case 0: // Start / Resume
                if (!m_balls.empty() && m_levelTime > 0.1f) {
                    m_state = GameState::Playing;
                } else {
                    startNewGame();
                }
                break;
            case 1: // Select Level
                m_previousState = GameState::MainMenu;
                m_levelSelectIndex = m_levelManager.getCurrentLevelIndex();
                m_state = GameState::LevelSelect;
                break;
            case 2: // Options
                m_previousState = GameState::MainMenu;
                m_optionsMenuIndex = 0;
                m_state = GameState::OptionsMenu;
                break;
            case 3: // Help
                m_previousState = GameState::MainMenu;
                m_state = GameState::HelpMenu;
                break;
            case 4: // Quit
                QCoreApplication::quit();
                break;
        }
        return;
    }

    if (m_state == GameState::LevelSelect) {
        selectAndStartLevel(m_levelSelectIndex);
        return;
    }

    if (m_state == GameState::OptionsMenu) {
        if (m_optionsMenuIndex == 8) { // Back
            menuBack();
        } else {
            menuRight(); // Toggle option value
        }
        return;
    }

    if (m_state == GameState::HelpMenu || m_state == GameState::AboutMenu) {
        menuBack();
        return;
    }

    if (m_state == GameState::Paused) {
        switch (m_pauseMenuIndex) {
            case 0: // Resume
                m_state = GameState::Playing;
                break;
            case 1: // Restart Stage
                restartCurrentLevel();
                break;
            case 2: // Options
                m_previousState = GameState::Paused;
                m_optionsMenuIndex = 0;
                m_state = GameState::OptionsMenu;
                break;
            case 3: // Select Level
                m_previousState = GameState::Paused;
                m_levelSelectIndex = m_levelManager.getCurrentLevelIndex();
                m_state = GameState::LevelSelect;
                break;
            case 4: // Main Menu
                m_state = GameState::MainMenu;
                break;
        }
        return;
    }

    actionLaunchOrShoot();
}

void GameEngine::menuBack() {
    SoundManager::instance().play(SoundEffect::MenuChange);
    if (m_state == GameState::OptionsMenu || m_state == GameState::LevelSelect ||
        m_state == GameState::HelpMenu || m_state == GameState::AboutMenu) {
        m_state = m_previousState;
    } else if (m_state == GameState::Playing) {
        m_state = GameState::Paused;
        m_pauseMenuIndex = 0;
    } else if (m_state == GameState::Paused) {
        m_state = GameState::Playing;
    }
}

void GameEngine::handleMouseMove(float vx, float /*vy*/) {
    if (m_state == GameState::Ready || m_state == GameState::Playing) {
        setMouseTargetX(vx);
    }
}

void GameEngine::handleMouseClick(float vx, float vy) {
    if (m_state == GameState::MainMenu) {
        for (int i = 0; i < 5; ++i) {
            int itemY = 96 + i * 20;
            if (vy >= itemY - 4 && vy <= itemY + 16 && vx >= 40 && vx <= 280) {
                m_mainMenuIndex = i;
                menuConfirm();
                return;
            }
        }
        return;
    } else if (m_state == GameState::Paused) {
        for (int i = 0; i < 5; ++i) {
            int itemY = 76 + i * 20;
            if (vy >= itemY - 4 && vy <= itemY + 16 && vx >= 50 && vx <= 270) {
                m_pauseMenuIndex = i;
                menuConfirm();
                return;
            }
        }
        return;
    } else if (m_state == GameState::HelpMenu || m_state == GameState::AboutMenu) {
        menuConfirm();
    } else if (m_state == GameState::LevelSelect) {
        if (vy >= 40 && vy <= 70) {
            if (vx < 120) menuLeft();
            else if (vx > 200) menuRight();
        } else if (vy >= 180 && vy <= 204) {
            menuConfirm();
        } else if (vy >= 205) {
            menuBack();
        }
    } else if (m_state == GameState::OptionsMenu) {
        for (int i = 0; i < 9; ++i) {
            int itemY = 38 + i * 19;
            if (vy >= itemY - 4 && vy <= itemY + 16 && vx >= 20 && vx <= 300) {
                m_optionsMenuIndex = i;
                if (i == 8) { // [ BACK TO MENU ]
                    menuBack();
                } else {
                    if (vx < 210) {
                        menuLeft();
                    } else {
                        menuRight();
                    }
                }
                return;
            }
        }
    } else {
        actionLaunchOrShoot();
    }
}

void GameEngine::actionLaunchOrShoot() {
    if (m_state == GameState::Ready) {
        if (!m_balls.empty()) {
            m_balls[0].launch(-1.5707963f);
        }
        m_state = GameState::Playing;
        SoundManager::instance().play(SoundEffect::PaddleHit);
        return;
    }

    if (m_state == GameState::Playing) {
        if (m_paddle.hasLaser()) {
            auto fired = m_paddle.tryFireLasers();
            if (!fired.empty()) {
                for (const auto& l : fired) {
                    m_lasers.push_back(l);
                }
                SoundManager::instance().play(SoundEffect::LaserFire);
            }
        }

        for (auto& ball : m_balls) {
            if (ball.isStuck()) {
                ball.launch(-1.5707963f);
                SoundManager::instance().play(SoundEffect::PaddleHit);
            }
        }
        return;
    }

    if (m_state == GameState::LevelWon) {
        if (m_levelManager.hasNextLevel()) {
            m_levelManager.advanceLevel();
            restartCurrentLevel();
        } else {
            m_state = GameState::GameCompleted;
        }
        return;
    }

    if (m_state == GameState::GameOver || m_state == GameState::GameCompleted) {
        m_state = GameState::MainMenu;
        return;
    }
}

void GameEngine::togglePause() {
    if (m_state == GameState::Playing) {
        m_state = GameState::Paused;
        m_pauseMenuIndex = 0;
    } else if (m_state == GameState::Paused) {
        m_state = GameState::Playing;
    } else if (m_state == GameState::OptionsMenu || m_state == GameState::LevelSelect ||
               m_state == GameState::HelpMenu || m_state == GameState::AboutMenu) {
        menuBack();
    }
}

void GameEngine::update(float dt) {
    dt = std::min(dt, 0.05f);
    m_titleAnimTimer += dt;

    if (m_state == GameState::MainMenu || m_state == GameState::LevelSelect ||
        m_state == GameState::OptionsMenu || m_state == GameState::HelpMenu ||
        m_state == GameState::AboutMenu || m_state == GameState::Paused) {
        return;
    }

    if (m_shakeTimer > 0.0f) {
        m_shakeTimer = std::max(0.0f, m_shakeTimer - dt);
    }
    updateFloatingTexts(dt);

    if (m_state == GameState::BallLost) {
        m_stateTimer -= dt;
        m_particles.update(dt);
        if (m_stateTimer <= 0.0f) {
            if (m_lives > 0) {
                m_balls.clear();
                Ball ball;
                ball.resetOnPaddle(m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f, m_paddle.getPosition().y);
                ball.setSlow(m_difficultyMultiplier < 0.95f, m_difficultyMultiplier);
                m_balls.push_back(ball);
                m_state = GameState::Ready;
            } else {
                m_state = GameState::GameOver;
                saveHighScore();
                SoundManager::instance().play(SoundEffect::GameOver);
            }
        }
        return;
    }

    if (m_state == GameState::LevelWon || m_state == GameState::GameOver || m_state == GameState::GameCompleted) {
        m_particles.update(dt);
        return;
    }

    m_levelTime += dt;
    m_paddle.update(dt);
    m_particles.update(dt);

    if (m_state == GameState::Ready) {
        if (!m_balls.empty()) {
            m_balls[0].resetOnPaddle(m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f, m_paddle.getPosition().y);
        }
        return;
    }

    updatePhysicsSubSteps(dt);
    updateLasers(dt);
    updatePowerUps(dt);

    if (m_levelManager.getRemainingDestructibleBricks() == 0) {
        m_state = GameState::LevelWon;
        saveHighScore();
        SoundManager::instance().play(SoundEffect::LevelWon);
        m_particles.spawnBrickExplosion(Vec2{VIRTUAL_WIDTH * 0.5f, 80.0f}, 120.0f, 40.0f, Colors::NeonYellow, 40);
    }
}

void GameEngine::updatePhysicsSubSteps(float dt) {
    float subDt = dt / static_cast<float>(PHYSICS_SUB_STEPS);

    for (int step = 0; step < PHYSICS_SUB_STEPS; ++step) {
        AABB paddleBox = m_paddle.getBounds();

        for (auto& ball : m_balls) {
            if (!ball.isAlive()) continue;

            if (ball.isStuck()) {
                ball.setPosition(Vec2{m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f,
                                      m_paddle.getPosition().y - ball.getRadius() - 1.0f});
                continue;
            }

            ball.update(subDt);

            int boundHit = ball.checkArenaBoundaries();
            if (boundHit == 1 || boundHit == 2) {
                SoundManager::instance().play(SoundEffect::WallBounce);
                m_particles.spawnSparks(ball.getPosition(), Colors::BorderGlow, 4, 40.0f);
            } else if (boundHit == -1) {
                if (m_hasShield) {
                    m_hasShield = false;
                    ball.setPosition(Vec2{ball.getPosition().x, static_cast<float>(PLAYFIELD_BOTTOM - 2)});
                    Vec2 vel = ball.getVelocity();
                    vel.y = -std::abs(vel.y);
                    ball.setVelocity(vel);
                    SoundManager::instance().play(SoundEffect::Explosion);
                    triggerScreenShake(0.20f, 2.5f);
                    m_particles.spawnBrickExplosion(Vec2{VIRTUAL_WIDTH * 0.5f, static_cast<float>(PLAYFIELD_BOTTOM)},
                                                   VIRTUAL_WIDTH, 4.0f, Colors::NeonCyan, 30);
                }
            }

            if (!ball.isAlive()) continue;

            CollisionResult pCol = Collision::testCircleAABB(ball.getPosition(), ball.getRadius(), paddleBox);
            if (pCol.collided && ball.getVelocity().y > 0.0f) {
                m_paddle.triggerHitFlash();
                if (m_paddle.isSticky()) {
                    ball.stickToPaddle(ball.getPosition().x - (m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f));
                    SoundManager::instance().play(SoundEffect::PaddleHit);
                } else {
                    ball.deflectPaddle(m_paddle.getPosition().x + m_paddle.getWidth() * 0.5f,
                                       m_paddle.getWidth(),
                                       m_paddle.getVelocityX());
                    m_combo = 0;
                    SoundManager::instance().play(SoundEffect::PaddleHit);
                    m_particles.spawnSparks(pCol.contactPoint, Colors::NeonCyan, 8, 70.0f);
                }
                continue;
            }

            auto& bricks = m_levelManager.getBricks();
            for (auto& brick : bricks) {
                if (brick.isDestroyed()) continue;

                CollisionResult bCol = Collision::testCircleAABB(ball.getPosition(), ball.getRadius(), brick.getBounds());
                if (bCol.collided) {
                    if (!ball.isFireball()) {
                        ball.setPosition(ball.getPosition() + bCol.normal * (bCol.penetration + 0.2f));
                        ball.deflectNormal(bCol.normal);
                    }

                    bool destroyed = brick.hit(1);
                    onBrickHit(brick, bCol.contactPoint, destroyed);

                    if (!ball.isFireball()) {
                        break;
                    }
                }
            }
        }

        m_balls.erase(std::remove_if(m_balls.begin(), m_balls.end(),
                                     [](const Ball& b) { return !b.isAlive(); }),
                      m_balls.end());

        if (m_balls.empty()) {
            handleBallLost();
            break;
        }
    }
}

void GameEngine::onBrickHit(Brick& brick, Vec2 hitPoint, bool destroyed) {
    m_combo++;
    int points = (destroyed ? 100 : 40) + m_combo * 15;
    m_score += points;
    saveHighScore();

    if (destroyed) {
        SoundManager::instance().playBrickTing(m_combo);
        m_particles.spawnBrickExplosion(brick.getCenter(), 28.0f, 9.0f, brick.getColor(), 18);

        char scoreStr[16];
        std::snprintf(scoreStr, sizeof(scoreStr), "+%d", points);
        spawnFloatingScore(brick.getCenter(), scoreStr, (m_combo > 2) ? Colors::NeonYellow : Colors::White);
        if (m_combo >= 3 && m_combo % 2 == 1) {
            char comboStr[16];
            std::snprintf(comboStr, sizeof(comboStr), "COMBO x%d!", m_combo);
            spawnFloatingScore(Vec2{brick.getCenter().x, brick.getCenter().y - 8.0f}, comboStr, Colors::NeonGreen);
        }

        if (brick.isExplosive()) {
            triggerExplosiveChain(brick.getCenter(), 34.0f);
        }

        if (brick.getType() == BrickType::PowerUp) {
            spawnRandomPowerUp(brick.getCenter());
        } else {
            std::uniform_int_distribution<int> dropDist(1, 100);
            if (dropDist(getEngineRng()) <= 15) {
                spawnRandomPowerUp(brick.getCenter());
            }
        }
    } else {
        SoundManager::instance().playBrickTing(0);
        m_particles.spawnSparks(hitPoint, brick.getColor(), 6, 60.0f);
    }
}

void GameEngine::triggerExplosiveChain(Vec2 center, float radius) {
    SoundManager::instance().play(SoundEffect::Explosion);
    triggerScreenShake(0.22f, 3.0f);
    auto& bricks = m_levelManager.getBricks();
    for (auto& b : bricks) {
        if (!b.isDestroyed() && !b.isIndestructible()) {
            if (b.getCenter().distanceTo(center) <= radius) {
                bool destroyed = b.hit(2);
                if (destroyed) {
                    m_score += 150;
                    spawnFloatingScore(b.getCenter(), "+150", Colors::NeonOrange);
                    m_particles.spawnBrickExplosion(b.getCenter(), 28.0f, 9.0f, b.getColor(), 14);
                }
            }
        }
    }
}

void GameEngine::spawnRandomPowerUp(Vec2 pos) {
    std::uniform_int_distribution<int> typeDist(1, 9);
    PowerUpType type = static_cast<PowerUpType>(typeDist(getEngineRng()));
    m_powerUps.emplace_back(pos, type);
}

void GameEngine::activatePowerUp(PowerUpType type) {
    SoundManager::instance().play(SoundEffect::PowerUpCollect);
    m_score += 250;
    char pupScoreStr[32];
    std::snprintf(pupScoreStr, sizeof(pupScoreStr), "+250 %s", PowerUp::getName(type));
    spawnFloatingScore(m_paddle.getPosition(), pupScoreStr, PowerUp::getColor(type));

    switch (type) {
        case PowerUpType::Elongate:
        case PowerUpType::Shrink:
        case PowerUpType::Laser:
        case PowerUpType::StickyCatch:
            m_paddle.applyPowerUp(type);
            break;

        case PowerUpType::MultiBall: {
            std::vector<Ball> spawned;
            for (const auto& b : m_balls) {
                Vec2 pos = b.getPosition();
                Vec2 vel = b.getVelocity();
                float spd = vel.length();
                if (spd < 10.0f) spd = BALL_BASE_SPEED;

                spawned.emplace_back(pos, Vec2{vel.x * 0.866f - vel.y * 0.5f, vel.x * 0.5f + vel.y * 0.866f});
                spawned.emplace_back(pos, Vec2{vel.x * 0.866f + vel.y * 0.5f, -vel.x * 0.5f + vel.y * 0.866f});
            }
            for (auto& s : spawned) {
                if (m_balls.size() < 12) {
                    m_balls.push_back(s);
                }
            }
            break;
        }

        case PowerUpType::Fireball:
            for (auto& b : m_balls) {
                b.setFireball(true, 9.0f);
            }
            break;

        case PowerUpType::SlowBall:
            for (auto& b : m_balls) {
                b.setSlow(true, 0.65f);
            }
            break;

        case PowerUpType::Shield:
            m_hasShield = true;
            break;

        case PowerUpType::ExtraLife:
            m_lives = std::min(m_lives + 1, 9);
            break;

        default:
            break;
    }
}

void GameEngine::updateLasers(float dt) {
    for (auto& laser : m_lasers) {
        laser.update(dt);
        if (!laser.isAlive()) continue;

        AABB lBox = laser.getBounds();
        auto& bricks = m_levelManager.getBricks();
        for (auto& brick : bricks) {
            if (brick.isDestroyed()) continue;
            if (Collision::testAABBAABB(lBox, brick.getBounds())) {
                laser.kill();
                bool destroyed = brick.hit(1);
                onBrickHit(brick, laser.getBounds().center(), destroyed);
                break;
            }
        }
    }

    m_lasers.erase(std::remove_if(m_lasers.begin(), m_lasers.end(),
                                  [](const Laser& l) { return !l.isAlive(); }),
                   m_lasers.end());
}

void GameEngine::updatePowerUps(float dt) {
    AABB pBox = m_paddle.getBounds();

    for (auto& pup : m_powerUps) {
        pup.update(dt);
        if (!pup.isAlive()) continue;

        if (Collision::testAABBAABB(pup.getBounds(), pBox)) {
            activatePowerUp(pup.getType());
            pup.kill();
        }
    }

    m_powerUps.erase(std::remove_if(m_powerUps.begin(), m_powerUps.end(),
                                    [](const PowerUp& p) { return !p.isAlive(); }),
                     m_powerUps.end());
}

void GameEngine::handleBallLost() {
    m_lives--;
    m_combo = 0;
    triggerScreenShake(0.25f, 3.5f);
    SoundManager::instance().play(SoundEffect::BallLost);
    m_state = GameState::BallLost;
    m_stateTimer = 1.2f;
}

// ---------------- In-Game Screen Renderers ----------------

void GameEngine::renderMainMenu() {
    // Top marquee border
    m_buffer.drawRect(8, 8, VIRTUAL_WIDTH - 16, VIRTUAL_HEIGHT - 16, Colors::NeonCyan);
    m_buffer.drawRect(10, 10, VIRTUAL_WIDTH - 20, VIRTUAL_HEIGHT - 20, Colors::GridLine);

    // Glowing Retro Arcade Title with 3D drop shadow
    int titleY = 28;
    m_buffer.drawBitmapTextCentered(titleY + 2, "RETRO BREAKOUT", Colors::Black, 2);
    m_buffer.drawBitmapTextCentered(titleY + 1, "RETRO BREAKOUT", Colors::NeonPurple, 2);
    m_buffer.drawBitmapTextCentered(titleY, "RETRO BREAKOUT", Colors::NeonCyan, 2);

    // Subtitle
    m_buffer.drawBitmapTextCentered(50, "-- PURE RASTER GRID ARCADE --", Colors::NeonYellow, 1);

    // High Score badge
    char hiStr[32];
    std::snprintf(hiStr, sizeof(hiStr), "ALL-TIME HIGH: %06d", m_highScore);
    m_buffer.drawBitmapTextCentered(66, hiStr, Colors::GrayLight, 1);

    // Menu options
    const char* options[5] = {
        (!m_balls.empty() && m_levelTime > 0.1f) ? "RESUME GAME" : "START GAME",
        "SELECT STAGE",
        "SETTINGS & OPTIONS",
        "HOW TO PLAY",
        "QUIT GAME"
    };

    for (int i = 0; i < 5; ++i) {
        int itemY = 96 + i * 20;
        bool selected = (m_mainMenuIndex == i);

        if (selected) {
            // Selected highlight bar
            m_buffer.fillRect(40, itemY - 3, VIRTUAL_WIDTH - 80, 14, Colors::GridLine);
            m_buffer.drawRect(40, itemY - 3, VIRTUAL_WIDTH - 80, 14, Colors::NeonGreen);
            m_buffer.drawBitmapText(48, itemY, ">", Colors::NeonYellow, 1);
            m_buffer.drawBitmapTextCentered(itemY, options[i], Colors::White, 1);
        } else {
            m_buffer.drawBitmapTextCentered(itemY, options[i], Colors::GrayLight, 1);
        }
    }

    // Bottom help tip
    m_buffer.drawBitmapTextCentered(216, "UP/DOWN: SELECT   SPACE: CONFIRM", Colors::GrayMid, 1);
}

void GameEngine::renderLevelSelect() {
    m_buffer.drawRect(8, 8, VIRTUAL_WIDTH - 16, VIRTUAL_HEIGHT - 16, Colors::NeonPurple);
    m_buffer.drawBitmapTextCentered(18, "STAGE SELECT", Colors::NeonYellow, 2);

    int total = m_levelManager.getTotalLevels();
    char stageTitle[64];
    std::snprintf(stageTitle, sizeof(stageTitle), "< STAGE %d OF %d >", m_levelSelectIndex + 1, total);
    m_buffer.drawBitmapTextCentered(44, stageTitle, Colors::NeonCyan, 1);

    LevelData previewLevel;
    bool hasPreview = m_levelManager.peekLevel(m_levelSelectIndex, previewLevel);
    std::string displayName = (hasPreview && !previewLevel.name.empty()) ? previewLevel.name : ("STAGE " + std::to_string(m_levelSelectIndex + 1));
    m_buffer.drawBitmapTextCentered(58, displayName, Colors::White, 1);

    // Render a mini preview map of the selected stage in center
    int previewBoxX = 50;
    int previewBoxY = 74;
    int previewBoxW = 220;
    int previewBoxH = 104;
    m_buffer.fillRect(previewBoxX, previewBoxY, previewBoxW, previewBoxH, Colors::Black);
    m_buffer.drawRect(previewBoxX, previewBoxY, previewBoxW, previewBoxH, Colors::BorderWall);

    if (hasPreview && !previewLevel.bricks.empty()) {
        float scaleX = static_cast<float>(previewBoxW - 20) / static_cast<float>(PLAYFIELD_RIGHT - PLAYFIELD_LEFT);
        float scaleY = static_cast<float>(previewBoxH - 30) / 120.0f;
        float baseOffsetX = static_cast<float>(previewBoxX + 10);
        float baseOffsetY = static_cast<float>(previewBoxY + 10);

        for (const auto& b : previewLevel.bricks) {
            int bx = static_cast<int>(baseOffsetX + (b.getBounds().min.x - PLAYFIELD_LEFT) * scaleX);
            int by = static_cast<int>(baseOffsetY + (b.getBounds().min.y - PLAYFIELD_TOP) * scaleY);
            int bw = std::max(2, static_cast<int>(b.getBounds().width() * scaleX) - 1);
            int bh = std::max(2, static_cast<int>(b.getBounds().height() * scaleY) - 1);
            m_buffer.fillRect(bx, by, bw, bh, b.getColor());
        }
    }

    // Mini paddle at bottom of preview
    m_buffer.fillRect(previewBoxX + previewBoxW / 2 - 16, previewBoxY + previewBoxH - 10, 32, 4, Colors::NeonCyan);

    // Instructions
    m_buffer.drawBitmapTextCentered(190, "[ SPACE / ENTER: LAUNCH STAGE ]", Colors::NeonGreen, 1);
    m_buffer.drawBitmapTextCentered(210, "LEFT/RIGHT: STAGE   ESC: BACK", Colors::GrayMid, 1);
}

void GameEngine::renderOptionsMenu() {
    m_buffer.drawRect(8, 8, VIRTUAL_WIDTH - 16, VIRTUAL_HEIGHT - 16, Colors::NeonGreen);
    m_buffer.drawBitmapTextCentered(15, "GAME SETTINGS & OPTIONS", Colors::NeonYellow, 1);
    m_buffer.drawFastHLine(20, VIRTUAL_WIDTH - 20, 26, Colors::GridLine);

    char speedBuf[32];
    std::snprintf(speedBuf, sizeof(speedBuf), "< %.2fx >", m_difficultyMultiplier);

    char livesBuf[32];
    std::snprintf(livesBuf, sizeof(livesBuf), "< %d LIVES >", m_startingLives);

    const char* crtNames[3] = {"< OFF (RAW) >", "< SUBTLE >", "< ARCADE CRT >"};
    const char* crtStr = crtNames[std::clamp(static_cast<int>(m_crtMode), 0, 2)];

    const char* themeNames[6] = {
        "< NEON ARCADE >",
        "< DARK OLED >",
        "< GRUVBOX >",
        "< NEOVIM >",
        "< GAME BOY >",
        "< AMBER CRT >"
    };
    const char* themeStr = themeNames[std::clamp(static_cast<int>(m_paletteTheme), 0, 5)];

    const char* paddleNames[4] = {"< SKATEBOARD >", "< CLASSIC >", "< CYBER HOVER >", "< RETRO WOOD >"};
    const char* paddleStr = paddleNames[std::clamp(static_cast<int>(m_paddleSkin), 0, 3)];

    const char* ballNames[4] = {"< ENERGY ORB >", "< PLASMA CORE >", "< NEON DIAMOND >", "< CYBER CUBE >"};
    const char* ballStr = ballNames[std::clamp(static_cast<int>(m_ballSkin), 0, 3)];

    char audioBuf[32];
    if (SoundManager::instance().isMuted()) {
        std::snprintf(audioBuf, sizeof(audioBuf), "< MUTED >");
    } else {
        std::snprintf(audioBuf, sizeof(audioBuf), "< %d%% >", static_cast<int>(SoundManager::instance().getVolume() * 100.0f));
    }

    const char* mouseStr = m_mouseControlEnabled ? "< ENABLED >" : "< DISABLED >";

    struct OptionItem {
        const char* label;
        const char* value;
    };

    OptionItem items[9] = {
        {"BALL SPEED", speedBuf},
        {"START LIVES", livesBuf},
        {"CRT SCANLINES", crtStr},
        {"COLOR PALETTE", themeStr},
        {"BOARD PRESET", paddleStr},
        {"BALL PRESET", ballStr},
        {"AUDIO VOLUME", audioBuf},
        {"MOUSE PADDLE", mouseStr},
        {"[ BACK TO MENU ]", ""}
    };

    for (int i = 0; i < 9; ++i) {
        int itemY = 36 + i * 19;
        bool selected = (m_optionsMenuIndex == i);

        if (selected) {
            m_buffer.fillRect(18, itemY - 3, VIRTUAL_WIDTH - 36, 14, Colors::GridLine);
            m_buffer.drawRect(18, itemY - 3, VIRTUAL_WIDTH - 36, 14, Colors::NeonCyan);
            m_buffer.drawBitmapText(24, itemY, ">", Colors::NeonYellow, 1);
        }

        if (i == 8) {
            m_buffer.drawBitmapTextCentered(itemY, items[i].label, selected ? Colors::NeonYellow : Colors::White, 1);
        } else {
            m_buffer.drawBitmapText(36, itemY, items[i].label, selected ? Colors::White : Colors::GrayLight, 1);
            m_buffer.drawBitmapText(174, itemY, items[i].value, selected ? Colors::NeonCyan : Colors::GrayMid, 1);
        }
    }

    m_buffer.drawBitmapTextCentered(214, "LEFT/RIGHT: CHANGE   ESC: BACK", Colors::GrayMid, 1);
}

void GameEngine::renderHelpMenu() {
    m_buffer.drawRect(8, 8, VIRTUAL_WIDTH - 16, VIRTUAL_HEIGHT - 16, Colors::NeonCyan);
    m_buffer.drawBitmapTextCentered(14, "HOW TO PLAY & POWER-UPS", Colors::NeonYellow, 1);
    m_buffer.drawFastHLine(20, VIRTUAL_WIDTH - 20, 24, Colors::GridLine);

    m_buffer.drawBitmapText(18, 28, "CONTROLS:", Colors::NeonCyan, 1);
    m_buffer.drawBitmapText(18, 38, "LEFT/RIGHT OR A/D: MOVE PADDLE", Colors::White, 1);
    m_buffer.drawBitmapText(18, 48, "SPACE: LAUNCH BALL / FIRE LASERS", Colors::White, 1);
    m_buffer.drawBitmapText(18, 58, "P OR ESC: PAUSE / IN-GAME MENU", Colors::White, 1);
    m_buffer.drawBitmapText(18, 68, "MOUSE: DIRECT GLIDE (CONFIGURABLE)", Colors::GrayLight, 1);

    m_buffer.drawBitmapText(18, 82, "POWER-UP CAPSULES:", Colors::NeonYellow, 1);

    struct PowerUpHelp {
        char icon;
        uint32_t color;
        const char* desc;
    };
    PowerUpHelp items[9] = {
        {'E', Colors::NeonGreen,  "ELONGATE: EXTENDS PADDLE"},
        {'S', Colors::NeonPink,   "SHRINK: NARROWS PADDLE (RISK)"},
        {'M', Colors::NeonCyan,   "MULTI-BALL: 3-BALL SPLIT"},
        {'F', Colors::NeonOrange, "FIREBALL: PIERCES BRICKS"},
        {'L', Colors::NeonYellow, "LASERS: PRESS SPACE TO SHOOT"},
        {'C', Colors::NeonPurple, "CATCH: STICKY PADDLE"},
        {'Z', Colors::NeonBlue,   "SLOW-MO: REDUCES BALL SPEED"},
        {'B', Colors::BrickCyan,  "SHIELD: BOTTOM SAFETY BARRIER"},
        {'+', Colors::BrickRed,   "+1 LIFE: EXTRA CHANCE"}
    };

    for (int i = 0; i < 9; ++i) {
        int py = 95 + i * 12;
        // Pill icon (14x8)
        m_buffer.fillRect(18, py, 14, 8, items[i].color);
        char iconStr[2] = {items[i].icon, '\0'};
        m_buffer.drawBitmapText(21, py, iconStr, Colors::Black, 1);
        m_buffer.drawBitmapText(36, py, items[i].desc, Colors::GrayLight, 1);
    }

    m_buffer.drawBitmapTextCentered(216, "PRESS SPACE OR ESCAPE TO RETURN", Colors::NeonCyan, 1);
}

void GameEngine::renderAboutMenu() {
    m_buffer.drawRect(8, 8, VIRTUAL_WIDTH - 16, VIRTUAL_HEIGHT - 16, Colors::NeonCyan);
    m_buffer.drawBitmapTextCentered(16, "ABOUT RETRO BREAKOUT", Colors::NeonYellow, 1);
    m_buffer.drawFastHLine(20, VIRTUAL_WIDTH - 20, 26, Colors::GridLine);

    m_buffer.drawBitmapText(18, 34, "CROSS-PLATFORM RETRO ARCADE", Colors::White, 1);
    m_buffer.drawBitmapText(18, 48, "C++20 & QT 6 (CORE/GUI/WIDGETS/MULTIMEDIA)", Colors::GrayMid, 1);

    m_buffer.drawBitmapText(18, 68, "ENGINE ARCHITECTURE:", Colors::NeonCyan, 1);
    m_buffer.drawBitmapText(18, 82, "- PURE SOFTWARE RASTER (320X240 ARGB)", Colors::GrayLight, 1);
    m_buffer.drawBitmapText(18, 96, "- 4X SUB-STEPPED CCD PHYSICS ENGINE", Colors::GrayLight, 1);
    m_buffer.drawBitmapText(18, 110, "- 44.1 KHZ ADDITIVE HARMONIC AUDIO", Colors::GrayLight, 1);
    m_buffer.drawBitmapText(18, 124, "- REAL-TIME SCANLINE & PALETTE FILTERS", Colors::GrayLight, 1);
    m_buffer.drawBitmapText(18, 138, "- WINDOWS, MACOS & LINUX NATIVE", Colors::NeonGreen, 1);

    m_buffer.drawBitmapText(18, 158, "CONTROLS & SHORTCUTS:", Colors::NeonYellow, 1);
    m_buffer.drawBitmapText(18, 172, "CTRL+1..4: ZOOM   CTRL+0: FIT   F11: FULLSCREEN", Colors::GrayLight, 1);
    m_buffer.drawBitmapText(18, 186, "P / ESC: PAUSE    R: RESTART    SPACE: FIRE/LAUNCH", Colors::GrayLight, 1);

    m_buffer.drawBitmapTextCentered(216, "PRESS SPACE OR ESCAPE TO RETURN", Colors::NeonCyan, 1);
}

void GameEngine::renderPauseMenu() {
    // Dim the background playfield
    for (int y = PLAYFIELD_TOP; y <= PLAYFIELD_BOTTOM; ++y) {
        for (int x = PLAYFIELD_LEFT; x <= PLAYFIELD_RIGHT; ++x) {
            uint32_t c = m_buffer.getPixel(x, y);
            uint32_t r = ((c >> 16) & 0xFF) / 3;
            uint32_t g = ((c >> 8) & 0xFF) / 3;
            uint32_t b = (c & 0xFF) / 3;
            m_buffer.setPixel(x, y, 0xFF000000 | (r << 16) | (g << 8) | b);
        }
    }

    // Modal popup box
    int boxX = 40;
    int boxY = 40;
    int boxW = VIRTUAL_WIDTH - 80;
    int boxH = 150;
    m_buffer.fillRect(boxX, boxY, boxW, boxH, Colors::Black);
    m_buffer.drawRect(boxX, boxY, boxW, boxH, Colors::NeonCyan);
    m_buffer.drawRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, Colors::GridLine);

    m_buffer.drawBitmapTextCentered(boxY + 12, "-- GAME PAUSED --", Colors::NeonYellow, 1);

    const char* pauseOpts[5] = {
        "RESUME GAME",
        "RESTART STAGE",
        "SETTINGS & OPTIONS",
        "SELECT STAGE",
        "MAIN MENU"
    };

    for (int i = 0; i < 5; ++i) {
        int itemY = boxY + 36 + i * 20;
        bool selected = (m_pauseMenuIndex == i);

        if (selected) {
            m_buffer.fillRect(boxX + 16, itemY - 3, boxW - 32, 14, Colors::GridLine);
            m_buffer.drawRect(boxX + 16, itemY - 3, boxW - 32, 14, Colors::NeonGreen);
            m_buffer.drawBitmapText(boxX + 22, itemY, ">", Colors::NeonYellow, 1);
            m_buffer.drawBitmapTextCentered(itemY, pauseOpts[i], Colors::White, 1);
        } else {
            m_buffer.drawBitmapTextCentered(itemY, pauseOpts[i], Colors::GrayLight, 1);
        }
    }
}

void GameEngine::renderHUD() {
    m_buffer.drawFastHLine(0, VIRTUAL_WIDTH - 1, PLAYFIELD_TOP - 1, Colors::GridLine);

    char scoreStr[32];
    std::snprintf(scoreStr, sizeof(scoreStr), "SCORE %06d", m_score);
    m_buffer.drawBitmapText(8, 5, scoreStr, Colors::NeonCyan, 1);

    char hiStr[32];
    std::snprintf(hiStr, sizeof(hiStr), "HI %06d", m_highScore);
    m_buffer.drawBitmapText(124, 5, hiStr, Colors::NeonYellow, 1);

    int totalSec = static_cast<int>(m_levelTime);
    int mins = totalSec / 60;
    int secs = totalSec % 60;
    char timeStr[16];
    std::snprintf(timeStr, sizeof(timeStr), "%02d:%02d", mins, secs);
    m_buffer.drawBitmapText(216, 5, timeStr, Colors::White, 1);

    for (int i = 0; i < m_lives; ++i) {
        int lx = VIRTUAL_WIDTH - 12 - i * 14;
        m_buffer.fillRect(lx, 6, 10, 4, Colors::NeonCyan);
        m_buffer.drawFastHLine(lx + 1, lx + 8, 6, Colors::White);
    }

    if (m_paddle.hasLaser()) {
        m_buffer.fillRect(8, VIRTUAL_HEIGHT - 6, 28, 4, Colors::NeonYellow);
        m_buffer.drawBitmapText(8, VIRTUAL_HEIGHT - 6, "LASER", Colors::Black, 1);
    }
    if (m_paddle.getWidthTimeRemaining() > 0.0f) {
        m_buffer.fillRect(40, VIRTUAL_HEIGHT - 6, 26, 4, Colors::NeonGreen);
        m_buffer.drawBitmapText(40, VIRTUAL_HEIGHT - 6, "WIDE", Colors::Black, 1);
    }
}

void GameEngine::renderOverlays() {
    if (m_state == GameState::Ready) {
        m_buffer.drawBitmapTextCentered(150, "PRESS SPACE TO LAUNCH", Colors::NeonYellow, 1);
        m_buffer.drawBitmapTextCentered(165, "A / D OR ARROWS TO MOVE", Colors::GrayLight, 1);
    } else if (m_state == GameState::GameOver) {
        m_buffer.fillRect(30, 80, VIRTUAL_WIDTH - 60, 70, Colors::Black);
        m_buffer.drawRect(30, 80, VIRTUAL_WIDTH - 60, 70, Colors::NeonPink);
        m_buffer.drawBitmapTextCentered(95, "GAME OVER", Colors::NeonPink, 2);
        m_buffer.drawBitmapTextCentered(122, "PRESS SPACE FOR MENU", Colors::White, 1);
    } else if (m_state == GameState::LevelWon) {
        m_buffer.fillRect(30, 80, VIRTUAL_WIDTH - 60, 70, Colors::Black);
        m_buffer.drawRect(30, 80, VIRTUAL_WIDTH - 60, 70, Colors::NeonGreen);
        m_buffer.drawBitmapTextCentered(95, "STAGE CLEARED!", Colors::NeonGreen, 1);
        m_buffer.drawBitmapTextCentered(115, "PRESS SPACE FOR NEXT STAGE", Colors::White, 1);
    } else if (m_state == GameState::GameCompleted) {
        m_buffer.fillRect(20, 75, VIRTUAL_WIDTH - 40, 80, Colors::Black);
        m_buffer.drawRect(20, 75, VIRTUAL_WIDTH - 40, 80, Colors::NeonYellow);
        m_buffer.drawBitmapTextCentered(90, "CONGRATULATIONS!", Colors::NeonYellow, 1);
        m_buffer.drawBitmapTextCentered(108, "YOU BEAT ALL STAGES!", Colors::White, 1);
        m_buffer.drawBitmapTextCentered(128, "PRESS SPACE FOR MENU", Colors::NeonCyan, 1);
    }
}

void GameEngine::render() {
    m_buffer.clear(Colors::DarkBg);

    if (m_state == GameState::MainMenu) {
        renderMainMenu();
        m_buffer.applyScanlineFilter(m_crtMode);
        m_buffer.applyThemeFilter(m_paletteTheme);
        return;
    }

    if (m_state == GameState::LevelSelect) {
        renderLevelSelect();
        m_buffer.applyScanlineFilter(m_crtMode);
        m_buffer.applyThemeFilter(m_paletteTheme);
        return;
    }

    if (m_state == GameState::OptionsMenu) {
        renderOptionsMenu();
        m_buffer.applyScanlineFilter(m_crtMode);
        m_buffer.applyThemeFilter(m_paletteTheme);
        return;
    }

    if (m_state == GameState::HelpMenu) {
        renderHelpMenu();
        m_buffer.applyScanlineFilter(m_crtMode);
        m_buffer.applyThemeFilter(m_paletteTheme);
        return;
    }

    if (m_state == GameState::AboutMenu) {
        renderAboutMenu();
        m_buffer.applyScanlineFilter(m_crtMode);
        m_buffer.applyThemeFilter(m_paletteTheme);
        return;
    }

    // Active gameplay rendering
    m_buffer.drawFastVLine(PLAYFIELD_LEFT - 1, PLAYFIELD_TOP, PLAYFIELD_BOTTOM, Colors::BorderWall);
    m_buffer.drawFastVLine(PLAYFIELD_LEFT - 2, PLAYFIELD_TOP, PLAYFIELD_BOTTOM, Colors::BorderGlow);
    m_buffer.drawFastVLine(PLAYFIELD_RIGHT + 1, PLAYFIELD_TOP, PLAYFIELD_BOTTOM, Colors::BorderWall);
    m_buffer.drawFastVLine(PLAYFIELD_RIGHT + 2, PLAYFIELD_TOP, PLAYFIELD_BOTTOM, Colors::BorderGlow);

    if (m_hasShield) {
        m_buffer.drawFastHLine(PLAYFIELD_LEFT, PLAYFIELD_RIGHT, PLAYFIELD_BOTTOM, Colors::NeonCyan);
        m_buffer.drawFastHLine(PLAYFIELD_LEFT, PLAYFIELD_RIGHT, PLAYFIELD_BOTTOM - 1, Colors::White);
    }

    for (const auto& brick : m_levelManager.getBricks()) {
        brick.render(m_buffer);
    }

    for (const auto& pup : m_powerUps) {
        pup.render(m_buffer);
    }

    for (const auto& laser : m_lasers) {
        laser.render(m_buffer);
    }

    m_paddle.render(m_buffer);

    for (const auto& ball : m_balls) {
        ball.render(m_buffer);
    }

    m_particles.render(m_buffer);
    renderFloatingTexts();

    renderHUD();
    renderOverlays();

    if (m_state == GameState::Paused) {
        renderPauseMenu();
    }

    // Apply retro screen shake if active
    if (m_shakeTimer > 0.0f) {
        std::uniform_real_distribution<float> shakeDist(-m_shakeIntensity, m_shakeIntensity);
        int sx = static_cast<int>(std::round(shakeDist(getEngineRng())));
        int sy = static_cast<int>(std::round(shakeDist(getEngineRng())));
        m_buffer.applyScreenShake(sx, sy);
    }

    m_buffer.applyScanlineFilter(m_crtMode);
    m_buffer.applyThemeFilter(m_paletteTheme);
}

} // namespace Breakout
