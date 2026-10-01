#pragma once

#include "core/Config.h"
#include <QImage>
#include <vector>
#include <string_view>
#include <cstdint>

namespace Breakout {

class RasterBuffer {
public:
    RasterBuffer(int width = VIRTUAL_WIDTH, int height = VIRTUAL_HEIGHT);

    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] uint32_t* data() { return m_pixels.data(); }
    [[nodiscard]] const uint32_t* data() const { return m_pixels.data(); }
    [[nodiscard]] const QImage& qimage() const { return m_image; }

    void clear(uint32_t color = Colors::Black);

    inline void setPixel(int x, int y, uint32_t color) {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            m_pixels[static_cast<size_t>(y * m_width + x)] = color;
        }
    }

    inline void setPixelBlend(int x, int y, uint32_t color, float alpha) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height || alpha <= 0.0f) return;
        if (alpha >= 1.0f) {
            setPixel(x, y, color);
            return;
        }
        size_t idx = static_cast<size_t>(y * m_width + x);
        uint32_t bg = m_pixels[idx];

        uint32_t r_src = (color >> 16) & 0xFF;
        uint32_t g_src = (color >> 8) & 0xFF;
        uint32_t b_src = color & 0xFF;

        uint32_t r_bg = (bg >> 16) & 0xFF;
        uint32_t g_bg = (bg >> 8) & 0xFF;
        uint32_t b_bg = bg & 0xFF;

        uint32_t r = static_cast<uint32_t>(r_src * alpha + r_bg * (1.0f - alpha));
        uint32_t g = static_cast<uint32_t>(g_src * alpha + g_bg * (1.0f - alpha));
        uint32_t b = static_cast<uint32_t>(b_src * alpha + b_bg * (1.0f - alpha));

        m_pixels[idx] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    [[nodiscard]] inline uint32_t getPixel(int x, int y) const {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            return m_pixels[static_cast<size_t>(y * m_width + x)];
        }
        return 0;
    }

    void drawFastHLine(int x1, int x2, int y, uint32_t color);
    void drawFastVLine(int x, int y1, int y2, uint32_t color);
    void drawRect(int x, int y, int w, int h, uint32_t color);
    void fillRect(int x, int y, int w, int h, uint32_t color);
    void drawLine(int x0, int y0, int x1, int y1, uint32_t color);
    void drawCircle(int cx, int cy, int radius, uint32_t color);
    void fillCircle(int cx, int cy, int radius, uint32_t color);

    void drawBitmapText(int x, int y, std::string_view text, uint32_t color, int scale = 1, int spacing = 1);
    void drawBitmapTextCentered(int y, std::string_view text, uint32_t color, int scale = 1);

    // Retro post-processing filter passes
    void applyScanlineFilter(CrtScanlineMode mode);
    void applyThemeFilter(PaletteTheme theme);

private:
    int m_width;
    int m_height;
    std::vector<uint32_t> m_pixels;
    QImage m_image;
};

} // namespace Breakout
