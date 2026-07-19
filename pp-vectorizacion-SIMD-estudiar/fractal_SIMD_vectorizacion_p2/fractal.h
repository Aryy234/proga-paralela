#ifndef FRACTAL_H
#define FRACTAL_H

#include <cstdint>

void calcular_burning_ship_escalar(
    uint32_t* pixeles,
    int ancho,
    int alto,
    int max_iter,
    double xmin,
    double xmax,
    double ymin,
    double ymax
);

void calcular_burning_ship_simd(
    uint32_t* pixeles,
    int ancho,
    int alto,
    int max_iter,
    double xmin,
    double xmax,
    double ymin,
    double ymax
);

#endif // FRACTAL_H
