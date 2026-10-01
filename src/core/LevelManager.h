#pragma once

#include "entities/Brick.h"
#include <vector>
#include <string>
#include <memory>

namespace Breakout {

struct LevelData {
    std::string name;
    int index = 0;
    std::vector<Brick> bricks;
    int destructibleCount = 0;
};

class LevelManager {
public:
    LevelManager();

    // Scans assets/levels/ for JSON level files; falls back to embedded levels
    void discoverLevels(const std::string& assetsPath = "assets/levels");

    // Load level by index (0-based)
    bool loadLevel(int index);

    // Progression
    bool advanceLevel();
    void restartLevel();

    [[nodiscard]] const LevelData& getCurrentLevel() const { return m_currentLevel; }
    [[nodiscard]] std::vector<Brick>& getBricks() { return m_currentLevel.bricks; }
    [[nodiscard]] const std::vector<Brick>& getBricks() const { return m_currentLevel.bricks; }

    [[nodiscard]] int getCurrentLevelIndex() const { return m_currentLevelIndex; }
    [[nodiscard]] int getTotalLevels() const { return static_cast<int>(m_levelPaths.size()); }
    [[nodiscard]] bool hasNextLevel() const { return m_currentLevelIndex + 1 < getTotalLevels(); }

    [[nodiscard]] int getRemainingDestructibleBricks() const;

private:
    std::vector<std::string> m_levelPaths;
    int m_currentLevelIndex = 0;
    LevelData m_currentLevel;

    // JSON file loader
    bool loadFromFile(const std::string& filepath, LevelData& outLevel);

    // Embedded fallback levels
    void loadEmbeddedLevel(int index, LevelData& outLevel);
    void setupEmbeddedFallbackLevels();
};

} // namespace Breakout
