# Breakout Game: Architecture, Physics & Implementation Plan

A retro pixel-art Breakout game built in **C++20** and **Qt 6**, featuring a pure software raster grid, arcade-accurate continuous physics, modular level architecture, and power-up mechanics.

---

## 1. Project Overview & Guiding Constraints

1. **Pure Raster Graphics (No OpenGL / No Vector Graphics)**:
   - The game renders entirely into an internal low-resolution software pixel buffer (framebuffer).
   - All shapes, sprites, particles, and text are drawn pixel-by-pixel (or raster-blitted) into memory.
   - The final framebuffer is scaled up using nearest-neighbor scaling (`Qt::FastTransformation`) to maintain sharp, unblurred retro pixels.
   - Optional software CRT scanline / pixel-grid overlay for arcade aesthetics.

2. **Arcade-Quality Physics**:
   - Sub-stepped continuous collision detection (CCD) to prevent ball tunneling at high velocities.
   - Continuous angle response based on hit distance from paddle center with spin/paddle momentum influence.
   - Accurate rectangular corner/edge penetration resolution.

3. **Extensible Level Pipeline**:
   - Declarative level format (JSON / ASCII matrix) allowing quick creation of new levels without recompiling code.
   - Multiple brick behaviors: standard multi-hit, armored, indestructible metal, explosive, and item drops.

4. **Power-Up Items**:
   - Falling capsules: Elongated paddle, multi-ball, piercing fireball, laser cannons, sticky catch, speed control, safety shield, extra lives.

5. **Multi-IDE & Tooling Support**:
   - Modern CMake with automatic `compile_commands.json` export and symlink to root.
   - Configured `.clangd` for seamless Neovim (`nvim-lspconfig`, `mason`, `coc`) and VS Code / CLion support.
   - Strict `.editorconfig` for formatting standards.

---

## 2. Directory Structure

```text
Breakout-game/
├── .clangd                     # Clangd configuration for Neovim / other IDEs
├── .editorconfig               # Code style & whitespace conventions
├── CMakeLists.txt              # Build configuration with compile_commands export
├── README.md                   # Build instructions, controls, architecture docs
├── PLAN.md                     # This plan document
├── assets/
│   └── levels/
│       ├── level1.json         # Level 1: Classic layout
│       ├── level2.json         # Level 2: Armored & explosive puzzle
│       └── level3.json         # Level 3: Advanced obstacles & shields
└── src/
    ├── main.cpp                # Application entrypoint
    ├── core/
    │   ├── Config.h            # Virtual resolution (320x240), speeds, timings
    │   ├── GameEngine.h/cpp    # State machine (Menu, Playing, Paused, GameOver, Win)
    │   └── LevelManager.h/cpp  # Level parser, progression & validation
    ├── entities/
    │   ├── Ball.h/cpp          # Ball state, velocity, trail, piercing mode
    │   ├── Paddle.h/cpp        # Paddle geometry, elongation, laser attachments
    │   ├── Brick.h/cpp         # Durability, types, damage visual states
    │   ├── PowerUp.h/cpp       # Droppable capsules, duration timers
    │   └── Laser.h/cpp         # Laser projectile entities
    ├── physics/
    │   ├── Vec2.h              # 2D vector arithmetic helper
    │   └── Collision.h/cpp     # Swept AABB & sub-step collision resolution
    ├── raster/
    │   ├── RasterBuffer.h/cpp  # Software 320x240 ARGB pixel buffer & draw primitives
    │   ├── BitmapFont.h/cpp    # Retro 8x8 bitmap font renderer (no vector fonts)
    │   └── ParticleSystem.h/cpp# Software pixel sparks, dust, and explosions
    └── ui/
        ├── MainWindow.h/cpp    # Window chrome, menus, full-screen toggle
        └── RasterWidget.h/cpp  # QWidget hosting and scaling the raster buffer
```

---

## 3. Detailed Component Architecture

### 3.1 Raster Grid & Pixel Engine

- **Virtual Resolution**: Fixed at **`320 × 240`** (4:3 classic aspect ratio).
- **Software Buffer**:
  - An internal array of 32-bit ARGB pixels wrapped in a `QImage` (`Format_ARGB32_Premultiplied`).
  - Primitive drawing routines implemented directly on the pixel buffer:
    - `drawPixel(x, y, color)`
    - `drawRect(x, y, w, h, color)` & `fillRect(...)`
    - `drawCircle(cx, cy, r, color)` & `fillCircle(...)`
    - `drawSprite(x, y, spriteData, ...)`
    - `drawBitmapText(x, y, text, color)` using an embedded 8×8 monochrome font table.
- **Pixel Scaler**:
  - `RasterWidget::paintEvent()` calculates the maximum integer or aspect-ratio preserving rectangle in the window and draws the `QImage` using `QPainter::drawImage()` with smooth pixmap transform disabled (`painter.setRenderHint(QPainter::SmoothPixmapTransform, false)`).
  - Letterbox margins filled with matte black.
