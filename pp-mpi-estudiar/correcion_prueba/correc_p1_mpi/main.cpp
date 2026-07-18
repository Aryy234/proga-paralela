#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>

#include <fmt/format.h>
#include <mpi.h>
#include <SFML/Graphics.hpp>

#include "draw_text.h"
#include "fractal.h"

#define WIDTH 1600
#define HEIGHT 900

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);

    int rank = 0;
    int nprocs = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int delta = (HEIGHT + nprocs - 1) / nprocs;
    int fila_inicial = rank * delta;
    int fila_final = std::min(fila_inicial + delta, HEIGHT);
    if (fila_inicial > HEIGHT) {
        fila_inicial = HEIGHT;
    }
    int filas_locales = fila_final - fila_inicial;
    int capacidad_pixeles = WIDTH * delta;
    uint32_t* buffer_pixeles = new uint32_t[capacidad_pixeles];

    int max_iteraciones = 50;
    int running = 1;
    double x_min = -1.5;
    double x_max = 1.5;
    double y_min = -1.0;
    double y_max = 1.0;

    sf::RenderWindow* ventana = nullptr;
    sf::Texture* textura = nullptr;
    sf::Sprite* sprite = nullptr;
    sf::Font* fuente = nullptr;
    uint32_t* imagen_completa = nullptr;

    if (rank == 0) {
        ventana = new sf::RenderWindow(sf::VideoMode({WIDTH, HEIGHT}),
                                       "Fractal de Newton con MPI");
        ventana->setFramerateLimit(60);
        textura = new sf::Texture({WIDTH, HEIGHT});
        sprite = new sf::Sprite(*textura);
        fuente = new sf::Font();
        cargar_fuente_arial(*fuente);
        imagen_completa = new uint32_t[WIDTH * HEIGHT];
    }

    auto instante_fps = std::chrono::steady_clock::now();

    while (true) {
        if (rank == 0) {
            while (const std::optional evento = ventana->pollEvent()) {
                if (evento->is<sf::Event::Closed>()) {
                    running = 0;
                }
                if (const auto* tecla = evento->getIf<sf::Event::KeyPressed>()) {
                    if (tecla->code == sf::Keyboard::Key::Escape) {
                        running = 0;
                    } else if (tecla->code == sf::Keyboard::Key::Up) {
                        max_iteraciones += 10;
                    } else if (tecla->code == sf::Keyboard::Key::Down) {
                        max_iteraciones = std::max(10, max_iteraciones - 10);
                    }
                }
            }
        }

        std::array<int, 2> estado = {max_iteraciones, running};
        std::array<double, 4> plano = {x_min, x_max, y_min, y_max};
        MPI_Bcast(estado.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(plano.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        max_iteraciones = estado[0];
        running = estado[1];
        x_min = plano[0];
        x_max = plano[1];
        y_min = plano[2];
        y_max = plano[3];

        if (running == 0) {
            break;
        }

        double inicio_calculo = MPI_Wtime();
        long long iteraciones_locales = 0;
        calcular_fractal_newton(x_min, y_min, x_max, y_max,
                                WIDTH, HEIGHT, fila_inicial, fila_final,
                                max_iteraciones, buffer_pixeles,
                                iteraciones_locales);
        double tiempo_local_ms = (MPI_Wtime() - inicio_calculo) * 1000.0;

        if (rank == 0) {
            std::memcpy(imagen_completa + fila_inicial * WIDTH,
                        buffer_pixeles,
                        filas_locales * WIDTH * sizeof(uint32_t));

            for (int proceso = 1; proceso < nprocs; proceso++) {
                int inicio_proceso = std::min(proceso * delta, HEIGHT);
                int final_proceso = std::min(inicio_proceso + delta, HEIGHT);
                int pixeles_proceso = (final_proceso - inicio_proceso) * WIDTH;
                MPI_Recv(buffer_pixeles, pixeles_proceso, MPI_UINT32_T,
                         proceso, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                std::memcpy(imagen_completa + inicio_proceso * WIDTH,
                            buffer_pixeles,
                            pixeles_proceso * sizeof(uint32_t));
            }
        } else {
            MPI_Send(buffer_pixeles, filas_locales * WIDTH, MPI_UINT32_T,
                     0, 0, MPI_COMM_WORLD);
        }

        double max_compute_ms = 0.0;
        long long total_iteraciones = 0;
        MPI_Reduce(&tiempo_local_ms, &max_compute_ms, 1, MPI_DOUBLE,
                   MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&iteraciones_locales, &total_iteraciones, 1, MPI_LONG_LONG,
                   MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            textura->update(reinterpret_cast<const std::uint8_t*>(imagen_completa));

            auto ahora = std::chrono::steady_clock::now();
            double segundos = std::chrono::duration<double>(ahora - instante_fps).count();
            double fps = 1.0 / std::max(segundos, 1.0e-9);
            instante_fps = ahora;

            sf::Text informacion(*fuente,
                fmt::format("RANKS: {}  max_iter: {}  max_compute_ms: {:.2f}  total_iters: {}  FPS: {:.1f}",
                            nprocs, max_iteraciones, max_compute_ms,
                            total_iteraciones, fps), 20);
            informacion.setPosition({12.0f, 8.0f});
            informacion.setFillColor(sf::Color::White);
            informacion.setOutlineColor(sf::Color::Black);
            informacion.setOutlineThickness(2.0f);

            sf::Text ayuda(*fuente, "Up/Down: cambiar max_iter    Esc: cerrar", 20);
            ayuda.setPosition({12.0f, HEIGHT - 34.0f});
            ayuda.setFillColor(sf::Color::White);
            ayuda.setOutlineColor(sf::Color::Black);
            ayuda.setOutlineThickness(2.0f);

            ventana->clear();
            ventana->draw(*sprite);
            ventana->draw(informacion);
            ventana->draw(ayuda);
            ventana->display();
        }
    }

    delete[] buffer_pixeles;
    if (rank == 0) {
        delete[] imagen_completa;
        delete fuente;
        delete sprite;
        delete textura;
        delete ventana;
    }

    MPI_Finalize();
    return 0;
}
