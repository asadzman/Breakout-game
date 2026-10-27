#include "RasterBuffer.h"
#include "BitmapFont.h"
#include <algorithm>
#include <cmath>

namespace Breakout {

RasterBuffer::RasterBuffer(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_pixels(static_cast<size_t>(width * height), Colors::Black)
    , m_image(reinterpret_cast<uchar*>(m_pixels.data()),
              width,
              height,
              width * static_cast<qsizetype>(sizeof(uint32_t)),
              QImage::Format_ARGB32_Premultiplied)
{
}

void RasterBuffer::clear(uint32_t color) {
    std::fill(m_pixels.begin(), m_pixels.end(), color);
}

void RasterBuffer::drawFastHLine(int x1, int x2, int y, uint32_t color) {
    if (y < 0 || y >= m_height) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = std::max(0, x1);
    x2 = std::min(m_width - 1, x2);
    if (x1 > x2) return;

    size_t start = static_cast<size_t>(y * m_width + x1);
    size_t count = static_cast<size_t>(x2 - x1 + 1);
    std::fill_n(m_pixels.data() + start, count, color);
}

void RasterBuffer::drawFastVLine(int x, int y1, int y2, uint32_t color) {
    if (x < 0 || x >= m_width) return;
    if (y1 > y2) std::swap(y1, y2);
    y1 = std::max(0, y1);
    y2 = std::min(m_height - 1, y2);
    if (y1 > y2) return;

    for (int y = y1; y <= y2; ++y) {
        m_pixels[static_cast<size_t>(y * m_width + x)] = color;
    }
}

void RasterBuffer::drawRect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    drawFastHLine(x, x + w - 1, y, color);
    drawFastHLine(x, x + w - 1, y + h - 1, color);
    drawFastVLine(x, y, y + h - 1, color);
    drawFastVLine(x + w - 1, y, y + h - 1, color);
}

void RasterBuffer::fillRect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    int yEnd = std::min(m_height - 1, y + h - 1);
    int yStart = std::max(0, y);
    for (int curY = yStart; curY <= yEnd; ++curY) {
        drawFastHLine(x, x + w - 1, curY, color);
    }
}

