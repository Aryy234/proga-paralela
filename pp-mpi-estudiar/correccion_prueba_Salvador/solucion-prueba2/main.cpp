#include <iostream>

#include <fmt/core.h>
#include <fmt/ranges.h>
#include <SFML/Graphics.hpp>
#include <complex>

#include <mpi.h>

#include "fractal_newton.h"
#include "draw_text.h"

#ifdef _WIN32
    #include <windows.h>
#endif  

namespace arial_ttf 
{
    extern size_t data_len;
    extern unsigned char data[];
}

//dimension de la imagen
#define WIDTH 1600
#define HEIGHT 900

uint32_t* pixel_buffer = nullptr;
uint32_t* texture_buffer = nullptr; //solo RANK_0

double compute_time;
double compute_time_ms;
double total_iters;

int running = 1;

int nprocs;
int rank;

int row_start;
int row_end;
int padding;
int delta;

double epsilon = 0.0001f;

std::string machine_name() {
    //--machine name
    std::string mname = "";
#ifdef _WIN32
    char hostname[256];
    DWORD size = sizeof(hostname);
    GetComputerNameA(hostname, &size);
    mname = hostname;
#endif
    return mname;
}

void dibujar_texto(int rank) {
    auto texto = fmt::format("RANK_{}==>{}", rank, machine_name());

    draw_text_to_texture(
        (unsigned char*)pixel_buffer, 
        WIDTH, delta, 
        texto.c_str(), 
        10, 25, 20);
}

void setup_ui( ) {
    //-- parametros Newton
    double x_min = -1.5;
    double x_max = 1.5;
    double y_min = -1;
    double y_max = 1;

    int max_iteraciones = 50;

    texture_buffer = new uint32_t[WIDTH*HEIGHT];
    std::memset(texture_buffer, 0, WIDTH*HEIGHT*sizeof(uint32_t));

    //inicializar la UI
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Fractal - MPI");

#ifdef _WIN32
    HWND hwnd = window.getNativeHandle();
    ShowWindow(hwnd, SW_MAXIMIZE);
#endif   

    sf::Texture texture({WIDTH, HEIGHT});
    texture.update((const uint8_t *)texture_buffer);
    sf::Sprite sprite(texture);

    //textos
    const sf::Font font(arial_ttf::data, arial_ttf::data_len);

    sf::Text text(font, "Fractal", 24);
    text.setFillColor(sf::Color::White);
    text.setPosition({10,10});
    text.setStyle(sf::Text::Bold);

    std::string options ="UP/DOWN: Iterations";
    sf::Text textOptions(font, options, 18);;
    textOptions.setFillColor(sf::Color::White);
    textOptions.setStyle(sf::Text::Bold);
    textOptions.setPosition({10, window.getView().getSize().y - 40});

    //FPS
    int frames = 0;
    int fps = 0;
    sf::Clock clock;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>()) {
                running = 0;
                window.close();
            }
            else if(event->is<sf::Event::KeyReleased>()) {
                auto evt = event->getIf<sf::Event::KeyReleased>();

                switch(evt->scancode) {
                    case sf::Keyboard::Scan::Up:
                        max_iteraciones += 10;
                        break;
                    case sf::Keyboard::Scan::Down:
                        max_iteraciones -= 10;
                        if(max_iteraciones < 10) max_iteraciones = 10;
                        break;
                }

                std::memset(texture_buffer, 0, WIDTH*HEIGHT*sizeof(uint32_t));
            }
        }

        //notificar a los otros RANKs que la app se está cerrando
        std::vector<double> dummy = {max_iteraciones*1.0, x_min, x_max, y_min , y_max, running*1.0};
        MPI_Bcast(dummy.data(), 6, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        if(running == 0) {
            break;
        }

        //-- dibujar la porcion del RANK_0
        double start_time = MPI_Wtime();
        burning_ship_mpi(max_iteraciones, x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer);
        dibujar_texto(0);
        double end_time = MPI_Wtime();
        compute_time = end_time - start_time;
        
        // copiar el pixel_buffer a texture_buffer
        std::memcpy(texture_buffer, pixel_buffer, WIDTH*delta*sizeof(uint32_t));

        //recibir las imagens aprciales de los otros RANKs
        MPI_Gather(
                nullptr, 
                0, 
                MPI_UNSIGNED, 
                texture_buffer, 
                WIDTH*delta, 
                MPI_UNSIGNED, 
                0, 
                MPI_COMM_WORLD
            );

        //recibir el tiempo con MPI_Reduce
        compute_time_ms = 0;
        MPI_Reduce(&compute_time, &compute_time_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        textOptions.setString(fmt::format("Compute Time: {:.3f} ms", compute_time_ms*1000.0) );

        //actualizar la textura
        texture.update((const uint8_t *)texture_buffer);

        //contar FPS
        frames++;

        if(clock.getElapsedTime().asSeconds() >= 1.0f) {
            fps = frames;
            frames = 0;
            clock.restart();
        }

        //actualizar el titulo de la ventana con el FPS
        auto msg = fmt::format("Fractal: Iterations: {},  FPS: {}, Mode: MPI", max_iteraciones, fps);
        text.setString(msg);

        window.clear();
        {
            window.draw(sprite);
            window.draw(text);
            window.draw(textOptions);
        } 
        window.display();
    }  
}

int main(int argc, char**argv) {

    MPI_Init(&argc, &argv);

    //-ranks
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    init_freetype();

    delta = std::ceil(HEIGHT*1.0/nprocs); // 1600/4 = 400
    
    row_start = rank * delta;
    row_end = row_start + delta;
    padding = delta * nprocs - HEIGHT; 

    if(row_end>HEIGHT) {
        row_end = HEIGHT;
    }

    pixel_buffer = new uint32_t[WIDTH*delta];
    std::memset(pixel_buffer, 0, WIDTH*delta*sizeof(uint32_t));

    fmt::println("RANK_{}: rows {} to {}", rank, row_start, row_end);

    if(rank==0) {
        setup_ui( );
    }
    else {
        //dibujar
        while(true) {
            std::vector<double> dummy = {0, 0, 0, 0, 0, 0};
            MPI_Bcast(dummy.data(), 6, MPI_DOUBLE, 0, MPI_COMM_WORLD);

            int max_iteraciones = (int)dummy[0];
            double x_min = dummy[1];
            double x_max = dummy[2];
            double y_min = dummy[3];
            double y_max = dummy[4];
            int running = (int)dummy[5];

            if(running == 0) {
                fmt::println("RANK_{}: received shutdown signal, exiting...", rank);
                break;
            }

            //--histograma
            double start_time = MPI_Wtime();
            
            burning_ship_mpi( max_iteraciones, x_min, y_min, x_max, y_max, WIDTH, HEIGHT, row_start, row_end, pixel_buffer);
            dibujar_texto(rank);
            
            double end_time = MPI_Wtime();
            compute_time = (end_time - start_time);

            //enviar la porcion de la imagen: 1600x225

            MPI_Gather(
                pixel_buffer, 
                WIDTH*delta, 
                MPI_UNSIGNED, 
                nullptr, 
                0, 
                MPI_UNSIGNED, 
                0, 
                MPI_COMM_WORLD
            );

            //enviar el tiempo con MPI_Reduce
            MPI_Reduce(&compute_time, &compute_time_ms, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        }

    }

    MPI_Finalize();

    return 0;
}