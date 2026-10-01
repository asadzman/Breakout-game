#include "ui/MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("RetroGames");
    app.setApplicationName("BreakoutGame");

    Breakout::MainWindow window;
    window.show();

    return app.exec();
}
