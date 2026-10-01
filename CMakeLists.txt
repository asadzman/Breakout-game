cmake_minimum_required(VERSION 3.19)
project(Breakout-game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt6 COMPONENTS Multimedia QUIET)

qt_standard_project_setup()

set(GAME_CORE_SOURCES
    src/core/Config.h
    src/core/GameEngine.h
    src/core/GameEngine.cpp
    src/core/LevelManager.h
    src/core/LevelManager.cpp
    src/physics/Vec2.h
    src/physics/Collision.h
    src/physics/Collision.cpp
    src/raster/RasterBuffer.h
    src/raster/RasterBuffer.cpp
    src/raster/BitmapFont.h
    src/raster/BitmapFont.cpp
    src/raster/ParticleSystem.h
    src/raster/ParticleSystem.cpp
    src/entities/Laser.h
    src/entities/Laser.cpp
    src/entities/PowerUp.h
    src/entities/PowerUp.cpp
    src/entities/Brick.h
    src/entities/Brick.cpp
    src/entities/Paddle.h
    src/entities/Paddle.cpp
    src/entities/Ball.h
    src/entities/Ball.cpp
    src/audio/SoundManager.h
    src/audio/SoundManager.cpp
    assets.qrc
)

set(UI_SOURCES
    src/ui/RasterWidget.h
    src/ui/RasterWidget.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

qt_add_executable(Breakout-game
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    ${GAME_CORE_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(Breakout-game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Breakout-game
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

if(TARGET Qt6::Multimedia)
    target_link_libraries(Breakout-game PRIVATE Qt6::Multimedia)
    target_compile_definitions(Breakout-game PRIVATE BREAKOUT_HAS_MULTIMEDIA)
endif()

if(MSVC)
    target_compile_options(Breakout-game PRIVATE /W4 /permissive-)
else()
    target_compile_options(Breakout-game PRIVATE -Wall -Wextra)
endif()

if(APPLE)
    set_target_properties(Breakout-game PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Retro Breakout"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.retroarcade.breakout"
    )
    # Ensure macOS app bundle contains assets in Contents/Resources
    add_custom_command(TARGET Breakout-game POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_BUNDLE_DIR:Breakout-game>/Contents/Resources/assets
    )
endif()

# Copy assets to build directory for easy relative loading
add_custom_target(copy_assets ALL
    ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_dependencies(Breakout-game copy_assets)

# ----------------- Automated Unit Tests -----------------
enable_testing()

add_executable(Breakout-tests
    tests/test_main.cpp
    ${GAME_CORE_SOURCES}
)

target_include_directories(Breakout-tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Breakout-tests
    PRIVATE
        Qt6::Core
        Qt6::Gui
)

if(TARGET Qt6::Multimedia)
    target_link_libraries(Breakout-tests PRIVATE Qt6::Multimedia)
    target_compile_definitions(Breakout-tests PRIVATE BREAKOUT_HAS_MULTIMEDIA)
endif()

if(MSVC)
    target_compile_options(Breakout-tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(Breakout-tests PRIVATE -Wall -Wextra)
endif()

add_dependencies(Breakout-tests copy_assets)
add_test(NAME AllUnitTests COMMAND Breakout-tests)
