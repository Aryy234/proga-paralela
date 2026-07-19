#include "fractal_common.h"
#include "fractal_cpu.h"

#include <SFML/Graphics.hpp>
#include <cuda_runtime.h>
#include <fmt/core.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>

extern "C" void cuda_upload_palette(const Pixel* host_palette);
extern "C" void cuda_render_fractal(const FractalParams& params, Pixel* device_pixels);

#define CHECK_CUDA(expr) do { \
    cudaError_t err = (expr); \
    if (err != cudaSuccess) { \
        fmt::print(stderr, "CUDA error {}: {} at line {}\n", static_cast<int>(err), cudaGetErrorString(err), __LINE__); \
        std::exit(EXIT_FAILURE); \
    } \
} while (0)

enum class Mode { Cpu, Cuda };

static bool load_font(sf::Font& font) {
    const std::vector<std::string> candidates = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/Arial.ttf",
        "arial.ttf"
    };
    for (const auto& path : candidates) {
        if (font.openFromFile(path)) {
            return true;
        }
    }
    return false;
}

static double measure_cpu(const FractalParams& params, std::vector<Pixel>& pixels) {
    auto start = std::chrono::high_resolution_clock::now();
    render_fractal_cpu(params, pixels.data());
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static double measure_cuda(const FractalParams& params, std::vector<Pixel>& pixels, Pixel* device_pixels) {
    auto start = std::chrono::high_resolution_clock::now();
    cuda_render_fractal(params, device_pixels);
    CHECK_CUDA(cudaGetLastError());
    CHECK_CUDA(cudaDeviceSynchronize());
    CHECK_CUDA(cudaMemcpy(pixels.data(), device_pixels, pixels.size() * sizeof(Pixel), cudaMemcpyDeviceToHost));
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    FractalParams params;
    std::vector<Pixel> host_pixels(static_cast<std::size_t>(params.width) * params.height);
    Pixel* device_pixels = nullptr;
    CHECK_CUDA(cudaMalloc(reinterpret_cast<void**>(&device_pixels), host_pixels.size() * sizeof(Pixel)));
    cuda_upload_palette(palette().data());

    sf::RenderWindow window(sf::VideoMode({static_cast<unsigned int>(params.width), static_cast<unsigned int>(params.height)}), "Burning Ship CUDA");
    window.setFramerateLimit(60);

    sf::Texture texture(sf::Vector2u{static_cast<unsigned int>(params.width), static_cast<unsigned int>(params.height)});
    sf::Sprite sprite(texture);

    sf::Font font;
    const bool has_font = load_font(font);
    sf::Text overlay(font);
    if (has_font) {
        overlay.setCharacterSize(20);
        overlay.setFillColor(sf::Color::White);
        overlay.setPosition(sf::Vector2f{14.f, 10.f});
    }

    Mode mode = Mode::Cuda;
    double last_ms = measure_cuda(params, host_pixels, device_pixels);
    texture.update(reinterpret_cast<const std::uint8_t*>(host_pixels.data()));

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                } else if (keyPressed->code == sf::Keyboard::Key::Space) {
                    mode = (mode == Mode::Cuda) ? Mode::Cpu : Mode::Cuda;
                } else if (keyPressed->code == sf::Keyboard::Key::Up) {
                    params.max_iter += 25;
                } else if (keyPressed->code == sf::Keyboard::Key::Down) {
                    params.max_iter = std::max(25, params.max_iter - 25);
                }
            }
        }

        if (mode == Mode::Cpu) {
            last_ms = measure_cpu(params, host_pixels);
        } else {
            last_ms = measure_cuda(params, host_pixels, device_pixels);
        }

        texture.update(reinterpret_cast<const std::uint8_t*>(host_pixels.data()));
        window.clear();
        window.draw(sprite);

        if (has_font) {
            const float fps = last_ms > 0.0 ? static_cast<float>(1000.0 / last_ms) : 0.0f;
            overlay.setString(fmt::format(
                "modo: {}\nmax_iter: {}\ntiempo_ms: {:.3f}\nFPS: {:.2f}\n[Space] alternar CPU/CUDA\n[Up/Down] ajustar iter\n[Esc] salir",
                mode == Mode::Cpu ? "CPU" : "CUDA",
                params.max_iter,
                last_ms,
                fps
            ));
            window.draw(overlay);
        }

        window.display();
    }

    CHECK_CUDA(cudaFree(device_pixels));
    return 0;
}
