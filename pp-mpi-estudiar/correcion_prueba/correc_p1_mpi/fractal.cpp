#include "fractal.h"
#include "palette.h"

#include <cmath>

static const double EPSILON = 1.0e-4;
static const double RADIO_ESCAPE = 2.0;

static uint32_t calcular_color_newton(double parte_real,
                                      double parte_imaginaria,
                                      int max_iteraciones,
                                      int& iteraciones)
{
    const double raiz_real[3] = {1.0, -0.5, -0.5};
    const double raiz_imaginaria[3] = {
        0.0,
        std::sqrt(3.0) / 2.0,
        -std::sqrt(3.0) / 2.0
    };

    double zr = parte_real;
    double zi = parte_imaginaria;
    iteraciones = 0;

    for (int iteracion = 0; iteracion < max_iteraciones; iteracion++) {
        double norma_cuadrada = zr * zr + zi * zi;
        if (norma_cuadrada > RADIO_ESCAPE * RADIO_ESCAPE ||
            norma_cuadrada < 1.0e-20) {
            return 0xFF000000;
        }

        // (z^3 - 1) / (3 z^2), escrito con componentes reales.
        double z2_real = zr * zr - zi * zi;
        double z2_imaginaria = 2.0 * zr * zi;
        double z3_real = z2_real * zr - z2_imaginaria * zi;
        double z3_imaginaria = z2_real * zi + z2_imaginaria * zr;
        double denominador_real = 3.0 * z2_real;
        double denominador_imaginario = 3.0 * z2_imaginaria;
        double denominador_norma = denominador_real * denominador_real +
                                   denominador_imaginario * denominador_imaginario;

        if (denominador_norma < 1.0e-20) {
            return 0xFF000000;
        }

        double numerador_real = z3_real - 1.0;
        double numerador_imaginario = z3_imaginaria;
        double cociente_real =
            (numerador_real * denominador_real +
             numerador_imaginario * denominador_imaginario) / denominador_norma;
        double cociente_imaginario =
            (numerador_imaginario * denominador_real -
             numerador_real * denominador_imaginario) / denominador_norma;

        zr -= cociente_real;
        zi -= cociente_imaginario;
        iteraciones = iteracion + 1;

        for (int raiz = 0; raiz < 3; raiz++) {
            double diferencia_real = zr - raiz_real[raiz];
            double diferencia_imaginaria = zi - raiz_imaginaria[raiz];
            double distancia_cuadrada = diferencia_real * diferencia_real +
                                        diferencia_imaginaria * diferencia_imaginaria;
            if (distancia_cuadrada < EPSILON * EPSILON) {
                int indice_color = (raiz * 5 + iteraciones) % PALETTE_SIZE;
                return color_ramp[indice_color];
            }
        }
    }

    return 0xFF000000;
}

void calcular_fractal_newton(double x_min, double y_min,
                             double x_max, double y_max,
                             int ancho, int alto,
                             int fila_inicial, int fila_final,
                             int max_iteraciones,
                             uint32_t* buffer_pixeles,
                             long long& iteraciones_realizadas)
{
    iteraciones_realizadas = 0;
    double paso_x = (x_max - x_min) / (ancho - 1.0);
    double paso_y = (y_max - y_min) / (alto - 1.0);

    for (int fila = fila_inicial; fila < fila_final; fila++) {
        double parte_imaginaria = y_max - fila * paso_y;
        int fila_local = fila - fila_inicial;

        for (int columna = 0; columna < ancho; columna++) {
            double parte_real = x_min + columna * paso_x;
            int iteraciones_pixel = 0;
            int posicion = fila_local * ancho + columna;
            buffer_pixeles[posicion] = calcular_color_newton(
                parte_real, parte_imaginaria, max_iteraciones, iteraciones_pixel);
            iteraciones_realizadas += iteraciones_pixel;
        }
    }
}
