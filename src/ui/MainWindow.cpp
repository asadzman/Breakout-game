#include "MainWindow.h"
#include "RasterWidget.h"
#include "core/Config.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QKeySequence>

namespace Breakout {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Retro Breakout - Pure Software Raster Arcade");
    m_rasterWidget = new RasterWidget(this);
    setCentralWidget(m_rasterWidget);

    setupMenus();

    // Default window size: 3x integer scale (960x720) + titlebar
    resize(VIRTUAL_WIDTH * 3, VIRTUAL_HEIGHT * 3 + 28);
}

void MainWindow::setWindowZoom(int scale) {
    if (isFullScreen()) {
        showNormal();
    }
    m_rasterWidget->setFitMode(FitMode::IntegerScale);
    int targetW = VIRTUAL_WIDTH * scale;
    int targetH = VIRTUAL_HEIGHT * scale;
    int extraH = menuBar()->isVisible() ? menuBar()->height() : 0;
    resize(targetW, targetH + extraH);
}

void MainWindow::toggleProportionalFit() {
    if (m_rasterWidget->getFitMode() == FitMode::IntegerScale) {
        m_rasterWidget->setFitMode(FitMode::Proportional);
    } else {
        m_rasterWidget->setFitMode(FitMode::IntegerScale);
    }
}

void MainWindow::openInGameOptions() {
    auto& engine = m_rasterWidget->getEngine();
    GameState s = engine.getState();
    if (s != GameState::OptionsMenu) {
        if (s == GameState::Playing) {
            engine.togglePause();
        }
        engine.menuConfirm(); // Enters options menu
    }
}

void MainWindow::openInGameLevelSelect() {
    auto& engine = m_rasterWidget->getEngine();
    GameState s = engine.getState();
    if (s == GameState::Playing) {
        engine.togglePause();
    }
    engine.menuDown();
    engine.menuConfirm();
}

void MainWindow::setupMenus() {
    // 1. Game Menu
    QMenu* gameMenu = menuBar()->addMenu("&Game");

    auto* newGameAct = gameMenu->addAction("&New Game", [this]() {
        m_rasterWidget->getEngine().startNewGame();
    });
    newGameAct->setShortcut(QKeySequence::New);

    auto* levelSelectAct = gameMenu->addAction("&Select Stage...", [this]() {
        openInGameLevelSelect();
    });
    levelSelectAct->setShortcut(QKeySequence("Ctrl+L"));

    auto* restartAct = gameMenu->addAction("&Restart Current Stage", [this]() {
        m_rasterWidget->getEngine().restartCurrentLevel();
    });
    restartAct->setShortcut(QKeySequence(Qt::Key_R));

    auto* pauseAct = gameMenu->addAction("&Pause / In-Game Menu", [this]() {
        m_rasterWidget->getEngine().togglePause();
    });
    pauseAct->setShortcut(QKeySequence(Qt::Key_P));

    gameMenu->addSeparator();

    auto* settingsAct = gameMenu->addAction("&Settings & Options", [this]() {
        openInGameOptions();
    });
    settingsAct->setShortcut(QKeySequence::Preferences);

    gameMenu->addSeparator();

    auto* quitAct = gameMenu->addAction("&Quit", this, &QWidget::close);
    quitAct->setShortcut(QKeySequence::Quit);

    // 2. View Menu (Zoom & Scaler)
    QMenu* viewMenu = menuBar()->addMenu("&View");

    viewMenu->addAction("1x Scale (320x240)", [this]() { setWindowZoom(1); })
        ->setShortcut(QKeySequence("Ctrl+1"));
    viewMenu->addAction("2x Scale (640x480)", [this]() { setWindowZoom(2); })
        ->setShortcut(QKeySequence("Ctrl+2"));
    viewMenu->addAction("3x Scale (960x720)", [this]() { setWindowZoom(3); })
        ->setShortcut(QKeySequence("Ctrl+3"));
    viewMenu->addAction("4x Scale (1280x960)", [this]() { setWindowZoom(4); })
        ->setShortcut(QKeySequence("Ctrl+4"));

    viewMenu->addSeparator();

    auto* fitAct = viewMenu->addAction("Toggle Proportional 4:3 Fit", this, &MainWindow::toggleProportionalFit);
    fitAct->setShortcut(QKeySequence("Ctrl+0"));

    viewMenu->addSeparator();

    auto* fullScreenAct = viewMenu->addAction("&Toggle Fullscreen", [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });
    fullScreenAct->setShortcut(QKeySequence::FullScreen);

    // 3. Help Menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    helpMenu->addAction("&How to Play", this, &MainWindow::showInstructions);
    helpMenu->addAction("&About Retro Breakout", this, &MainWindow::showAbout);
}

void MainWindow::showInstructions() {
    QMessageBox::information(
        this,
        "How to Play",
        "<h3>Retro Breakout Controls & Rules</h3>"
        "<ul>"
        "<li><b>Left / Right Arrows</b> or <b>A / D</b>: Move Paddle</li>"
        "<li><b>Mouse Cursor</b>: Glide paddle smoothly to cursor (configurable in Settings)</li>"
        "<li><b>Spacebar / Enter</b>: Launch Ball / Fire Lasers / Confirm Menu</li>"
        "<li><b>P or Escape</b>: Pause / Seamless In-Game Menu</li>"
        "<li><b>R</b>: Quick restart stage</li>"
        "<li><b>F11</b>: Toggle Fullscreen</li>"
        "</ul>"
        "<h4>Seamless In-Game Menus:</h4>"
        "<p>All settings (speed, lives, CRT scanlines, theme, volume) are seamlessly accessible in-game via the main menu or pause menu!</p>"
    );
}

void MainWindow::showAbout() {
    QMessageBox::about(
        this,
        "About Retro Breakout",
        "<h3>Retro Breakout</h3>"
        "<p>Built with <b>Qt 6</b> and <b>C++20</b>.</p>"
        "<p>Features a <b>pure software raster grid</b> (320x240 internal framebuffer, "
        "no OpenGL, no vector graphics), sub-stepped arcade continuous collision physics, "
        "and seamless in-game retro UI navigation.</p>"
    );
}

} // namespace Breakout
