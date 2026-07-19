#include "fractal.h"
#include "palette.h"
#include "params.h"

#include <cmath>

static uint32_t color_por_iteracion(int iteracion, int max_iter)
{
    if (iteracion >= max_iter) {
        return 0xFF000000u;
    }

    const std::size_t indice = static_cast<std::size_t>(iteracion) % color_ramp.size();
    return color_ramp[indice];
}

void calcular_burning_ship_escalar(
    uint32_t* pixeles,
    int ancho,
    int alto,
    int max_iter,
    double xmin_local,
    double xmax_local,
    double ymin_local,
    double ymax_local
)
{
    const double paso_x = (xmax_local - xmin_local) / static_cast<double>(ancho - 1);
    const double paso_y = (ymax_local - ymin_local) / static_cast<double>(alto - 1);

    for (int y = 0; y < alto; y++) {
        const double c_imag = ymin_local + static_cast<double>(y) * paso_y;

        for (int x = 0; x < ancho; x++) {
            const double c_real = xmin_local + static_cast<double>(x) * paso_x;

            double z_real = 0.0;
            double z_imag = 0.0;
            int iteracion = 0;

            while (iteracion < max_iter) {
                const double abs_real = std::fabs(z_real);
                const double abs_imag = std::fabs(z_imag);
                const double nuevo_real = abs_real * abs_real - abs_imag * abs_imag + c_real;
                const double nuevo_imag = 2.0 * abs_real * abs_imag + c_imag;

                z_real = nuevo_real;
                z_imag = nuevo_imag;

                const double radio_cuadrado = z_real * z_real + z_imag * z_imag;
                if (radio_cuadrado > limite_escape_cuadrado) {
                    break;
                }

                iteracion++;
            }

            pixeles[y * ancho + x] = color_por_iteracion(iteracion, max_iter);
        }
    }
}
