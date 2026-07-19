#include "fractal_newton.h"

#include <SFML/Graphics.hpp>

#include <omp.h>

#include <chrono>
#include <cmath>
#include <limits>

#include "palette.h"

struct Complejo {
    double re;
    double im;
};

static const Complejo raices[3] = {
    {1.0, 0.0},
    {-0.5, 0.86602540378443864676},
    {-0.5, -0.86602540378443864676}
};

static inline double modulo_cuadrado(const Complejo& z) {
    return z.re * z.re + z.im * z.im;
}

static inline Complejo paso_newton(const Complejo& z) {
    const double a = z.re;
    const double b = z.im;
    const double a2 = a * a;
    const double b2 = b * b;
    const double denom = 3.0 * (a2 - b2);
    const double denom_im = 6.0 * a * b;
    const double denom_mod = denom * denom + denom_im * denom_im;

    if (denom_mod < 1e-30) {
        return {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    }

    const double z3_re = a * (a2 - 3.0 * b2);
    const double z3_im = b * (3.0 * a2 - b2);

    const double num_re = z3_re - 1.0;
    const double num_im = z3_im;

    const double frac_re = (num_re * denom + num_im * denom_im) / denom_mod;
    const double frac_im = (num_im * denom - num_re * denom_im) / denom_mod;

    return {a - frac_re, b - frac_im};
}

static int clasificar_punto(double x, double y, int max_iter, int& iteraciones) {
    Complejo z{x, y};
    iteraciones = 0;

    for (int iter = 0; iter < max_iter; ++iter) {
        if (modulo_cuadrado(z) > 4.0) {
            iteraciones = iter;
            return -1;
        }

        Complejo siguiente = paso_newton(z);
        if (!std::isfinite(siguiente.re) || !std::isfinite(siguiente.im)) {
            iteraciones = iter + 1;
            return -1;
        }

        z = siguiente;
        iteraciones = iter + 1;

        for (int raiz = 0; raiz < 3; ++raiz) {
            const double dr = z.re - raices[raiz].re;
            const double di = z.im - raices[raiz].im;
            if (dr * dr + di * di < 1e-8) {
                return raiz;
            }
        }
    }

    return -1;
}

static sf::Color color_desde_uint32(uint32_t valor) {
    const std::uint8_t a = static_cast<std::uint8_t>((valor >> 24) & 0xFF);
    const std::uint8_t r = static_cast<std::uint8_t>((valor >> 16) & 0xFF);
    const std::uint8_t g = static_cast<std::uint8_t>((valor >> 8) & 0xFF);
    const std::uint8_t b = static_cast<std::uint8_t>(valor & 0xFF);
    return sf::Color(r, g, b, a);
}

ResultadoFractal dibujar_fractal(const ConfiguracionFractal& config, std::vector<std::uint8_t>& pixeles) {
    ResultadoFractal resultado{};
    auto inicio = std::chrono::steady_clock::now();

    const double escala_x = (config.xmax - config.xmin) / static_cast<double>(config.ancho - 1);
    const double escala_y = (config.ymax - config.ymin) / static_cast<double>(config.alto - 1);

    long long total_iters = 0;

    #pragma omp parallel for schedule(static) reduction(+:total_iters)
    for (int y = 0; y < config.alto; ++y) {
        const double cy = config.ymax - static_cast<double>(y) * escala_y;
        for (int x = 0; x < config.ancho; ++x) {
            const double cx = config.xmin + static_cast<double>(x) * escala_x;
            int iteraciones = 0;
            const int raiz = clasificar_punto(cx, cy, config.max_iter, iteraciones);

            uint32_t color = 0xFF000000u;
            if (raiz >= 0) {
                const int indice_color = (raiz * 5 + iteraciones) % PALETTE_SIZE;
                color = color_ramp[static_cast<size_t>(indice_color)];
            }

            const sf::Color rgba = color_desde_uint32(color);
            const std::size_t indice = static_cast<std::size_t>(y * config.ancho + x) * 4u;
            pixeles[indice + 0] = rgba.r;
            pixeles[indice + 1] = rgba.g;
            pixeles[indice + 2] = rgba.b;
            pixeles[indice + 3] = rgba.a;
            total_iters += iteraciones;
        }
    }

    auto fin = std::chrono::steady_clock::now();
    resultado.tiempo_ms = std::chrono::duration<double, std::milli>(fin - inicio).count();
    resultado.total_iters = total_iters;
    return resultado;
}
