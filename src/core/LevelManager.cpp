#include "LevelManager.h"
#include "core/Config.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <algorithm>

namespace Breakout {

LevelManager::LevelManager() {
    discoverLevels();
    loadLevel(0);
}

void LevelManager::discoverLevels(const std::string& assetsPath) {
    m_levelPaths.clear();

    QStringList searchDirs = {
        ":/assets/levels", // Qt compiled resource system (standalone executable)
        QString::fromStdString(assetsPath),
        QCoreApplication::applicationDirPath() + "/" + QString::fromStdString(assetsPath),
        QCoreApplication::applicationDirPath() + "/../Resources/" + QString::fromStdString(assetsPath),
        QCoreApplication::applicationDirPath() + "/../" + QString::fromStdString(assetsPath),
        QCoreApplication::applicationDirPath() + "/../../" + QString::fromStdString(assetsPath)
    };

    auto extractNum = [](const QString& s) {
        int n = 0;
        bool inNum = false;
        for (QChar c : s) {
            if (c.isDigit()) {
                n = n * 10 + c.digitValue();
                inNum = true;
            } else if (inNum) {
                break;
            }
        }
        return n;
    };

    for (const QString& dirPath : searchDirs) {
        QDir dir(dirPath);
        if (dir.exists()) {
            QStringList filters;
            filters << "*.json";
            QStringList files = dir.entryList(filters, QDir::Files, QDir::Name);
            // Sort naturally (level1, level2, ... level10)
            std::sort(files.begin(), files.end(), [&](const QString& a, const QString& b) {
                int numA = extractNum(a);
                int numB = extractNum(b);
                if (numA != numB && numA > 0 && numB > 0) return numA < numB;
                return a < b;
            });

            for (const QString& file : files) {
                m_levelPaths.push_back(dir.absoluteFilePath(file).toStdString());
            }
            if (!m_levelPaths.empty()) {
                break;
            }
        }
    }

    // If no level files found on disk, register 6 embedded fallback levels
    if (m_levelPaths.empty()) {
        m_levelPaths = {"embedded://1", "embedded://2", "embedded://3", "embedded://4", "embedded://5", "embedded://6"};
    }
}

bool LevelManager::loadFromFile(const std::string& filepath, LevelData& outLevel) {
    QFile file(QString::fromStdString(filepath));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();
    outLevel.name = root["name"].toString("Unnamed Level").toStdString();
    outLevel.bricks.clear();
    outLevel.destructibleCount = 0;

    float bw = static_cast<float>(root["brickWidth"].toDouble(28.0));
    float bh = static_cast<float>(root["brickHeight"].toDouble(9.0));
    float startX = static_cast<float>(root["startX"].toDouble(16.0));
    float startY = static_cast<float>(root["startY"].toDouble(28.0));
    float spX = static_cast<float>(root["spacingX"].toDouble(2.0));
    float spY = static_cast<float>(root["spacingY"].toDouble(2.0));

    QJsonObject legend = root["legend"].toObject();
    QJsonArray rows = root["rows"].toArray();

    for (int r = 0; r < rows.size(); ++r) {
        QString rowStr = rows[r].toString();
        for (int c = 0; c < rowStr.length(); ++c) {
            QChar ch = rowStr[c];
            if (ch == '.' || ch == ' ') continue;

            QString key = QString(ch);
            QJsonObject brickDef = legend[key].toObject();

            QString typeStr = brickDef["type"].toString("normal");
            int hp = brickDef["hp"].toInt(1);
            QString colorStr = brickDef["color"].toString();
            bool ok = false;
            uint32_t color = colorStr.toUInt(&ok, 16);
            if (!ok || color == 0) color = Colors::NeonCyan;

            BrickType bType = BrickType::Normal;
            if (typeStr == "armored") bType = BrickType::Armored;
            else if (typeStr == "indestructible") bType = BrickType::Indestructible;
            else if (typeStr == "explosive") bType = BrickType::Explosive;
            else if (typeStr == "powerup") bType = BrickType::PowerUp;

            float bx = startX + c * (bw + spX);
            float by = startY + r * (bh + spY);

            outLevel.bricks.emplace_back(bx, by, bw, bh, bType, hp, color);
            if (bType != BrickType::Indestructible) {
                outLevel.destructibleCount++;
            }
        }
    }

    return true;
}

void LevelManager::loadEmbeddedLevel(int index, LevelData& outLevel) {
    outLevel.bricks.clear();
    outLevel.destructibleCount = 0;
    outLevel.index = index;

    constexpr float bw = 28.0f;
    constexpr float bh = 9.0f;
    constexpr float startX = 14.0f;
    constexpr float startY = 28.0f;
    constexpr float spX = 2.0f;
    constexpr float spY = 2.0f;

    std::vector<std::string> rows;

    if (index == 0) {
        outLevel.name = "STAGE 1: NEON HORIZON";
        rows = {
            "RRRRRRRRRR",
            "OOOOOOOOOO",
            "YYYYYYYYYY",
            "GGGGGGGGGG",
            "CCCCCCCCCC",
            "P...PP...P"
        };
    } else if (index == 1) {
        outLevel.name = "STAGE 2: ARMORED FORTRESS";
        rows = {
            "X22222222X",
            "2111111112",
            "21PPEEPP12",
            "2111111112",
            "X22222222X",
            ".P..EE..P."
        };
    } else if (index == 2) {
        outLevel.name = "STAGE 3: EXPLOSIVE MINEFIELD";
        rows = {
            "3.E.33.E.3",
            "2E1E22E1E2",
            "3.E.33.E.3",
            "X..P..P..X",
            "2222222222",
            ".E..PP..E."
        };
    } else if (index == 3) {
        outLevel.name = "STAGE 4: CITADEL MATRIX";
        rows = {
            "X3X3X3X3X3",
            "3E2P22P2E3",
            "X22222222X",
            "1111111111",
            "EPPEEPPEEP",
            "X..X..X..X"
        };
    } else if (index == 4) {
        outLevel.name = "STAGE 5: CYBER DIAMOND";
        rows = {
            "....22....",
            "...2EE2...",
            "..21PP12..",
            ".21EEEE12.",
            "211PPPP112",
            ".21EEEE12.",
            "..21PP12..",
            "...2EE2...",
            "....22...."
        };
    } else {
        outLevel.name = "STAGE 6: SUPERNOVA CASCADE";
        rows = {
            "E11E11E11E",
            "1221PP1221",
            "12E2112E21",
            "E122EE221E",
            "PPP1221PPP",
            "E122EE221E",
            "12E2112E21",
            "1221PP1221",
            "E11E11E11E"
        };
    }

    for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
        const std::string& row = rows[static_cast<size_t>(r)];
        for (int c = 0; c < static_cast<int>(row.size()); ++c) {
            char ch = row[static_cast<size_t>(c)];
            if (ch == '.' || ch == ' ') continue;

            BrickType type = BrickType::Normal;
            int hp = 1;
            uint32_t col = Colors::NeonCyan;

            switch (ch) {
                case 'R': hp = 1; col = Colors::BrickRed; type = BrickType::Normal; break;
                case 'O': hp = 1; col = Colors::BrickOrange; type = BrickType::Normal; break;
                case 'Y': hp = 1; col = Colors::BrickYellow; type = BrickType::Normal; break;
                case 'G': hp = 1; col = Colors::BrickGreen; type = BrickType::Normal; break;
                case 'C': hp = 1; col = Colors::BrickCyan; type = BrickType::Normal; break;
                case 'P': hp = 1; col = Colors::NeonPurple; type = BrickType::PowerUp; break;
                case '1': hp = 1; col = Colors::NeonCyan; type = BrickType::Normal; break;
                case '2': hp = 2; col = Colors::NeonYellow; type = BrickType::Armored; break;
                case '3': hp = 3; col = Colors::NeonPink; type = BrickType::Armored; break;
                case 'X': hp = 999; col = Colors::BorderWall; type = BrickType::Indestructible; break;
                case 'E': hp = 1; col = Colors::NeonOrange; type = BrickType::Explosive; break;
                default: break;
            }

            float bx = startX + static_cast<float>(c) * (bw + spX);
            float by = startY + static_cast<float>(r) * (bh + spY);

            outLevel.bricks.emplace_back(bx, by, bw, bh, type, hp, col);
            if (type != BrickType::Indestructible) {
                outLevel.destructibleCount++;
            }
        }
    }
}

