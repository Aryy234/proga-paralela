#include "fractal_common.h"

#include <cmath>

Pixel burning_ship_color(double cr, double ci, int max_iter, const std::array<Pixel, kPaletteSize>& pal) {
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (iter < max_iter && (zr * zr + zi * zi) <= 4.0) {
        double ar = std::fabs(zr);
        double ai = std::fabs(zi);
        double next_r = ar * ar - ai * ai + cr;
        double next_i = 2.0 * ar * ai + ci;
        zr = next_r;
        zi = next_i;
        ++iter;
    }

    if (iter >= max_iter) {
        return 0x000000FFu;
    }

    return pal[iter % pal.size()];
}

void render_fractal_cpu(const FractalParams& params, std::uint32_t* pixels) {
    const auto& pal = palette();
    for (int row = 0; row < params.height; ++row) {
        const double ty = static_cast<double>(row) / static_cast<double>(params.height - 1);
        const double cy = params.y_max - ty * (params.y_max - params.y_min);
        for (int col = 0; col < params.width; ++col) {
            const double tx = static_cast<double>(col) / static_cast<double>(params.width - 1);
            const double cx = params.x_min + tx * (params.x_max - params.x_min);
            pixels[row * params.width + col] = burning_ship_color(cx, cy, params.max_iter, pal);
        }
    }
}