void RasterBuffer::drawLine(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void RasterBuffer::drawCircle(int cx, int cy, int radius, uint32_t color) {
    if (radius <= 0) {
        setPixel(cx, cy, color);
        return;
    }
    int x = radius;
    int y = 0;
    int err = 1 - radius;

    while (x >= y) {
        setPixel(cx + x, cy + y, color);
        setPixel(cx + y, cy + x, color);
        setPixel(cx - y, cy + x, color);
        setPixel(cx - x, cy + y, color);
        setPixel(cx - x, cy - y, color);
        setPixel(cx - y, cy - x, color);
        setPixel(cx + y, cy - x, color);
        setPixel(cx + x, cy - y, color);

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void RasterBuffer::fillCircle(int cx, int cy, int radius, uint32_t color) {
    if (radius <= 0) {
        setPixel(cx, cy, color);
        return;
    }
    int x = radius;
    int y = 0;
    int err = 1 - radius;

    while (x >= y) {
        drawFastHLine(cx - x, cx + x, cy + y, color);
        drawFastHLine(cx - x, cx + x, cy - y, color);
        drawFastHLine(cx - y, cx + y, cy + x, color);
        drawFastHLine(cx - y, cx + y, cy - x, color);

        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void RasterBuffer::drawBitmapText(int x, int y, std::string_view text, uint32_t color, int scale, int spacing) {
    if (scale < 1) scale = 1;
    int cursorX = x;

    for (char c : text) {
        const uint8_t* glyph = BitmapFont::getGlyph(c);
        for (int row = 0; row < BitmapFont::GLYPH_HEIGHT; ++row) {
            uint8_t rowBits = glyph[row];
            for (int col = 0; col < BitmapFont::GLYPH_WIDTH; ++col) {
                if ((rowBits & (0x80 >> col)) != 0) {
                    if (scale == 1) {
                        setPixel(cursorX + col, y + row, color);
                    } else {
                        fillRect(cursorX + col * scale, y + row * scale, scale, scale, color);
                    }
                }
            }
        }
        cursorX += BitmapFont::GLYPH_WIDTH * scale + spacing;
    }
}

void RasterBuffer::drawBitmapTextCentered(int y, std::string_view text, uint32_t color, int scale) {
    int textW = BitmapFont::measureText(text, scale);
    int startX = (m_width - textW) / 2;
    drawBitmapText(startX, y, text, color, scale);
}

void RasterBuffer::renderBackground(BackgroundStyle style, float animTime, int stageIndex) {
    if (style == BackgroundStyle::DynamicStage) {
        int stg = std::max(0, stageIndex) % 10;
        switch (stg) {
            case 0: // Stage 1: Neon Horizon
                style = BackgroundStyle::NeonSynthwave;
                break;
            case 1: // Stage 2: Armored Fortress
                style = BackgroundStyle::CyberGrid;
                break;
            case 2: { // Stage 3: Explosive Minefield - Volcanic obsidian with glowing embers
                for (int y = 0; y < m_height; ++y) {
                    float t = static_cast<float>(y) / static_cast<float>(m_height);
                    uint32_t r = static_cast<uint32_t>(18 + 42 * t);
                    uint32_t g = static_cast<uint32_t>(8 + 12 * t);
                    uint32_t b = static_cast<uint32_t>(10 + 6 * t);
                    drawFastHLine(0, m_width - 1, y, 0xFF000000 | (r << 16) | (g << 8) | b);
                }
                for (int i = 0; i < 35; ++i) {
                    float seedX = static_cast<float>((i * 67 + 23) % m_width);
                    float speed = 15.0f + static_cast<float>((i * 13) % 25);
                    float ey = std::fmod(m_height * 2.0f - animTime * speed + (i * 37), static_cast<float>(m_height));
                    float ex = seedX + std::sin(animTime * 2.0f + i) * 6.0f;
                    int px = static_cast<int>(ex);
                    int py = static_cast<int>(ey);
                    uint32_t emberCol = (i % 2 == 0) ? Colors::NeonOrange : Colors::NeonYellow;
                    setPixelBlend(px, py, emberCol, 0.75f);
                    setPixelBlend(px, py + 1, Colors::BrickRed, 0.4f);
                }
                return;
            }
            case 3: // Stage 4: Citadel Matrix
                style = BackgroundStyle::CyberGrid;
                break;
            case 4: // Stage 5: Cyber Diamond
                style = BackgroundStyle::CosmicNebula;
                break;
            case 5: // Stage 6: Supernova Cascade
                style = BackgroundStyle::CosmicNebula;
                break;
            case 6: // Stage 7: Vortex Galaxy
                style = BackgroundStyle::AuroraGlow;
                break;
            case 7: // Stage 8: Retro Invader
                style = BackgroundStyle::CosmicNebula;
                break;
            case 8: // Stage 9: Prism Pinball
                style = BackgroundStyle::CyberGrid;
                break;
            case 9: default: // Stage 10: Cosmic Phoenix
                style = BackgroundStyle::NeonSynthwave;
                break;
        }
    }

    if (style == BackgroundStyle::DeepVoid) {
        clear(Colors::DarkBg);
        for (int x = 20; x < m_width; x += 40) {
            drawFastVLine(x, 0, m_height - 1, 0xFF141722);
        }
        for (int y = 20; y < m_height; y += 40) {
            drawFastHLine(0, m_width - 1, y, 0xFF141722);
        }
        return;
    }

    if (style == BackgroundStyle::NeonSynthwave) {
        constexpr int HORIZON_Y = 135;
        for (int y = 0; y < HORIZON_Y; ++y) {
            float t = static_cast<float>(y) / static_cast<float>(HORIZON_Y);
            uint32_t r = static_cast<uint32_t>(16.0f * (1.0f - t) + 85.0f * t);
            uint32_t g = static_cast<uint32_t>(6.0f * (1.0f - t) + 16.0f * t);
            uint32_t b = static_cast<uint32_t>(38.0f * (1.0f - t) + 70.0f * t);
            drawFastHLine(0, m_width - 1, y, 0xFF000000 | (r << 16) | (g << 8) | b);
        }

        constexpr int SUN_CX = 160;
        constexpr int SUN_CY = 135;
        constexpr int SUN_R = 28;
        for (int dy = -SUN_R; dy <= 0; ++dy) {
            if (dy > -16 && (dy % 4 == 0 || dy % 4 == 1)) {
                continue;
            }
            int halfW = static_cast<int>(std::sqrt(SUN_R * SUN_R - dy * dy));
            float st = static_cast<float>(dy + SUN_R) / static_cast<float>(SUN_R);
            uint32_t sr = static_cast<uint32_t>(255);
            uint32_t sg = static_cast<uint32_t>(220.0f * (1.0f - st) + 40.0f * st);
            uint32_t sb = static_cast<uint32_t>(40.0f * (1.0f - st) + 120.0f * st);
            drawFastHLine(SUN_CX - halfW, SUN_CX + halfW, SUN_CY + dy, 0xFF000000 | (sr << 16) | (sg << 8) | sb);
        }

        for (int y = HORIZON_Y; y < m_height; ++y) {
            drawFastHLine(0, m_width - 1, y, 0xFF0A0516);
        }

        for (int k = 1; k <= 7; ++k) {
            float frac = static_cast<float>(k) / 7.0f;
            int py = HORIZON_Y + static_cast<int>((m_height - HORIZON_Y) * frac * frac);
            uint32_t lineCol = (k % 2 == 0) ? 0xFFE0247D : 0xFF99155C;
            drawFastHLine(0, m_width - 1, py, lineCol);
        }

        for (int gx = -160; gx <= 480; gx += 40) {
            drawLine(SUN_CX, HORIZON_Y, gx, m_height - 1, 0xFF5C1045);
        }

        for (int s = 0; s < 30; ++s) {
            int sx = (s * 47 + 19) % m_width;
            int sy = (s * 31 + 7) % 95;
            float tw = std::sin(animTime * 3.0f + s * 1.5f);
            if (tw > 0.0f) {
                uint32_t sc = (s % 3 == 0) ? 0xFF00E5FF : ((s % 3 == 1) ? 0xFFFFD600 : 0xFFFFFFFF);
                setPixelBlend(sx, sy, sc, 0.4f + 0.5f * tw);
            }
        }
        return;
    }

    if (style == BackgroundStyle::CosmicNebula) {
        clear(0xFF070914);

        for (int y = 0; y < m_height; y += 2) {
            for (int x = 0; x < m_width; x += 2) {
                float d1 = std::hypot(x - 90, y - 80);
                float d2 = std::hypot(x - 230, y - 130);
                float d3 = std::hypot(x - 160, y - 190);

                float neb1 = std::max(0.0f, 1.0f - d1 / 75.0f);
                float neb2 = std::max(0.0f, 1.0f - d2 / 85.0f);
                float neb3 = std::max(0.0f, 1.0f - d3 / 65.0f);

                uint32_t nr = static_cast<uint32_t>(neb1 * 40.0f + neb3 * 30.0f);
                uint32_t ng = static_cast<uint32_t>(neb2 * 25.0f + neb3 * 10.0f);
                uint32_t nb = static_cast<uint32_t>(neb1 * 50.0f + neb2 * 55.0f + neb3 * 45.0f);

                if (nr > 0 || ng > 0 || nb > 0) {
                    uint32_t nebCol = 0xFF000000 | (std::min(255u, nr) << 16) | (std::min(255u, ng) << 8) | std::min(255u, nb);
                    setPixel(x, y, nebCol);
                    setPixel(x + 1, y, nebCol);
                    setPixel(x, y + 1, nebCol);
                    setPixel(x + 1, y + 1, nebCol);
                }
            }
        }

        for (int s = 0; s < 50; ++s) {
            int sx = (s * 73 + 17) % m_width;
            int sy = (s * 59 + 29) % m_height;
            float tw = 0.5f + 0.5f * std::sin(animTime * 2.5f + s * 1.8f);

            uint32_t scol = Colors::White;
            switch (s % 5) {
                case 0: scol = Colors::NeonCyan; break;
                case 1: scol = Colors::NeonPink; break;
                case 2: scol = Colors::NeonYellow; break;
                case 3: scol = Colors::NeonGreen; break;
                default: scol = Colors::White; break;
            }

            setPixelBlend(sx, sy, scol, tw);
            if (s % 7 == 0 && tw > 0.7f) {
                setPixelBlend(sx - 1, sy, scol, 0.4f * tw);
                setPixelBlend(sx + 1, sy, scol, 0.4f * tw);
                setPixelBlend(sx, sy - 1, scol, 0.4f * tw);
                setPixelBlend(sx, sy + 1, scol, 0.4f * tw);
            }
        }
        return;
    }

    if (style == BackgroundStyle::CyberGrid) {
        clear(0xFF090C16);

        constexpr int SPACING = 20;
        float pulsePhase = std::fmod(animTime * 35.0f, static_cast<float>(m_height));
        int pulseY = static_cast<int>(pulsePhase);

        for (int x = SPACING; x < m_width; x += SPACING) {
            drawFastVLine(x, 0, m_height - 1, 0xFF0F2636);
        }
        for (int y = SPACING; y < m_height; y += SPACING) {
            drawFastHLine(0, m_width - 1, y, 0xFF1E1330);
        }

        drawFastHLine(0, m_width - 1, pulseY, 0xFF00E5FF);
        if (pulseY > 0) drawFastHLine(0, m_width - 1, pulseY - 1, 0x8800B0D0);
        if (pulseY < m_height - 1) drawFastHLine(0, m_width - 1, pulseY + 1, 0x8800B0D0);

        for (int x = SPACING; x < m_width; x += SPACING) {
            for (int y = SPACING; y < m_height; y += SPACING) {
                float distToPulse = std::abs(static_cast<float>(y - pulseY));
                if (distToPulse < 25.0f) {
                    setPixel(x, y, Colors::NeonCyan);
                } else {
                    setPixel(x, y, 0xFF2A5070);
                }
            }
        }
        return;
    }

    if (style == BackgroundStyle::AuroraGlow) {
        clear(0xFF060F17);

        for (int x = 0; x < m_width; x += 2) {
            float fx = static_cast<float>(x);
            int y1 = static_cast<int>(70.0f + 25.0f * std::sin(fx * 0.02f + animTime * 1.4f));
            int y2 = static_cast<int>(110.0f + 30.0f * std::sin(fx * 0.024f + animTime * 1.1f + 1.2f));
            int y3 = static_cast<int>(150.0f + 25.0f * std::sin(fx * 0.018f + animTime * 0.9f + 2.5f));

            for (int dy = -10; dy <= 10; ++dy) {
                float a = (1.0f - std::abs(dy) / 11.0f) * 0.35f;
                setPixelBlend(x, y1 + dy, 0xFF00FF99, a);
                setPixelBlend(x + 1, y1 + dy, 0xFF00FF99, a);

                setPixelBlend(x, y2 + dy, 0xFF00E5FF, a * 0.9f);
                setPixelBlend(x + 1, y2 + dy, 0xFF00E5FF, a * 0.9f);

                setPixelBlend(x, y3 + dy, 0xFFB300FF, a * 0.8f);
                setPixelBlend(x + 1, y3 + dy, 0xFFB300FF, a * 0.8f);
            }
        }

        for (int s = 0; s < 25; ++s) {
            int sx = (s * 83 + 31) % m_width;
            int sy = (s * 41 + 13) % m_height;
            float tw = 0.4f + 0.4f * std::sin(animTime * 1.8f + s);
            setPixelBlend(sx, sy, 0xFFE2E8F0, tw);
        }
        return;
    }
}

void RasterBuffer::applyScanlineFilter(CrtScanlineMode mode) {
    if (mode == CrtScanlineMode::Off) return;

    float dimFactor = (mode == CrtScanlineMode::Subtle) ? 0.85f : 0.65f;

    for (int y = 0; y < m_height; y += 2) {
        size_t rowStart = static_cast<size_t>(y * m_width);
        for (int x = 0; x < m_width; ++x) {
            uint32_t c = m_pixels[rowStart + static_cast<size_t>(x)];
            uint32_t r = static_cast<uint32_t>(((c >> 16) & 0xFF) * dimFactor);
            uint32_t g = static_cast<uint32_t>(((c >> 8) & 0xFF) * dimFactor);
            uint32_t b = static_cast<uint32_t>((c & 0xFF) * dimFactor);
            m_pixels[rowStart + static_cast<size_t>(x)] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
}

static inline uint32_t computeThemeColor(uint32_t c, PaletteTheme theme) {
    uint32_t r = (c >> 16) & 0xFF;
    uint32_t g = (c >> 8) & 0xFF;
    uint32_t b = c & 0xFF;

    float lum = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;

    if (theme == PaletteTheme::GameBoy) {
        if (lum < 0.25f)      return 0xFF0F380F;
        else if (lum < 0.50f) return 0xFF306230;
        else if (lum < 0.75f) return 0xFF8BAC0F;
        else                  return 0xFF9BBC0F;
    } else if (theme == PaletteTheme::CyberpunkAmber) {
        uint32_t amberR = static_cast<uint32_t>(std::min(255.0f, lum * 255.0f));
        uint32_t amberG = static_cast<uint32_t>(std::min(255.0f, lum * 176.0f));
        uint32_t amberB = static_cast<uint32_t>(std::min(255.0f, lum * 32.0f));
        return 0xFF000000 | (amberR << 16) | (amberG << 8) | amberB;
    } else {
        if (lum < 0.12f) {
            if (theme == PaletteTheme::Gruvbox)            return 0xFF282828;
            else if (theme == PaletteTheme::NeovimDefault) return 0xFF14161B;
            else if (theme == PaletteTheme::DarkOled)       return 0xFF050508;
            return c;
        }

        float maxC = static_cast<float>(std::max({r, g, b}));
        float minC = static_cast<float>(std::min({r, g, b}));
        float delta = maxC - minC;
        float sat = (maxC > 0.001f) ? (delta / maxC) : 0.0f;

        if (sat < 0.18f) {
            if (lum > 0.65f) {
                if (theme == PaletteTheme::Gruvbox)            return 0xFFEBDBB2;
                else if (theme == PaletteTheme::NeovimDefault) return 0xFFE0E2EA;
                else if (theme == PaletteTheme::DarkOled)       return 0xFFF0F4F8;
            } else {
                if (theme == PaletteTheme::Gruvbox)            return 0xFF504945;
                else if (theme == PaletteTheme::NeovimDefault) return 0xFF4F5258;
                else if (theme == PaletteTheme::DarkOled)       return 0xFF1C1D28;
            }
            return c;
        }

        float hue = 0.0f;
        if (delta > 0.001f) {
            if (maxC == r) {
                hue = 60.0f * std::fmod(((g - b) / delta), 6.0f);
            } else if (maxC == g) {
                hue = 60.0f * (((b - r) / delta) + 2.0f);
            } else {
                hue = 60.0f * (((r - g) / delta) + 4.0f);
            }
            if (hue < 0.0f) hue += 360.0f;
        }

        if (theme == PaletteTheme::Gruvbox) {
            if (hue < 20.0f || hue >= 335.0f)     return 0xFFFB4934;
            else if (hue < 50.0f)                  return 0xFFFE8019;
            else if (hue < 85.0f)                  return 0xFFFABD2F;
            else if (hue < 165.0f)                 return 0xFFB8BB26;
            else if (hue < 205.0f)                 return 0xFF8EC07C;
            else if (hue < 270.0f)                 return 0xFF83A598;
            else                                   return 0xFFD3869B;
        } else if (theme == PaletteTheme::NeovimDefault) {
            if (hue < 35.0f || hue >= 330.0f)      return 0xFF8CF8F7;
            else if (hue < 75.0f)                  return 0xFFFCE094;
            else if (hue < 155.0f)                 return 0xFFB3F6C0;
            else if (hue < 195.0f)                 return 0xFF5FD7AF;
            else if (hue < 240.0f)                 return 0xFF70B8FF;
            else if (hue < 290.0f)                 return 0xFFA6DBFF;
            else                                   return 0xFF8CF8F7;
        } else if (theme == PaletteTheme::DarkOled) {
            if (hue < 20.0f || hue >= 335.0f)     return 0xFFFF0055;
            else if (hue < 50.0f)                  return 0xFFFF6600;
            else if (hue < 85.0f)                  return 0xFFFFEE00;
            else if (hue < 165.0f)                 return 0xFF00FF66;
            else if (hue < 210.0f)                 return 0xFF00F0FF;
            else if (hue < 270.0f)                 return 0xFF3377FF;
            else                                   return 0xFFFF00D4;
        }
    }
    return c;
}

void RasterBuffer::applyThemeFilter(PaletteTheme theme) {
    if (theme == PaletteTheme::NeonArcade) return; // Native palette

    struct CacheEntry {
        uint32_t in = 0;
        uint32_t out = 0;
        bool valid = false;
    };
    CacheEntry cache[256];

    for (size_t i = 0; i < m_pixels.size(); ++i) {
        uint32_t c = m_pixels[i];
        size_t idx = ((c >> 16) ^ (c >> 8) ^ c) & 0xFF;
        if (cache[idx].valid && cache[idx].in == c) {
            m_pixels[i] = cache[idx].out;
        } else {
            uint32_t out = computeThemeColor(c, theme);
            cache[idx] = {c, out, true};
            m_pixels[i] = out;
        }
    }
}

void RasterBuffer::applyScreenShake(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    std::vector<uint32_t> temp = m_pixels;
    clear(Colors::Black);
    for (int y = 0; y < m_height; ++y) {
        int srcY = y - dy;
        if (srcY < 0 || srcY >= m_height) continue;
        for (int x = 0; x < m_width; ++x) {
            int srcX = x - dx;
            if (srcX >= 0 && srcX < m_width) {
                m_pixels[static_cast<size_t>(y * m_width + x)] = temp[static_cast<size_t>(srcY * m_width + srcX)];
            }
        }
    }
}

} // namespace Breakout
