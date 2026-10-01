#include "ui/MainWindow.h"
#include "core/GameEngine.h"
#include <QApplication>
#include <QDir>
#include <iostream>
#include <string>

static void generateDemoScreenshots() {
    QDir().mkpath("assets/screenshots");

    std::cout << "[Demo Generator] Capturing arcade screenshots..." << std::endl;

    auto saveFrame = [](Breakout::GameEngine& engine, const QString& filename) {
        engine.render();
        QImage scaled = engine.getBuffer().qimage().scaled(640, 480, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        scaled.save(filename);
        std::cout << "  -> Saved: " << filename.toStdString() << std::endl;
    };

    // 1. Main Menu
    {
        Breakout::GameEngine engine;
        saveFrame(engine, "assets/screenshots/demo_main_menu.png");
    }

    // 2. Stage Select
    {
        Breakout::GameEngine engine;
        engine.menuDown();
        engine.menuConfirm();
        saveFrame(engine, "assets/screenshots/demo_stage_select.png");
    }

    // 3. Options Menu (Quack / Neovim Palette)
    {
        Breakout::GameEngine engine;
        engine.menuDown();
        engine.menuDown();
        engine.menuConfirm();
        engine.setPaletteTheme(Breakout::PaletteTheme::NeovimDefault);
        saveFrame(engine, "assets/screenshots/demo_options_menu.png");
    }

    // 4. Gameplay - Neon Arcade (Stage 1)
    {
        Breakout::GameEngine engine;
        engine.startNewGame();
        engine.actionLaunchOrShoot();
        for (int i = 0; i < 45; ++i) engine.update(1.0f / 60.0f);
        engine.setPaletteTheme(Breakout::PaletteTheme::NeonArcade);
        saveFrame(engine, "assets/screenshots/demo_gameplay_neon.png");
    }

    // 5. Gameplay - Quack / Neovim Palette (Stage 2: Armored Fortress)
    {
        Breakout::GameEngine engine;
        engine.selectAndStartLevel(1);
        engine.actionLaunchOrShoot();
        for (int i = 0; i < 60; ++i) engine.update(1.0f / 60.0f);
        engine.setPaletteTheme(Breakout::PaletteTheme::NeovimDefault);
        saveFrame(engine, "assets/screenshots/demo_gameplay_nvim.png");
    }

    // 6. Gameplay - Gruvbox Palette (Stage 3: Explosive Minefield)
    {
        Breakout::GameEngine engine;
        engine.selectAndStartLevel(2);
        engine.actionLaunchOrShoot();
        for (int i = 0; i < 50; ++i) engine.update(1.0f / 60.0f);
        engine.setPaletteTheme(Breakout::PaletteTheme::Gruvbox);
        saveFrame(engine, "assets/screenshots/demo_gameplay_gruvbox.png");
    }

    std::cout << "[Demo Generator] Successfully generated all screenshots!" << std::endl;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("RetroGames");
    app.setApplicationName("BreakoutGame");

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--generate-demos" || std::string(argv[i]) == "--screenshots") {
            generateDemoScreenshots();
            return 0;
        }
    }

    Breakout::MainWindow window;
    window.show();

    return app.exec();
}
