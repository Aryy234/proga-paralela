#ifndef FRACTAL_NEWTON_H
#define FRACTAL_NEWTON_H

#include <cstdint>
#include <vector>

struct ConfiguracionFractal {
    int ancho;
    int alto;
    int max_iter;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
};

struct ResultadoFractal {
    double tiempo_ms;
    long long total_iters;
};

ResultadoFractal dibujar_fractal(const ConfiguracionFractal& config, std::vector<std::uint8_t>& pixeles);

#endif
