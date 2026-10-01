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
    int err = 0;

    while (x >= y) {
        setPixel(cx + x, cy + y, color);
        setPixel(cx + y, cy + x, color);
        setPixel(cx - y, cy + x, color);
        setPixel(cx - x, cy + y, color);
        setPixel(cx - x, cy - y, color);
        setPixel(cx - y, cy - x, color);
        setPixel(cx + y, cy - x, color);
        setPixel(cx + x, cy - y, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
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
    int err = 0;

    while (x >= y) {
        drawFastHLine(cx - x, cx + x, cy + y, color);
        drawFastHLine(cx - x, cx + x, cy - y, color);
        drawFastHLine(cx - y, cx + y, cy + x, color);
        drawFastHLine(cx - y, cx + y, cy - x, color);

        if (err <= 0) {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2 * x + 1;
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

void RasterBuffer::applyThemeFilter(PaletteTheme theme) {
    if (theme == PaletteTheme::NeonArcade) return; // Native palette

    for (size_t i = 0; i < m_pixels.size(); ++i) {
        uint32_t c = m_pixels[i];
        uint32_t r = (c >> 16) & 0xFF;
        uint32_t g = (c >> 8) & 0xFF;
        uint32_t b = c & 0xFF;

        // Grayscale luminance
        float lum = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;

        if (theme == PaletteTheme::GameBoy) {
            // 4-shade classic Game Boy palette
            if (lum < 0.25f) {
                m_pixels[i] = 0xFF0F380F;
            } else if (lum < 0.50f) {
                m_pixels[i] = 0xFF306230;
            } else if (lum < 0.75f) {
                m_pixels[i] = 0xFF8BAC0F;
            } else {
                m_pixels[i] = 0xFF9BBC0F;
            }
        } else if (theme == PaletteTheme::CyberpunkAmber) {
            // Amber monochrome CRT palette
            uint32_t amberR = static_cast<uint32_t>(std::min(255.0f, lum * 255.0f));
            uint32_t amberG = static_cast<uint32_t>(std::min(255.0f, lum * 176.0f));
            uint32_t amberB = static_cast<uint32_t>(std::min(255.0f, lum * 32.0f));
            m_pixels[i] = 0xFF000000 | (amberR << 16) | (amberG << 8) | amberB;
        } else {
            // Color-aware theme transformations for Gruvbox, NeovimDefault, and DarkOled
            if (lum < 0.12f) {
                // Background dark tones
                if (theme == PaletteTheme::Gruvbox)            m_pixels[i] = 0xFF282828;
                else if (theme == PaletteTheme::NeovimDefault) m_pixels[i] = 0xFF14161B; // Quack/Nvim bg (#14161b)
                else if (theme == PaletteTheme::DarkOled)       m_pixels[i] = 0xFF050508;
                continue;
            }

            float maxC = std::max({r, g, b});
            float minC = std::min({r, g, b});
            float delta = maxC - minC;
            float sat = (maxC > 0.001f) ? (delta / maxC) : 0.0f;

            if (sat < 0.18f) {
                // Neutral gray / text / border tones
                if (lum > 0.65f) {
                    // Highlights & white text
                    if (theme == PaletteTheme::Gruvbox)            m_pixels[i] = 0xFFEBDBB2;
                    else if (theme == PaletteTheme::NeovimDefault) m_pixels[i] = 0xFFE0E2EA; // Quack/Nvim Normal fg (#e0e2ea)
                    else if (theme == PaletteTheme::DarkOled)       m_pixels[i] = 0xFFF0F4F8;
                } else {
                    // Border walls & dark grays
                    if (theme == PaletteTheme::Gruvbox)            m_pixels[i] = 0xFF504945;
                    else if (theme == PaletteTheme::NeovimDefault) m_pixels[i] = 0xFF4F5258; // Quack/Nvim LineNr/border (#4f5258)
                    else if (theme == PaletteTheme::DarkOled)       m_pixels[i] = 0xFF1C1D28;
                }
                continue;
            }

            // Hue calculation in degrees [0, 360)
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
                if (hue < 20.0f || hue >= 335.0f)     m_pixels[i] = 0xFFFB4934; // Red
                else if (hue < 50.0f)                  m_pixels[i] = 0xFFFE8019; // Orange
                else if (hue < 85.0f)                  m_pixels[i] = 0xFFFABD2F; // Yellow
                else if (hue < 165.0f)                 m_pixels[i] = 0xFFB8BB26; // Green
                else if (hue < 205.0f)                 m_pixels[i] = 0xFF8EC07C; // Aqua
                else if (hue < 270.0f)                 m_pixels[i] = 0xFF83A598; // Blue
                else                                   m_pixels[i] = 0xFFD3869B; // Purple
            } else if (theme == PaletteTheme::NeovimDefault) {
                // Quack/Neovim palette mapping
                if (hue < 20.0f || hue >= 335.0f)     m_pixels[i] = 0xFFFF5F5F; // Red (Removed/Error #ff5f5f)
                else if (hue < 50.0f)                  m_pixels[i] = 0xFFFFD787; // Amber/Orange (dirSize #ffd787)
                else if (hue < 85.0f)                  m_pixels[i] = 0xFFFCE094; // Yellow (Warning/Search #fce094)
                else if (hue < 165.0f)                 m_pixels[i] = 0xFFB3F6C0; // Green (String/ModeMsg #b3f6c0)
                else if (hue < 205.0f)                 m_pixels[i] = 0xFF8CF8F7; // Cyan (Directory/Special #8cf8f7)
                else if (hue < 270.0f)                 m_pixels[i] = 0xFFA6DBFF; // Blue (Identifier #a6dbff)
                else                                   m_pixels[i] = 0xFFD787D7; // Magenta (PmenuMatch #d787d7)
            } else if (theme == PaletteTheme::DarkOled) {
                if (hue < 20.0f || hue >= 335.0f)     m_pixels[i] = 0xFFFF0055; // Neon Red
                else if (hue < 50.0f)                  m_pixels[i] = 0xFFFF6600; // Neon Orange
                else if (hue < 85.0f)                  m_pixels[i] = 0xFFFFEE00; // Bright Yellow
                else if (hue < 165.0f)                 m_pixels[i] = 0xFF00FF66; // Bright Green
                else if (hue < 210.0f)                 m_pixels[i] = 0xFF00F0FF; // Ice Cyan
                else if (hue < 270.0f)                 m_pixels[i] = 0xFF3377FF; // Vivid Blue
                else                                   m_pixels[i] = 0xFFFF00D4; // Hot Pink
            }
        }
    }
}

} // namespace Breakout
