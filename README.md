# Breakout

A classic Breakout arcade clone written in modern **C++20** and **Qt 6**.

This project implements a pure software rasterizer that draws the entire game into an internal **320 × 240** 32-bit ARGB framebuffer, blitted with integer scaling to preserve razor-sharp retro pixels without relying on OpenGL or vector graphics.

---

## Features

- **Software Raster Framebuffer**:
  - Internal 320 × 240 virtual resolution.
  - Custom drawing routines (horizontal/vertical scanlines, filled rects, Bresenham circles).
  - Built-in 8×8 monochrome bitmap font renderer.

- **Physics & Collision Detection**:
  - 2D vector mathematics (`Vec2`).
  - Sub-stepped continuous circle-to-AABB collision resolution to prevent ball tunneling.
  - Analytical paddle reflection with deflection angle proportional to hit distance from paddle center ($\pm 70^\circ$).
  - Horizontal paddle velocity transfer (spin deflection).

- **Controls**:
  - Keyboard (`A` / `D` or Arrow keys).
  - Smooth 1:1 mouse paddle tracking.
  - `Spacebar` / Left Click to launch ball from paddle.

- **Gameplay Loop**:
  - 5-row color-banded brick matrix (50 destructible bricks).
  - Live HUD displaying score and remaining lives.
  - Win and Game Over states with instant restart.

---

## Controls

| Action | Key / Input |
| :--- | :--- |
| **Move Paddle** | `Left` / `Right` Arrow or `A` / `D` |
| **Mouse Tracking** | Move mouse horizontally |
| **Launch Ball / Restart** | `Spacebar` or Left Click |
| **Pause / Resume** | `P` or `Escape` |
| **Quick Restart** | `R` |
| **Toggle Fullscreen** | `F11` |

---

## Building & Running

### Requirements
- C++20 compliant compiler (Clang 14+, GCC 11+, or MSVC 2019+)
- CMake 3.19+
- Qt 6 (Core, Gui, Widgets)

### Build Commands
```bash
# Configure
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build build -j

# Run (macOS)
./build/Breakout-game.app/Contents/MacOS/Breakout-game

# Run (Linux / Windows)
./build/Breakout-game
```

---

## Roadmap / Planned Features
- [ ] Sound synthesis engine for paddle bounces, wall hits, and brick destruction
- [ ] Falling power-up capsules (multiball, lasers, fireball, sticky paddle)
- [ ] Particle explosion effects for brick shards and sparks
- [ ] Data-driven level loader supporting custom JSON stage files
- [ ] Multiple retro color palette themes and CRT scanlines
