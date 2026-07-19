#include "interfaz.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <omp.h>

#include <fmt/core.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "arial_ttf.h"
#include "fractal_newton.h"

static bool cargar_fuente(sf::Font& fuente) {
    return fuente.openFromMemory(arial_ttf::data, arial_ttf::data_len);
}

void ejecutar_aplicacion() {
    const int ancho = 1600;
    const int alto = 900;

    sf::RenderWindow ventana(sf::VideoMode({static_cast<unsigned int>(ancho), static_cast<unsigned int>(alto)}), "Fractal de Newton - OpenMP");
    ventana.setFramerateLimit(60);

    sf::Font fuente;
    if (!cargar_fuente(fuente)) {
        std::exit(EXIT_FAILURE);
    }

    sf::Texture textura;
    if (!textura.resize({static_cast<unsigned int>(ancho), static_cast<unsigned int>(alto)})) {
        std::exit(EXIT_FAILURE);
    }
    sf::Sprite sprite(textura);

    std::vector<std::uint8_t> pixeles(static_cast<std::size_t>(ancho * alto) * 4u, 0);

    sf::Text texto_superior(fuente);
    texto_superior.setCharacterSize(18);
    texto_superior.setFillColor(sf::Color::White);
    texto_superior.setPosition({16.0f, 10.0f});

    sf::Text texto_inferior(fuente);
    texto_inferior.setCharacterSize(16);
    texto_inferior.setFillColor(sf::Color(240, 240, 240));
    texto_inferior.setPosition({16.0f, static_cast<float>(alto) - 48.0f});

    ConfiguracionFractal config{ancho, alto, 50, -1.5, 1.5, -1.0, 1.0};
    ResultadoFractal resultado{};
    double fps = 0.0;
    bool recomputar = true;

    sf::Clock reloj_frame;

    while (ventana.isOpen()) {
        while (const auto evento = ventana.pollEvent()) {
            if (evento->is<sf::Event::Closed>()) {
                ventana.close();
            } else if (const auto* tecla = evento->getIf<sf::Event::KeyPressed>()) {
                if (tecla->scancode == sf::Keyboard::Scancode::Escape) {
                    ventana.close();
                } else if (tecla->scancode == sf::Keyboard::Scancode::Up) {
                    config.max_iter += 5;
                    recomputar = true;
                } else if (tecla->scancode == sf::Keyboard::Scancode::Down) {
                    if (config.max_iter > 5) {
                        config.max_iter -= 5;
                        recomputar = true;
                    }
                }
            }
        }

        if (recomputar) {
            resultado = dibujar_fractal(config, pixeles);
            textura.update(pixeles.data());
            recomputar = false;
        }

        const float frame_dt = reloj_frame.restart().asSeconds();
        if (frame_dt > 0.0f) {
            fps = 1.0 / static_cast<double>(frame_dt);
        }

        const int hilos = omp_get_max_threads();
        texto_superior.setString(fmt::format(
            "OMP_THREADS: {}  max_iter: {}  max_compute_ms: {:.2f}  total_iters: {}  FPS: {:.2f}",
            hilos, config.max_iter, resultado.tiempo_ms, resultado.total_iters, fps));
        texto_inferior.setString("Up/Down: cambiar max_iter   Esc: cerrar");

        ventana.clear(sf::Color::Black);
        ventana.draw(sprite);

        sf::RectangleShape banda_superior({static_cast<float>(ancho), 36.0f});
        banda_superior.setFillColor(sf::Color(0, 0, 0, 120));
        ventana.draw(banda_superior);

        sf::RectangleShape banda_inferior({static_cast<float>(ancho), 36.0f});
        banda_inferior.setPosition({0.0f, static_cast<float>(alto) - 42.0f});
        banda_inferior.setFillColor(sf::Color(0, 0, 0, 120));
        ventana.draw(banda_inferior);

        ventana.draw(texto_superior);
        ventana.draw(texto_inferior);
        ventana.display();
    }
}
