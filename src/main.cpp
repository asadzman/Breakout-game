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

    // 3. Options Menu (Neovim Palette)
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

    // 7. Animated Gameplay Demo (GIF & MP4)
    {
        std::cout << "[Demo Generator] Recording animated gameplay clip..." << std::endl;
        QDir().mkpath("assets/demos");
        QDir().mkpath("scratch/gif_frames");

        Breakout::GameEngine engine;
        engine.setPaddleSkin(Breakout::PaddleSkin::Skateboard);
        engine.startNewGame();
        engine.actionLaunchOrShoot();

        const int totalFrames = 180; // 6 seconds at 30 FPS
        for (int frame = 0; frame < totalFrames; ++frame) {
            if (!engine.getBalls().empty()) {
                float ballX = engine.getBalls()[0].getPosition().x;
                // Add slight offset for dynamic deflection angle
                float offset = std::sin(frame * 0.1f) * 8.0f;
                engine.setMouseTargetX(ballX + offset);
            }
            engine.update(1.0f / 30.0f);
            engine.render();

            char framePath[128];
            std::snprintf(framePath, sizeof(framePath), "scratch/gif_frames/frame_%04d.png", frame);
            QImage scaled = engine.getBuffer().qimage().scaled(640, 480, Qt::IgnoreAspectRatio, Qt::FastTransformation);
            scaled.save(framePath);
        }

        std::cout << "[Demo Generator] Compiling animated GIF via ffmpeg..." << std::endl;
        int resGif = std::system(
            "ffmpeg -y -framerate 30 -i scratch/gif_frames/frame_%04d.png "
            "-vf \"fps=30,scale=640:-1:flags=neighbor,split[s0][s1];[s0]palettegen=max_colors=64[p];[s1][p]paletteuse=dither=bayer:bayer_scale=1\" "
            "assets/demos/gameplay.gif > /dev/null 2>&1"
        );
        (void)resGif;

        int resMp4 = std::system(
            "ffmpeg -y -framerate 30 -i scratch/gif_frames/frame_%04d.png "
            "-vf \"scale=640:-1:flags=neighbor\" -c:v libx264 -pix_fmt yuv420p "
            "assets/demos/gameplay.mp4 > /dev/null 2>&1"
        );
        (void)resMp4;

        // Cleanup temporary frames
        QDir("scratch/gif_frames").removeRecursively();
        std::cout << "  -> Saved: assets/demos/gameplay.gif" << std::endl;
        std::cout << "  -> Saved: assets/demos/gameplay.mp4" << std::endl;
    }

    std::cout << "[Demo Generator] Successfully generated all screenshots and demos!" << std::endl;
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
