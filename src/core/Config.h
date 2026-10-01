#pragma once

#include <cstdint>

namespace Breakout {

// Virtual raster resolution (4:3 retro arcade aspect ratio)
constexpr int VIRTUAL_WIDTH = 320;
constexpr int VIRTUAL_HEIGHT = 240;

// Arena playfield boundaries
constexpr int PLAYFIELD_TOP = 18;      // Header HUD area: y in [0, 17]
constexpr int PLAYFIELD_BOTTOM = 236;  // Floor pit: y >= 236 loses ball
constexpr int PLAYFIELD_LEFT = 6;      // Left border wall
constexpr int PLAYFIELD_RIGHT = 314;   // Right border wall

// Paddle parameters
constexpr float DEFAULT_PADDLE_WIDTH = 44.0f;
constexpr float ELONGATED_PADDLE_WIDTH = 68.0f;
constexpr float SHRUNK_PADDLE_WIDTH = 28.0f;
constexpr float PADDLE_HEIGHT = 7.0f;
constexpr float PADDLE_Y = 222.0f;
constexpr float PADDLE_SPEED = 240.0f; // Pixels per second

// Ball parameters
constexpr float BALL_RADIUS = 3.0f;
constexpr float BALL_BASE_SPEED = 160.0f;
constexpr float BALL_MAX_SPEED = 320.0f;
constexpr float BALL_SPEED_INCREMENT = 3.0f; // Added per brick hit

// Laser parameters
constexpr float LASER_SPEED = 280.0f;
constexpr float LASER_WIDTH = 2.0f;
constexpr float LASER_HEIGHT = 6.0f;

// Power-up capsule parameters
constexpr float POWERUP_WIDTH = 14.0f;
constexpr float POWERUP_HEIGHT = 7.0f;
constexpr float POWERUP_FALL_SPEED = 55.0f;
constexpr float POWERUP_DURATION_SEC = 12.0f;

// Physics sub-steps per frame for continuous collision detection (CCD)
constexpr int PHYSICS_SUB_STEPS = 4;

// High-contrast 32-bit ARGB retro colors (0xAARRGGBB)
namespace Colors {
constexpr uint32_t Transparent = 0x00000000;
constexpr uint32_t Black       = 0xFF0A0A0C;
constexpr uint32_t DarkBg      = 0xFF10121A;
constexpr uint32_t GridLine    = 0xFF181C28;
constexpr uint32_t BorderWall  = 0xFF4A5568;
constexpr uint32_t BorderGlow  = 0xFF718096;

constexpr uint32_t White       = 0xFFF7FAFC;
constexpr uint32_t GrayLight   = 0xFFCBD5E0;
constexpr uint32_t GrayMid     = 0xFF718096;
constexpr uint32_t GrayDark    = 0xFF2D3748;

constexpr uint32_t NeonCyan    = 0xFF00E5FF;
constexpr uint32_t NeonPink    = 0xFFFF1744;
constexpr uint32_t NeonYellow  = 0xFFFFD600;
constexpr uint32_t NeonGreen   = 0xFF00E676;
constexpr uint32_t NeonPurple  = 0xFFD500F9;
constexpr uint32_t NeonOrange  = 0xFFFF6D00;
constexpr uint32_t NeonBlue    = 0xFF2979FF;
constexpr uint32_t LaserRed    = 0xFFFF1744;

constexpr uint32_t BrickRed    = 0xFFFF3366;
constexpr uint32_t BrickOrange = 0xFFFF7700;
constexpr uint32_t BrickYellow = 0xFFFFCC00;
constexpr uint32_t BrickGreen  = 0xFF33CC66;
constexpr uint32_t BrickCyan   = 0xFF00DDFF;
constexpr uint32_t BrickPurple = 0xFFBB33FF;
constexpr uint32_t BrickSilver = 0xFFE2E8F0;
constexpr uint32_t BrickGold   = 0xFFFFB300;
} // namespace Colors

enum class GameState {
    MainMenu,       // Retro title & main options: Start, Levels, Settings, Help, Quit
    LevelSelect,    // Interactive stage select with preview
    OptionsMenu,    // In-game options: speed, lives, scanlines, theme, board/ball presets, audio, mouse
    HelpMenu,       // How to play & power-up guide
    Ready,       // Ball on paddle, waiting for Space/Launch
    Playing,     // Ball active
    Paused,      // In-game pause menu (Resume, Restart, Options, Levels, Menu)
    BallLost,    // Life lost, short respawn pause
    GameOver,    // All lives lost
    LevelWon,    // Level cleared, showing victory message
    GameCompleted// All levels beaten
};

enum class PowerUpType {
    None = 0,
    Elongate,    // Expands paddle
    Shrink,      // Shrinks paddle (penalty)
    MultiBall,   // Spawns 2 additional balls
    Fireball,    // Penetrates bricks without bouncing back
    Laser,       // Allows shooting lasers with Space
    StickyCatch, // Ball sticks to paddle on hit until launched
    SlowBall,    // Reduces ball speed
    Shield,      // Energy barrier at bottom pit (1 save)
    ExtraLife    // +1 life
};

enum class PaletteTheme {
    NeonArcade,     // Vibrant classic neon
    DarkOled,       // Deep black with electric ice blue & magenta accents
    Gruvbox,        // Retro warm Gruvbox dark (earthy retro tones)
    NeovimDefault,  // Modern Neovim 0.10+ dark slate palette
    GameBoy,        // Nostalgic 4-shade LCD green
    CyberpunkAmber  // Monochrome CRT amber phosphor
};

enum class CrtScanlineMode {
    Off,
    Subtle,
    Arcade
};

enum class PaddleSkin {
    Skateboard = 0, // Street skateboard with deck stripe & urethane wheels
    ClassicArcade,  // Sleek beveled neon laser capsule
    CyberHover,     // High-tech hover jet with side thrusters
    RetroWood       // Vintage mahogany wood grain deck
};

enum class BallSkin {
    EnergyOrb = 0,  // Classic glowing sphere with bright core
    PlasmaCore,     // Pulsing electric plasma orb
    NeonDiamond,    // Sparkling 4-point rotating geometric diamond
    CyberCube       // Retro 3D wireframe pixel cube
};

} // namespace Breakout