bool LevelManager::loadLevel(int index) {
    if (m_levelPaths.empty()) return false;
    if (index < 0 || index >= static_cast<int>(m_levelPaths.size())) {
        index = 0;
    }
    m_currentLevelIndex = index;

    const std::string& path = m_levelPaths[static_cast<size_t>(index)];
    if (path.rfind("embedded://", 0) == 0) {
        loadEmbeddedLevel(index, m_currentLevel);
        return true;
    }

    if (!loadFromFile(path, m_currentLevel)) {
        loadEmbeddedLevel(index, m_currentLevel);
    }
    return true;
}

bool LevelManager::peekLevel(int index, LevelData& outLevel) {
    if (m_levelPaths.empty()) return false;
    if (index < 0 || index >= static_cast<int>(m_levelPaths.size())) return false;
    const std::string& path = m_levelPaths[static_cast<size_t>(index)];
    if (path.rfind("embedded://", 0) == 0) {
        loadEmbeddedLevel(index, outLevel);
        return true;
    }
    if (!loadFromFile(path, outLevel)) {
        loadEmbeddedLevel(index, outLevel);
    }
    return true;
}

bool LevelManager::advanceLevel() {
    if (hasNextLevel()) {
        return loadLevel(m_currentLevelIndex + 1);
    }
    return false;
}

void LevelManager::restartLevel() {
    loadLevel(m_currentLevelIndex);
}

int LevelManager::getRemainingDestructibleBricks() const {
    int remaining = 0;
    for (const auto& b : m_currentLevel.bricks) {
        if (!b.isDestroyed() && !b.isIndestructible()) {
            remaining++;
        }
    }
    return remaining;
}

} // namespace Breakout