- **Scanline & Pixel Grid Effect**:
  - An optional software post-processing pass applying subtle horizontal scanlines or grid dot matrix for a CRT arcade feel.

---

### 3.2 Physics & Collision Subsystem

- **Continuous Sub-Stepping**:
  - At standard 60 FPS update, high-velocity balls can move 8–12 pixels per frame, which exceeds the thickness of some bricks.
  - Physics updates run in **4 sub-steps per frame** ($\Delta t / 4$) or swept ray tests to guarantee zero tunneling.
- **Paddle Collision & Reflection Angle**:
  - Hitting the center of the paddle reflects the ball nearly straight up.
  - Hitting the left or right edges deflects the ball outward up to $\pm 75^\circ$.
  - If the paddle is moving horizontally when the ball hits it, a portion of the paddle's velocity is added to the ball as tangential "spin".
- **Brick Collision**:
  - Calculates overlap on both $X$ and $Y$ axes.
  - The axis with smaller penetration is chosen as the collision normal to guarantee proper corner bounce vs edge bounce.

---

### 3.3 Extensible Level System

- **Level Format**: JSON format for easy human editing and extensibility.
- **Example Schema**:
  ```json
  {
    "name": "Emerald Citadel",
    "rows": 8,
    "cols": 14,
    "layout": [
      "##############",
      "#222222222222#",
      "#211111111112#",
      "#21PPEEEEPP12#",
      "#211111111112#",
      "#222222222222#",
      "..............",
      ".............."
    ],
    "legend": {
      ".": "empty",
      "#": "indestructible",
      "1": { "type": "normal", "hp": 1, "color": "#00E5FF" },
      "2": { "type": "armored", "hp": 2, "color": "#FFD600" },
      "E": { "type": "explosive", "hp": 1, "color": "#FF3D00" },
      "P": { "type": "powerup_chance", "hp": 1, "color": "#D500F9" }
    }
  }
  ```
- **Extensibility**:
  - Adding a new level is as simple as creating `assets/levels/level4.json`. The `LevelManager` automatically discovers all `.json` files in the levels folder and chains them sequentially.

---

### 3.4 Power-Ups and Collectibles

When a brick is broken, it has a chance to drop a falling pixel capsule:

| Power-Up         | Visual Icon     | Effect Description                                                        |
| :--------------- | :-------------- | :------------------------------------------------------------------------ |
| **Elongate**     | Green `[ <-> ]` | Increases paddle width from 48px to 72px (15s duration).                  |
| **Shrink**       | Red `[ >-< ]`   | Reduces paddle width to 32px (10s penalty).                               |
| **Multi-Ball**   | Cyan `[ 3x ]`   | Clones each active ball into 3 balls with diverging angles.               |
| **Fireball**     | Orange `[ * ]`  | Ball penetrates through bricks without rebounding (8s duration).          |
| **Laser Cannon** | Yellow `[ ! ]`  | Mounts twin guns to paddle; press `Space` to shoot bricks (12s duration). |
| **Sticky Catch** | Purple `[ U ]`  | Catches ball on paddle; player presses `Space` to aim and launch.         |
| **Slow-Mo**      | Blue `[ >> ]`   | Slows ball speed by 35% for easier control (12s duration).                |
| **Safety Net**   | Green bar       | Adds a single-use energy barrier at bottom pit.                           |
| **Extra Life**   | Pink `[ +1 ]`   | Grants 1 additional life.                                                 |

---

### 3.5 Development & Tooling Setup (Neovim / Clangd)

- **CMakeLists.txt**:
  - Sets `CMAKE_EXPORT_COMPILE_COMMANDS ON`.
  - Adds custom command to create/update a symlink to `compile_commands.json` in the root folder:
    ```cmake
    add_custom_target(copy_compile_commands ALL
        ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
    ```
- **`.clangd`**:
  - Root file specifying compile database location and C++20 options so Neovim/clangd provides full autocompletion, diagnostic checks, and go-to-definition without error.
- **`.editorconfig`**:
  - Enforces UTF-8, 4 spaces indent, trailing whitespace removal, and newline at end of file.

---

## 4. Implementation Phases

1. **Phase 1: Environment & Tooling**
   - Setup `.clangd`, `.editorconfig`, updated `CMakeLists.txt`.
   - Verify Neovim/clangd compatibility with clean `compile_commands.json`.

2. **Phase 2: Raster Engine & Retro Font**
   - Implement `RasterBuffer`, `BitmapFont`, and `RasterWidget`.
   - Verify 320x240 pixel canvas scaling with integer/aspect ratio and CRT effect.

3. **Phase 3: Core Physics & Entities**
   - Implement `Paddle`, `Ball`, `Collision` (sub-stepped CCD & angle deflection).
   - Add keyboard/mouse controls and smooth paddle physics.

