# Retro Breakout

A retro arcade Breakout game engineered in modern **C++20** and **Qt 6**.

Built with a custom software rasterizer (320 × 240 internal resolution), dynamic sound synthesis, particle explosion physics, falling power-ups, paddle lasers, and an extensible JSON level loader.

---

## Features

- **Software Raster Graphics & Themes**:
  - Pure 320 × 240 ARGB framebuffer with integer scaling and aspect-ratio letterboxing.
  - Six colorway themes: **Neon Arcade**, **Dark OLED**, **Gruvbox**, **Neovim** (slate green-cyan), **Game Boy** (4-shade green), and **Cyberpunk Amber**.
  - Configurable CRT scanlines (Subtle / Arcade / Off).

- **Procedural Sound Engine**:
  - 44,100 Hz PCM WAV audio synthesized algorithmically in memory without pre-recorded audio files.
  - Brick hits climb an 8-note musical pentatonic scale (**C5 $\to$ D5 $\to$ E5 $\to$ G5 $\to$ A5 $\to$ C6 $\to$ D6 $\to$ E6**) as combos mount.
  - Distinct synthesized tones for paddle bounces, wall hits, laser fire, and power-up pickups.

- **Particle Dynamics**:
  - Real-time software pixel sparks and brick debris on impacts.
  - Velocity decay, gravity, and color fading.

- **Power-Up Items**:
  - **[E] Elongate**: Expands paddle width.
  - **[S] Shrink**: Shrinks paddle width (penalty).
  - **[M] Multi-Ball**: Clones 2 extra active balls.
  - **[F] Fireball**: Pierces through bricks without deflection.
  - **[L] Laser Cannon**: Equips twin blaster cannons (fire with `Spacebar`).
  - **[C] Sticky Catch**: Catches the ball to re-aim and launch.
  - **[Z] Slow-Mo**: Lowers ball speed for precision control.

- **Data-Driven Level Pipeline**:
  - 6 included retro stages defined in `assets/levels/`.
  - Supports Normal, Armored (multi-hit), Indestructible, Explosive (chain reactions), and Power-Up bricks.
  - Easy to add new stages by creating JSON files.

- **Board & Ball Cosmetics**:
  - Selectable in **Settings**: Street Skateboard, Classic Arcade, Cyber Hovercraft, Retro Wood; Energy Orb, Plasma Core, Neon Diamond, Cyber Cube.

---

## Controls

| Action | Primary | Secondary / Alternative |
| :--- | :--- | :--- |
| **Move Paddle** | `Left` / `Right` Arrow | `A` / `D` or Mouse horizontal movement |
| **Launch Ball / Fire Lasers** | `Spacebar` | Left Mouse Click |
| **Pause / In-Game Menu** | `P` | `Escape` |
| **Restart Stage** | `R` | Menu: `Game -> Restart` |
| **Navigate Menus** | `Up` / `Down` / `Left` / `Right` | `Enter` / `Spacebar` to select |
| **Toggle Fullscreen** | `F11` | `F` |

---

## Building & Running

### Requirements
- C++20 compiler
- CMake 3.19+
- Qt 6 (Core, Gui, Widgets, Multimedia)

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

## Custom Level Format

Custom stages can be added as JSON files under `assets/levels/level<N>.json`:

```json
{
  "name": "STAGE 7: CUSTOM LEVEL",
  "brickWidth": 28,
  "brickHeight": 9,
  "startX": 14,
  "startY": 28,
  "spacingX": 2,
  "spacingY": 2,
  "rows": [
    "1111111111",
    "2222222222",
    "X.PPEEPP.X"
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
