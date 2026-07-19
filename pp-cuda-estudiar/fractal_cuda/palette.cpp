#include "fractal_common.h"

const std::array<Pixel, kPaletteSize>& palette() {
    static const std::array<Pixel, kPaletteSize> kPalette = {
        0x071821FFu, 0x1B2632FFu, 0x31293BFFu, 0x5B2942FFu,
        0x8E2B48FFu, 0xC5334AFFu, 0xF05D3FFFu, 0xF9A03FFFu,
        0xF7D354FFu, 0xD9E76CFFu, 0x84C318FFu, 0x2F7D32FFu,
        0x1D5B79FFu, 0x2667A0FFu, 0x4A90E2FFu, 0xF2F2F2FFu
    };
    return kPalette;
}
