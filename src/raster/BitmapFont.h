#pragma once

#include <cstdint>
#include <string_view>

namespace Breakout {

class BitmapFont {
public:
    static constexpr int GLYPH_WIDTH = 8;
    static constexpr int GLYPH_HEIGHT = 8;

    // Returns a pointer to 8 bytes representing 8 rows of 8 bits for ASCII char c
    static const uint8_t* getGlyph(char c);

    // Measure pixel width of text
    static int measureText(std::string_view text, int scale = 1, int spacing = 1);
};

} // namespace Breakout
