#include "RasterWidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <algorithm>

namespace Breakout {

RasterWidget::RasterWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setMinimumSize(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    connect(&m_timer, &QTimer::timeout, this, &RasterWidget::gameLoopTick);
    m_elapsedTimer.start();
    m_timer.start(16); // 60 FPS
}

void RasterWidget::setFitMode(FitMode mode) {
    m_fitMode = mode;
    update();
}

void RasterWidget::gameLoopTick() {
    qint64 elapsedMs = m_elapsedTimer.restart();
    float dt = static_cast<float>(elapsedMs) / 1000.0f;

    m_engine.update(dt);
    update();
}

QRect RasterWidget::calculateViewportRect() const {
    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) {
        return QRect(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    }

    if (m_fitMode == FitMode::Proportional) {
        float targetAspect = static_cast<float>(VIRTUAL_WIDTH) / static_cast<float>(VIRTUAL_HEIGHT);
        int destW = w;
        int destH = static_cast<int>(destW / targetAspect);

        if (destH > h) {
            destH = h;
            destW = static_cast<int>(destH * targetAspect);
        }

        int destX = (w - destW) / 2;
        int destY = (h - destH) / 2;
        return QRect(destX, destY, destW, destH);
    }

    // Integer scaling: finds the largest integer scale that fits inside window
    int scale = std::max(1, std::min(w / VIRTUAL_WIDTH, h / VIRTUAL_HEIGHT));
    int destW = VIRTUAL_WIDTH * scale;
    int destH = VIRTUAL_HEIGHT * scale;

    int destX = (w - destW) / 2;
    int destY = (h - destH) / 2;
    return QRect(destX, destY, destW, destH);
}

float RasterWidget::windowToVirtualX(int windowX) const {
    QRect viewport = calculateViewportRect();
    if (viewport.width() <= 0) return static_cast<float>(VIRTUAL_WIDTH) * 0.5f;

    float rel = static_cast<float>(windowX - viewport.left()) / static_cast<float>(viewport.width());
    return std::clamp(rel * static_cast<float>(VIRTUAL_WIDTH), 0.0f, static_cast<float>(VIRTUAL_WIDTH));
}

float RasterWidget::windowToVirtualY(int windowY) const {
    QRect viewport = calculateViewportRect();
    if (viewport.height() <= 0) return static_cast<float>(VIRTUAL_HEIGHT) * 0.5f;

    float rel = static_cast<float>(windowY - viewport.top()) / static_cast<float>(viewport.height());
    return std::clamp(rel * static_cast<float>(VIRTUAL_HEIGHT), 0.0f, static_cast<float>(VIRTUAL_HEIGHT));
}

void RasterWidget::paintEvent(QPaintEvent* /*event*/) {
    m_engine.render();

    QPainter painter(this);
    // Draw black matte background for letterboxing
    painter.fillRect(rect(), Qt::black);

    // Disable anti-aliasing / smoothing so pixels remain razor sharp
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    QRect targetRect = calculateViewportRect();
    const QImage& img = m_engine.getBuffer().qimage();

    painter.drawImage(targetRect, img, img.rect());
}

void RasterWidget::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;

    GameState state = m_engine.getState();
    bool inMenu = (state == GameState::MainMenu || state == GameState::LevelSelect ||
                   state == GameState::OptionsMenu || state == GameState::HelpMenu ||
                   state == GameState::Paused);

    switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_W:
            if (inMenu) {
                m_engine.menuUp();
            }
            break;

        case Qt::Key_Down:
        case Qt::Key_S:
            if (inMenu) {
                m_engine.menuDown();
            }
            break;

        case Qt::Key_Left:
        case Qt::Key_A:
            if (inMenu) {
                m_engine.menuLeft();
            } else {
                m_engine.setMoveLeft(true);
            }
            break;

        case Qt::Key_Right:
        case Qt::Key_D:
            if (inMenu) {
                m_engine.menuRight();
            } else {
                m_engine.setMoveRight(true);
            }
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (inMenu) {
                m_engine.menuConfirm();
            } else {
                m_engine.actionLaunchOrShoot();
            }
            break;

        case Qt::Key_Space:
            if (inMenu) {
                m_engine.menuConfirm();
            } else {
                m_engine.actionLaunchOrShoot();
            }
            break;

        case Qt::Key_Escape:
            if (inMenu) {
                m_engine.menuBack();
            } else {
                m_engine.togglePause();
            }
            break;

        case Qt::Key_P:
            m_engine.togglePause();
            break;

        case Qt::Key_R:
            if (!inMenu) {
                m_engine.restartCurrentLevel();
            }
            break;

        case Qt::Key_F:
        case Qt::Key_F11:
            if (window()->isFullScreen()) {
                window()->showNormal();
            } else {
                window()->showFullScreen();
            }
            break;

        default:
            QWidget::keyPressEvent(event);
            break;
    }
}

void RasterWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
        case Qt::Key_Left:
        case Qt::Key_A:
            m_engine.setMoveLeft(false);
            break;

        case Qt::Key_Right:
        case Qt::Key_D:
            m_engine.setMoveRight(false);
            break;

        default:
            QWidget::keyReleaseEvent(event);
            break;
    }
}

void RasterWidget::mouseMoveEvent(QMouseEvent* event) {
    float vx = windowToVirtualX(event->pos().x());
    float vy = windowToVirtualY(event->pos().y());
    m_engine.handleMouseMove(vx, vy);
    QWidget::mouseMoveEvent(event);
}

void RasterWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        float vx = windowToVirtualX(event->pos().x());
        float vy = windowToVirtualY(event->pos().y());
        m_engine.handleMouseClick(vx, vy);
    }
    QWidget::mousePressEvent(event);
}

} // namespace Breakout
