#pragma once

#include <QMainWindow>

namespace Breakout {

class RasterWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

public slots:
    void setWindowZoom(int scale);
    void toggleProportionalFit();
    void openInGameOptions();
    void openInGameLevelSelect();
    void showInstructions();
    void showAbout();

private:
    void setupMenus();

    RasterWidget* m_rasterWidget = nullptr;
};

} // namespace Breakout
