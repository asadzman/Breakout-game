# AGENTS.md: Developer & Agent Guidelines for Breakout-game

This document establishes the architecture, constraints, build conventions, and operational rules for any AI coding agent (including Google Antigravity) working on this repository.

---

## 1. Project Overview & Architectural Principles

- **Tech Stack**: **C++20**, **Qt 6** (Core, Gui, Widgets, Multimedia), **CMake 3.19+**.
- **Visual Aesthetic**: Pure retro pixel-art arcade.
- **Core Paradigm**: Model-View-Controller engine with an authentic software raster framebuffer.

### ⚠️ Non-Negotiable Hard Constraints

1. **Pure Software Raster Grid (No OpenGL, No Vector Graphics)**:
   - All visual elements (bricks, paddle, ball, particles, text, menus) MUST be drawn in software into the internal **`320 × 240`** 32-bit ARGB framebuffer (`RasterBuffer`).
   - The buffer is blitted onto the screen in `RasterWidget::paintEvent()` with nearest-neighbor scaling:
     ```cpp
     painter.setRenderHint(QPainter::Antialiasing, false);
     painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
     ```
   - **Never** introduce OpenGL, Vulkan, Metal, or Qt vector drawing (`QPainterPath`, smoothed shapes) for gameplay rendering.

2. **Window Scaling & Geometry Discipline**:
   - `RasterWidget` must **never** call `this->resize(...)`. It has `QSizePolicy::Expanding` in both directions and expands to 100% of the window.
   - Letterboxing is handled by centering the aspect-ratio viewport in `RasterWidget::calculateViewportRect()`.
   - Window resizing is handled exclusively by the top-level `MainWindow::setWindowZoom()`.

3. **Sub-Stepped Physics Integration**:
   - Physics integration runs at 4 sub-steps per frame ($\Delta t / 4$) in `GameEngine::updatePhysicsSubSteps()` to eliminate ball tunneling at high velocities.
   - Ball deflection off the paddle is an analytical function of impact distance from paddle center ($\pm 70^\circ$) plus tangential paddle spin momentum. Minimum vertical velocity $|v_y| \ge 45$ is enforced to prevent horizontal bounce loops.

4. **In-Game Menus & UI Interaction**:
   - All menus (Main Menu, Level Select, Options, Pause Menu, Help) are rendered **inside the raster framebuffer** with the embedded 8×8 font. No separate OS popup dialogs.
   - Mouse hover must **never** mutate settings or play sounds. Options change **only on explicit left-click** or keyboard navigation (`Left`/`Right`/`Enter`).

5. **Audio Synthesis Architecture**:
   - Sound effects are synthesized as 44,100 Hz PCM WAV audio with additive harmonics (satisfying piano/kalimba "ting ting" tones).
   - Brick breaks climb an 8-note musical pentatonic scale (**C5 $\to$ D5 $\to$ E5 $\to$ G5 $\to$ A5 $\to$ C6 $\to$ D6 $\to$ E6**) as combos increase.
   - Playback is handled by polyphonic `QSoundEffect` instances with `QAudioSink` push-mode fallback.

---

## 2. Directory Structure

```text
Breakout-game/
├── .clangd                     # Compilation database config for Neovim/LSPs
├── .editorconfig               # Formatting rules (4 spaces, LF, trim whitespace)
├── CMakeLists.txt              # Modern C++20 build with compile_commands export
├── README.md                   # User guide, controls, screenshots, and features
├── BUILD.md                    # Detailed multi-platform build and multi-IDE setup
├── PLAN.md                     # Initial architectural specification
├── AGENTS.md                   # This instruction file
├── assets/
│   ├── demos/                  # Animated gameplay GIFs and demo recordings
│   │   ├── gameplay.gif
│   │   └── gameplay.mp4
│   ├── screenshots/            # Static promotional arcade screenshots
│   └── levels/                 # JSON level declarations
│       ├── level1.json
│       ├── level2.json
│       ├── level3.json
│       └── level4.json
└── src/
    ├── main.cpp                # Application entrypoint
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

---

## 3. Development & Tooling Guidelines

### Build Commands
```bash
# Configure with compile commands export
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Run (macOS)
./build/Breakout-game.app/Contents/MacOS/Breakout-game

# Run (Linux / Windows)
./build/Breakout-game
```

### IDE & LSP Support (Neovim, VS Code, CLion)
- The root [`.clangd`](file:///.clangd) file directs language servers directly to `build/compile_commands.json`:
  ```yaml
  CompileFlags:
    CompilationDatabase: "build"
    Add: [-std=c++20, -Wall, -Wextra]
  ```
- **Do not create symlinks** (`ln -s`) to `compile_commands.json` in the root folder, as `.clangd` handles this natively across all platforms.

---

## 4. How to Extend the Game

### Adding a New Level
1. Create a new file in `assets/levels/level<N>.json`:
   ```json
   {
     "name": "STAGE 5: CYBER LABYRINTH",
     "brickWidth": 28,
     "brickHeight": 9,
     "startX": 14,
     "startY": 28,
     "spacingX": 2,
     "spacingY": 2,
     "rows": [
       "X22222222X",
       "2111111112",
       "21PPEEPP12",
       "X22222222X"
     ],
     "legend": {
       ".": "empty",
       "1": { "type": "normal", "hp": 1, "color": "0xFF00E5FF" },
       "2": { "type": "armored", "hp": 2, "color": "0xFFFFD600" },
       "X": { "type": "indestructible", "hp": 999, "color": "0xFF718096" },
       "E": { "type": "explosive", "hp": 1, "color": "0xFFFF6D00" },
       "P": { "type": "powerup", "hp": 1, "color": "0xFFD500F9" }
     }
   }
   ```
2. The `LevelManager` will automatically discover and append the stage on startup.

### Adding a New Color Palette
1. Add the enum value to `PaletteTheme` in `src/core/Config.h`.
2. Implement colorway transformation logic in `RasterBuffer::applyThemeFilter()` in `src/raster/RasterBuffer.cpp`.
3. Add the label and cycle count to `GameEngine::menuLeft()`, `GameEngine::menuRight()`, and `GameEngine::renderOptionsMenu()` in `src/core/GameEngine.cpp`.

### Adding a New Power-Up
1. Add enum value to `PowerUpType` in `src/core/Config.h`.
2. Add letter icon and capsule color in `src/entities/PowerUp.cpp`.
3. Implement effect activation logic in `GameEngine::activatePowerUp()` in `src/core/GameEngine.cpp`.
4. Add description entry in `GameEngine::renderHelpMenu()` and `README.md`.

---

## 5. Important References

- **[PLAN.md](file:///PLAN.md)**: Original architecture specification and approved feature set.
- **[README.md](file:///README.md)**: User-facing documentation, controls, and build guide.
