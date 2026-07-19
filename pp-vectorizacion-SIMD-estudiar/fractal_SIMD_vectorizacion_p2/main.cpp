#include "fractal.h"
#include "params.h"
#include "arial_ttf.h"

#include <SFML/Graphics.hpp>
#include <fmt/core.h>

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <optional>

enum class ModoCalculo
{
    Escalar,
    SIMD
};

static void recalcular_fractal(std::vector<uint32_t>& pixeles, ModoCalculo modo, int max_iter_actual)
{
    if (modo == ModoCalculo::SIMD) {
        calcular_burning_ship_simd(
            pixeles.data(),
            ancho_imagen,
            alto_imagen,
            max_iter_actual,
            xmin,
            xmax,
            ymin,
            ymax
        );
    } else {
        calcular_burning_ship_escalar(
            pixeles.data(),
            ancho_imagen,
            alto_imagen,
            max_iter_actual,
            xmin,
            xmax,
            ymin,
            ymax
        );
    }
}

int main()
{
    sf::RenderWindow ventana(sf::VideoMode({static_cast<unsigned int>(ancho_imagen), static_cast<unsigned int>(alto_imagen)}), "Burning Ship SIMD");
    ventana.setFramerateLimit(60);

    sf::Font fuente;
    if (!fuente.openFromMemory(arial_ttf::data, arial_ttf::data_len)) {
        return 1;
    }

    std::vector<uint32_t> pixeles(static_cast<std::size_t>(ancho_imagen) * static_cast<std::size_t>(alto_imagen));
    sf::Texture textura;
    textura.resize({static_cast<unsigned int>(ancho_imagen), static_cast<unsigned int>(alto_imagen)});
    sf::Sprite sprite(textura);

    sf::Text texto_overlay(fuente);
    texto_overlay.setCharacterSize(18);
    texto_overlay.setFillColor(sf::Color::White);
    texto_overlay.setPosition({12.0f, 10.0f});

    sf::RectangleShape fondo_texto;
    fondo_texto.setFillColor(sf::Color(0, 0, 0, 160));
    fondo_texto.setPosition({8.0f, 6.0f});
    fondo_texto.setSize({580.0f, 70.0f});

    ModoCalculo modo = ModoCalculo::SIMD;
    int max_iter_actual = max_iter_default;
    bool necesita_recalculo = true;
    double tiempo_ms = 0.0;

    sf::Clock reloj_fps;
    double fps = 0.0;

    while (ventana.isOpen()) {
        while (const std::optional<sf::Event> evento = ventana.pollEvent()) {
            if (evento->is<sf::Event::Closed>()) {
                ventana.close();
            }

            if (const auto* tecla = evento->getIf<sf::Event::KeyPressed>()) {
                if (tecla->code == sf::Keyboard::Key::Escape) {
                    ventana.close();
                }

                if (tecla->code == sf::Keyboard::Key::Space) {
                    modo = (modo == ModoCalculo::SIMD) ? ModoCalculo::Escalar : ModoCalculo::SIMD;
                    necesita_recalculo = true;
                }

                if (tecla->code == sf::Keyboard::Key::Up) {
                    max_iter_actual += 10;
                    necesita_recalculo = true;
                }

                if (tecla->code == sf::Keyboard::Key::Down) {
                    if (max_iter_actual > 10) {
                        max_iter_actual -= 10;
                    } else {
                        max_iter_actual = 1;
                    }
                    necesita_recalculo = true;
                }
            }
        }

        if (necesita_recalculo) {
            const auto inicio = std::chrono::high_resolution_clock::now();
            recalcular_fractal(pixeles, modo, max_iter_actual);
            const auto fin = std::chrono::high_resolution_clock::now();
            tiempo_ms = std::chrono::duration<double, std::milli>(fin - inicio).count();
            textura.update(reinterpret_cast<const std::uint8_t*>(pixeles.data()));
            necesita_recalculo = false;
        }

        const float delta_segundos = reloj_fps.restart().asSeconds();
        if (delta_segundos > 0.0f) {
            fps = 1.0 / static_cast<double>(delta_segundos);
        }

        const char* nombre_modo = (modo == ModoCalculo::SIMD) ? "SIMD" : "Escalar";
        texto_overlay.setString(fmt::format(
            "Modo: {}\nmax_iter: {}\ntiempo_ms: {:.2f}\nFPS: {:.1f}\nTeclas: Space cambia modo, Up/Down ajustan, Esc sale",
            nombre_modo,
            max_iter_actual,
            tiempo_ms,
            fps
        ));

        ventana.clear();
        ventana.draw(sprite);
        ventana.draw(fondo_texto);
        ventana.draw(texto_overlay);
        ventana.display();
    }

    return 0;
}
