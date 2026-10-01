cmake_minimum_required(VERSION 3.19)
project(Breakout-game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/core/Config.h
    src/core/GameEngine.h
    src/core/GameEngine.cpp
    src/physics/Vec2.h
    src/physics/Collision.h
    src/physics/Collision.cpp
    src/raster/RasterBuffer.h
    src/raster/RasterBuffer.cpp
    src/raster/BitmapFont.h
    src/raster/BitmapFont.cpp
    src/entities/Brick.h
    src/entities/Brick.cpp
    src/entities/Paddle.h
    src/entities/Paddle.cpp
    src/entities/Ball.h
    src/entities/Ball.cpp
    src/ui/RasterWidget.h
    src/ui/RasterWidget.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

qt_add_executable(Breakout-game
    WIN32 MACOSX_BUNDLE
    ${SOURCES}
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
