#pragma once

#include "core/GameEngine.h"
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QRect>

namespace Breakout {

enum class FitMode {
    IntegerScale, // Largest integer multiple (sharpest pixel grid)
    Proportional  // Smooth proportional 4:3 fill
};

class RasterWidget : public QWidget {
    Q_OBJECT

public:
    explicit RasterWidget(QWidget* parent = nullptr);

    [[nodiscard]] GameEngine& getEngine() { return m_engine; }
    [[nodiscard]] const GameEngine& getEngine() const { return m_engine; }

    void setFitMode(FitMode mode);
    [[nodiscard]] FitMode getFitMode() const { return m_fitMode; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void gameLoopTick();

private:
    [[nodiscard]] QRect calculateViewportRect() const;
    [[nodiscard]] float windowToVirtualX(int windowX) const;
    [[nodiscard]] float windowToVirtualY(int windowY) const;

    GameEngine m_engine;
    QTimer m_timer;
    QElapsedTimer m_elapsedTimer;

    FitMode m_fitMode = FitMode::IntegerScale;
};

} // namespace Breakout
