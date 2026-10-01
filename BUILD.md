# Build & Setup Guide

This guide covers building, configuring, and developing **Retro Breakout** across **macOS**, **Linux**, and **Windows**, including setup instructions for Neovim, VS Code, CLion, and Qt Creator.

---

## System Requirements

- **C++ Compiler**: Modern C++20-compliant compiler
  - macOS: Apple Clang 15+ (via `xcode-select --install`)
  - Linux: GCC 12+ or Clang 15+
  - Windows: MSVC 2022 (v143) or Clang 15+
- **Build System**: CMake 3.19 or higher
- **Framework**: Qt 6 (Components: `Core`, `Gui`, `Widgets`, `Multimedia`)

---

## Installing Dependencies

### macOS (Homebrew)
```bash
brew install cmake qt@6
```
If CMake does not locate Qt automatically, specify:
```bash
export CMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
```

### Linux

#### Ubuntu / Debian (22.04+)
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev libgl1-mesa-dev
```

#### Fedora (38+)
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtmultimedia-devel
```

#### Arch Linux
```bash
sudo pacman -S base-devel cmake qt6-base qt6-multimedia
```

### Windows (vcpkg)
```powershell
vcpkg install qtbase:x64-windows qtmultimedia:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
```

---

## Building the Project

```bash
# 1. Configure the project and generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 2. Compile with parallel worker threads
cmake --build build -j
```

### Running the Executable

- **macOS**:
  ```bash
  ./build/Breakout-game.app/Contents/MacOS/Breakout-game
  ```
- **Linux & Windows**:
  ```bash
  ./build/Breakout-game
  ```

---

## Headless Demo & Screenshot Generation

The application includes an internal generator for producing promotional screenshots and animated demos:

```bash
./build/Breakout-game.app/Contents/MacOS/Breakout-game --generate-demos
```

This runs headless, renders each stage and menu state, and saves output assets directly into `assets/screenshots/` and `assets/demos/`.

---

## IDE & Editor Integration

The build automatically exports `build/compile_commands.json`.

### Neovim (`nvim-lspconfig` + `clangd`)
The project root contains a pre-configured [`.clangd`](.clangd) file referencing `build/compile_commands.json`:
```yaml
CompileFlags:
  CompilationDatabase: "build"
  Add: [-std=c++20, -Wall, -Wextra]
```
1. Run `cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` once.
2. Open files in Neovim (`nvim src/main.cpp`). Language server diagnostics and autocomplete will initialize automatically.

### Visual Studio Code
1. Install the **CMake Tools** and **clangd** (or **C/C++**) extensions.
2. Open the project root folder.
3. Select your compiler kit when prompted and press `F5` to build and debug.

### JetBrains CLion
1. Open the project root folder in CLion.
2. CLion detects `CMakeLists.txt` automatically.
3. Configure the Qt 6 prefix path under **Settings > Build, Execution, Deployment > CMake** if needed.
4. Click **Run** (`Shift + F10`).

### Qt Creator
1. Select **File > Open File or Project...** and select `CMakeLists.txt`.
2. Choose your Qt 6 kit.
3. Press `Ctrl + R` (`Cmd + R` on macOS) to build and run.