4. **Phase 4: Brick Subsystem & Level Manager**
   - Implement brick types (normal, armored with damage cracks, indestructible, explosive).
   - Implement JSON level loader and add 3 varied built-in levels.

5. **Phase 5: Power-Ups & Particle Effects**
   - Implement falling items, timers, active HUD indicators.
   - Add laser shooting, multi-ball handling, fireball piercing.
   - Add pixel particle explosions when bricks break.

6. **Phase 6: Audio, Polish & Documentation**
   - Synthesized retro audio beeps/boops (via Qt Audio or soft tone generator).
   - Score tracking, high score persistence, game over & victory screens.
   - Complete `README.md` with full control scheme and developer guide.

### 3.6 Controls, Keybindings & Zoom System

- **Paddle & Game Controls**:
  - `Left` / `Right` arrows or `A` / `D`: Smooth paddle movement with responsive arcade velocity.
  - `Space`: Launch ball from paddle / Fire laser cannons / Advance from menus & victory screens.
  - `P` or `Escape`: Pause / Resume game.
  - `R`: Quick restart current level.
  - Mouse movement support (optional toggle): Paddle follows cursor smoothly within the game bounds.

- **Dynamic Pixel Zoom & Window Scaling (Cross-Platform)**:
  - `Ctrl +` / `Cmd +` / `=`: Increase pixel scale factor ($1\times \to 2\times \to 3\times \to 4\times$).
  - `Ctrl -` / `Cmd -` / `-`: Decrease pixel scale factor.
  - `Ctrl 0` / `Cmd 0`: Auto-fit window preserving 4:3 pixel aspect ratio with crisp nearest-neighbor integer scaling.
  - `F` / `F11`: Fullscreen toggle with black matte arcade borders.
  - Uses Qt's cross-platform standard shortcuts (`QKeySequence::ZoomIn`, `QKeySequence::ZoomOut`, `QKeySequence::FullScreen`) ensuring identical behavior on both Windows, macOS, and Linux.

- **In-Game Settings & Dynamic Tuning Dialog**:
  - Accessible via Menu bar (`Game -> Settings`), standard shortcut `Ctrl ,` / `Cmd ,`, or in-game pause menu.
  - **Difficulty / Ball Speed**: Adjustable baseline speed multiplier (Easy: 0.8x, Normal: 1.0x, Fast/Arcade: 1.3x).
  - **Starting Lives**: Configurable (e.g., 3, 5, or 7 lives).
  - **Retro Display Filters**:
    - Pixel Grid / CRT Scanline intensity (Off, Subtle, Classic Arcade).
    - Palette mode (Classic Neon, Game Boy Monochrome, Cyberpunk Amber).
  - **Audio & Sound Effects**: Volume sliders and mute toggle.

- **Pixel HUD & Statistics**:
  - Rendered directly into the software raster buffer with the retro 8x8 font:
    - **Score**: Points per brick broken, with consecutive bounce combo multipliers.
    - **High Score**: Persisted between sessions via `QSettings`.
    - **Lives Indicator**: Visual mini pixel paddles.
    - **Level Timer**: Clean elapsed time display (`MM:SS`).
    - **Power-Up Bar**: Remaining time meters for active buffs (Lasers, Fireball, Elongate).

---

## 4. Implementation Phases

1. **Phase 1: Environment & Tooling Setup**
   - Configure `.clangd`, `.editorconfig`, and root `compile_commands.json` export in `CMakeLists.txt`.
   - Verify Neovim / clangd LSP compatibility.

2. **Phase 2: Pure Software Raster Engine & Scaler**
   - Implement `RasterBuffer` (320x240 ARGB pixel buffer with primitives: lines, circles, boxes, sprites).
   - Implement `BitmapFont` (embedded 8x8 retro font table).
   - Implement `RasterWidget` (nearest-neighbor scaling, zoom controls, scanline post-processing).

3. **Phase 3: Core Physics Engine & Player Controls**
   - Implement `Vec2` math and `Collision` (sub-stepped continuous collision detection).
   - Implement `Paddle` (with smooth input handling) and `Ball` (angle deflection, velocity clamping).

4. **Phase 4: Bricks, Extensible Level Loader & Levels**
   - Implement `Brick` types (normal, armored with damage cracks, explosive, indestructible).
   - Implement `LevelManager` loading JSON level files.
   - Author 3 distinct built-in levels with progressive complexity.

5. **Phase 5: Power-Ups, Lasers & Pixel Particle Effects**
   - Implement falling capsules (Elongate, Shrink, Multi-Ball, Fireball, Lasers, Sticky Catch, Shield).
   - Implement software particle spark engine for brick breaks and paddle bounces.

6. **Phase 6: Dynamic Settings, Audio & Polish**
   - Settings dialog (ball speed, lives, zoom, CRT filter toggles).
   - High score persistence, level transitions, sound effects via synthesized retro tones.
   - Comprehensive `README.md` with controls, architecture, and Neovim instructions.

