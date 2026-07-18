#include "fractal.h"
#include "palette.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

extern int max_iteraciones;

static uint32_t calcular_color_burning_ship(double cx, double cy, int& iteracion)
{
    double zr = 0.0;
    double zi = 0.0;
    iteracion = 0;

    while (iteracion < max_iteraciones && (zr * zr + zi * zi) <= 4.0) {
        double ar = std::abs(zr);
        double ai = std::abs(zi);

        double next_zr = ar * ar - ai * ai + cx;
        double next_zi = 2.0 * ar * ai + cy;

        zr = next_zr;
        zi = next_zi;
        iteracion++;
    }

    if (iteracion >= max_iteraciones) {
        return 0xFF000000;
    }

    return color_ramp[iteracion % PALETTE_SIZE];
}

void burning_mpi(double x_min, double y_min, double x_max, double y_max,
                 uint32_t width, uint32_t height,
                 uint32_t row_start, uint32_t row_end,
                 uint32_t* pixel_buffer, int* histograma_local)
{
    for (int bin = 0; bin < PALETTE_SIZE; bin++) {
        histograma_local[bin] = 0;
    }

    double dx = 0.0;
    double dy = 0.0;
    if (width > 1) {
        dx = (x_max - x_min) / (width - 1.0);
    }
    if (height > 1) {
        dy = (y_max - y_min) / (height - 1.0);
    }

    row_end = std::min(row_end, height);

    for (uint32_t row = row_start; row < row_end; row++) {
        double cy = y_max - row * dy;
        for (uint32_t col = 0; col < width; col++) {
            double cx = x_min + col * dx;
            int iteracion = 0;
            uint32_t color = calcular_color_burning_ship(cx, cy, iteracion);
            pixel_buffer[(row - row_start) * width + col] = color;

            if (iteracion < max_iteraciones) {
                int bin = iteracion * PALETTE_SIZE / max_iteraciones;
                if (bin >= PALETTE_SIZE) {
                    bin = PALETTE_SIZE - 1;
                }
                histograma_local[bin]++;
            }
        }
    }
}
