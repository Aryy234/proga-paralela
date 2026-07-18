#pragma once

#include <cstdint>

void calcular_fractal_newton(double x_min, double y_min,
                             double x_max, double y_max,
                             int ancho, int alto,
                             int fila_inicial, int fila_final,
                             int max_iteraciones,
                             uint32_t* buffer_pixeles,
                             long long& iteraciones_realizadas);
