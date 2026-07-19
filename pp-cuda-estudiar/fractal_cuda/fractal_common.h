#pragma once

#include <array>
#include <cstdint>

constexpr int kWidth = 1600;
constexpr int kHeight = 900;
constexpr int kMaxIterDefault = 100;
constexpr double kXMin = -2.2;
constexpr double kXMax = 1.2;
constexpr double kYMin = -2.0;
constexpr double kYMax = 1.2;
constexpr int kPaletteSize = 16;

using Pixel = std::uint32_t;

struct FractalParams {
    int width = kWidth;
    int height = kHeight;
    int max_iter = kMaxIterDefault;
    double x_min = kXMin;
    double x_max = kXMax;
    double y_min = kYMin;
    double y_max = kYMax;
};

const std::array<Pixel, kPaletteSize>& palette();

Pixel burning_ship_color(double cr, double ci, int max_iter, const std::array<Pixel, kPaletteSize>& pal);
