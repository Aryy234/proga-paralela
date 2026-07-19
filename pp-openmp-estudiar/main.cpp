#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <omp.h>

#include <chrono>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <fmt/core.h>
#include <sstream>
#include <string>
#include <vector>

#include "arial_ttf.h"
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
    const sf::Uint8 a = static_cast<sf::Uint8>((valor >> 24) & 0xFF);
    const sf::Uint8 r = static_cast<sf::Uint8>((valor >> 16) & 0xFF);
    const sf::Uint8 g = static_cast<sf::Uint8>((valor >> 8) & 0xFF);
    const sf::Uint8 b = static_cast<sf::Uint8>(valor & 0xFF);
    return sf::Color(r, g, b, a);
}

static void dibujar_fractal(std::vector<sf::Uint8>& pixeles, int width, int height,
                            double xmin, double xmax, double ymin, double ymax,
                            int max_iter, int& total_iters, double& tiempo_ms) {
    total_iters = 0;
    auto inicio = std::chrono::steady_clock::now();

    const double escala_x = (xmax - xmin) / static_cast<double>(width - 1);
    const double escala_y = (ymax - ymin) / static_cast<double>(height - 1);

    #pragma omp parallel for schedule(static) reduction(+:total_iters)
    for (int y = 0; y < height; ++y) {
        const double cy = ymax - static_cast<double>(y) * escala_y;
        for (int x = 0; x < width; ++x) {
            const double cx = xmin + static_cast<double>(x) * escala_x;
            int iteraciones = 0;
            const int raiz = clasificar_punto(cx, cy, max_iter, iteraciones);

            uint32_t color = 0xFF000000u;
            if (raiz >= 0) {
                const int indice_color = (raiz * 5 + iteraciones) % PALETTE_SIZE;
                color = color_ramp[static_cast<size_t>(indice_color)];
            }

            const sf::Color rgba = color_desde_uint32(color);
            const std::size_t indice = static_cast<std::size_t>(y * width + x) * 4u;
            pixeles[indice + 0] = rgba.r;
            pixeles[indice + 1] = rgba.g;
            pixeles[indice + 2] = rgba.b;
            pixeles[indice + 3] = rgba.a;
            total_iters += iteraciones;
        }
    }

    auto fin = std::chrono::steady_clock::now();
    tiempo_ms = std::chrono::duration<double, std::milli>(fin - inicio).count();
}

static bool cargar_fuente(sf::Font& fuente) {
    return fuente.loadFromMemory(arial_ttf::data, arial_ttf::data_len);
}

int main() {
    const int ancho = 1600;
    const int alto = 900;

    sf::RenderWindow ventana(sf::VideoMode(ancho, alto), "Fractal de Newton - OpenMP");
    ventana.setFramerateLimit(60);

    sf::Font fuente;
    if (!cargar_fuente(fuente)) {
        return EXIT_FAILURE;
    }

    sf::Texture textura;
    textura.create(static_cast<unsigned int>(ancho), static_cast<unsigned int>(alto));

    sf::Sprite sprite(textura);

    std::vector<sf::Uint8> pixeles(static_cast<std::size_t>(ancho * alto) * 4u, 0);
    sf::Text texto_superior;
    texto_superior.setFont(fuente);
    texto_superior.setCharacterSize(18);
    texto_superior.setFillColor(sf::Color::White);
    texto_superior.setPosition(16.0f, 10.0f);

    sf::Text texto_inferior;
    texto_inferior.setFont(fuente);
    texto_inferior.setCharacterSize(16);
    texto_inferior.setFillColor(sf::Color(240, 240, 240));
    texto_inferior.setPosition(16.0f, static_cast<float>(alto) - 48.0f);

    int max_iter = 50;
    double xmin = -1.5;
    double xmax = 1.5;
    double ymin = -1.0;
    double ymax = 1.0;

    int total_iters = 0;
    double tiempo_ms = 0.0;
    double fps = 0.0;
    bool recomputar = true;

    sf::Clock reloj_fps;
    sf::Clock reloj_frame;

    while (ventana.isOpen()) {
        sf::Event evento{};
        while (ventana.pollEvent(evento)) {
            if (evento.type == sf::Event::Closed) {
                ventana.close();
            } else if (evento.type == sf::Event::KeyPressed) {
                if (evento.key.code == sf::Keyboard::Escape) {
                    ventana.close();
                } else if (evento.key.code == sf::Keyboard::Up) {
                    max_iter += 5;
                    recomputar = true;
                } else if (evento.key.code == sf::Keyboard::Down) {
                    if (max_iter > 5) {
                        max_iter -= 5;
                        recomputar = true;
                    }
                }
            }
        }

        if (recomputar) {
            dibujar_fractal(pixeles, ancho, alto, xmin, xmax, ymin, ymax, max_iter, total_iters, tiempo_ms);
            textura.update(pixeles.data());
            recomputar = false;
        }

        const float frame_dt = reloj_frame.restart().asSeconds();
        if (frame_dt > 0.0f) {
            fps = 1.0 / static_cast<double>(frame_dt);
        }

        const int hilos = omp_get_max_threads();
        const std::string info_superior = fmt::format(
            "OMP_THREADS: {}  max_iter: {}  max_compute_ms: {:.2f}  total_iters: {}  FPS: {:.2f}",
            hilos, max_iter, tiempo_ms, total_iters, fps);
        const std::string ayuda = "Up/Down: cambiar max_iter   Esc: cerrar";

        texto_superior.setString(info_superior);
        texto_inferior.setString(ayuda);

        ventana.clear(sf::Color::Black);
        ventana.draw(sprite);

        sf::RectangleShape banda_superior(sf::Vector2f(static_cast<float>(ancho), 36.0f));
        banda_superior.setFillColor(sf::Color(0, 0, 0, 120));
        ventana.draw(banda_superior);

        sf::RectangleShape banda_inferior(sf::Vector2f(static_cast<float>(ancho), 36.0f));
        banda_inferior.setPosition(0.0f, static_cast<float>(alto) - 42.0f);
        banda_inferior.setFillColor(sf::Color(0, 0, 0, 120));
        ventana.draw(banda_inferior);

        ventana.draw(texto_superior);
        ventana.draw(texto_inferior);
        ventana.display();
    }

    return EXIT_SUCCESS;
}
