# Building & Developing Retro Breakout

This document provides complete instructions for compiling, running, and configuring your development environment for **Retro Breakout** on **macOS**, **Linux**, and **Windows**, across all modern IDEs and editors.

---

## 1. Prerequisites

- **C++ Compiler**: Modern compiler supporting **C++20**
  - **macOS**: Apple Clang 15+ (Xcode Command Line Tools: `xcode-select --install`)
  - **Linux**: GCC 12+ or Clang 15+
  - **Windows**: MSVC 2022 (v143) or Clang 15+
- **Build System**: **CMake 3.19** or higher
- **Framework**: **Qt 6** (Qt6 `Core`, `Gui`, `Widgets`, and optional `Multimedia`)

---

## 2. Installing Dependencies

### macOS (Homebrew)
```bash
brew install cmake qt@6
```
*(Optional) If `qt@6` is not automatically in your path:*
```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
```

### Linux (Ubuntu / Debian 22.04+)
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev libgl1-mesa-dev
```

### Linux (Fedora 38+)
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel
```

### Linux (Arch Linux)
```bash
sudo pacman -S base-devel cmake qt6-base qt6-multimedia
```

### Windows (vcpkg or Official Qt Installer)
Using [vcpkg](https://vcpkg.io/):
```powershell
vcpkg install qtbase:x64-windows qtmultimedia:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
```

---

## 3. Quick Build Commands

### Standard Build (macOS / Linux / Windows)
```bash
# 1. Configure CMake with compile_commands export
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 2. Build executable using all available CPU cores
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
```

### Launching the Game

- **macOS**:
  ```bash
  ./build/Breakout-game.app/Contents/MacOS/Breakout-game
  ```
- **Linux & Windows**:
  ```bash
  ./build/Breakout-game
  ```

---

## 4. Command-Line Options & Demo Generation

Retro Breakout includes an internal headless screenshot generator for capturing crisp, pixel-perfect promotional shots:

```bash
# Generate promotional screenshots into assets/screenshots/
./build/Breakout-game.app/Contents/MacOS/Breakout-game --generate-demos
```

This runs headless, simulates gameplay states, and saves 640×480 nearest-neighbor scaled PNGs for menus, stages, and color palettes.

---

## 5. Multi-IDE & LSP Setup

The project exports `compile_commands.json` into the `build/` directory for seamless multi-IDE language server support.

### Neovim (`nvim-lspconfig` + `clangd`)
The repository includes a root [`.clangd`](.clangd) file:
```yaml
CompileFlags:
  CompilationDatabase: "build"
  Add: [-std=c++20, -Wall, -Wextra]
```
1. Run `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` once.
2. Open Neovim in the project root: `nvim src/main.cpp`.
3. `clangd` immediately discovers all include paths and Qt 6 symbols with **zero diagnostic errors**.

### VS Code
1. Install extensions: **CMake Tools** (`ms-vscode.cmake-tools`) and **clangd** (`llvm-vs-code-extensions.vscode-clangd`).
2. Open the project folder.
3. When prompted, select your C++20 compiler kit.
4. Press `F5` to build and debug.

### Qt Creator
1. Select **File > Open File or Project...** and choose the root `CMakeLists.txt`.
2. Select your Qt 6 kit.
3. Press `Ctrl + R` (`Cmd + R` on macOS) to build and run.

### JetBrains CLion
1. Open the project root in CLion.
2. CLion detects `CMakeLists.txt` automatically.
3. In **Settings > Build, Execution, Deployment > CMake**, ensure your Qt 6 toolchain is selected.
4. Click **Run** or press `Shift + F10`.

---

## 6. Project Architecture Reference

```text
Breakout-game/
├── CMakeLists.txt              # Modern C++20 build with compile_commands export
├── .clangd                     # Compilation database config for Neovim/LSPs
├── .editorconfig               # Formatting rules (4 spaces, LF, trim whitespace)
├── README.md                   # Visual guide, gameplay, controls, and features
├── BUILD.md                    # This build and developer documentation
├── assets/
│   ├── levels/                 # JSON level declarations
│   └── screenshots/            # Gameplay and menu previews
└── src/
    ├── main.cpp                # Application entrypoint & CLI demo generator
    ├── core/
    │   ├── Config.h            # Virtual resolution, colors, GameState, themes
    │   ├── GameEngine.h/cpp    # State machine, menus, sub-step physics loop
    │   └── LevelManager.h/cpp  # JSON level loader & embedded stage fallbacks
    ├── physics/
    │   ├── Vec2.h              # 2D float vector math
    │   └── Collision.h/cpp     # Swept AABB & circle-AABB penetration resolution
    ├── raster/
    │   ├── RasterBuffer.h/cpp  # 320x240 ARGB buffer, scanlines, palette filters
    │   ├── BitmapFont.h/cpp    # 8x8 retro monochrome font table (ASCII 32-126)
    │   └── ParticleSystem.h/cpp# Software pixel sparks, debris, and explosions
    ├── entities/
    │   ├── Paddle.h/cpp        # Paddle movement, width buffs, laser guns
    │   ├── Ball.h/cpp          # Ball physics, angle deflection, fireball, trail
    │   ├── Brick.h/cpp         # Health, damage crack stages, bevel rendering
    │   ├── PowerUp.h/cpp       # Falling capsule items & duration timers
    │   └── Laser.h/cpp         # Twin blaster projectiles
    ├── audio/
    │   └── SoundManager.h/cpp  # Crystalline piano ting synthesizer & audio playback
    └── ui/
        ├── MainWindow.h/cpp    # Native menubar, window zoom, fullscreen toggle
        └── RasterWidget.h/cpp  # Viewport centering & nearest-neighbor pixel scaler
```
