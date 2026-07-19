#include "fractal.h"
#include "palette.h"
#include "params.h"

#include <immintrin.h>
#include <cstddef>
#include <cmath>

static uint32_t color_por_iteracion(int iteracion, int max_iter)
{
    if (iteracion >= max_iter) {
        return 0xFF000000u;
    }

    const std::size_t indice = static_cast<std::size_t>(iteracion) % color_ramp.size();
    return color_ramp[indice];
}

void calcular_burning_ship_simd(
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
    const __m256d limite_escape = _mm256_set1_pd(limite_escape_cuadrado);
    const __m256d dos = _mm256_set1_pd(2.0);
    const __m256d mascara_signo = _mm256_set1_pd(-0.0);
    const __m256i uno_i = _mm256_set1_epi32(1);

    alignas(32) double c_reales[4];
    alignas(32) int iteraciones[4];

    for (int y = 0; y < alto; y++) {
        const double c_imag = ymin_local + static_cast<double>(y) * paso_y;
        const __m256d c_imag_vec = _mm256_set1_pd(c_imag);

        int x = 0;
        for (; x <= ancho - 4; x += 4) {
            c_reales[0] = xmin_local + static_cast<double>(x + 0) * paso_x;
            c_reales[1] = xmin_local + static_cast<double>(x + 1) * paso_x;
            c_reales[2] = xmin_local + static_cast<double>(x + 2) * paso_x;
            c_reales[3] = xmin_local + static_cast<double>(x + 3) * paso_x;

            __m256d c_real_vec = _mm256_load_pd(c_reales);
            __m256d z_real_vec = _mm256_setzero_pd();
            __m256d z_imag_vec = _mm256_setzero_pd();
            __m256i iter_vec = _mm256_setzero_si256();
            __m256d activo_vec = _mm256_castsi256_pd(_mm256_set1_epi64x(-1));

            for (int iter = 0; iter < max_iter; iter++) {
                const __m256d abs_real_vec = _mm256_andnot_pd(mascara_signo, z_real_vec);
                const __m256d abs_imag_vec = _mm256_andnot_pd(mascara_signo, z_imag_vec);
                const __m256d abs_real_cuadrado = _mm256_mul_pd(abs_real_vec, abs_real_vec);
                const __m256d abs_imag_cuadrado = _mm256_mul_pd(abs_imag_vec, abs_imag_vec);
                const __m256d nuevo_real = _mm256_add_pd(_mm256_sub_pd(abs_real_cuadrado, abs_imag_cuadrado), c_real_vec);
                const __m256d nuevo_imag = _mm256_add_pd(_mm256_mul_pd(_mm256_mul_pd(abs_real_vec, abs_imag_vec), dos), c_imag_vec);
                const __m256d radio_cuadrado = _mm256_add_pd(_mm256_mul_pd(nuevo_real, nuevo_real), _mm256_mul_pd(nuevo_imag, nuevo_imag));
                const __m256d sigue_activo = _mm256_cmp_pd(radio_cuadrado, limite_escape, _CMP_LE_OQ);
                const __m256d activo_despues = _mm256_and_pd(activo_vec, sigue_activo);
                const __m256d incrementar_vec = _mm256_and_pd(activo_vec, sigue_activo);

                iter_vec = _mm256_add_epi32(iter_vec, _mm256_and_si256(_mm256_castpd_si256(incrementar_vec), uno_i));
                z_real_vec = _mm256_blendv_pd(z_real_vec, nuevo_real, activo_vec);
                z_imag_vec = _mm256_blendv_pd(z_imag_vec, nuevo_imag, activo_vec);
                activo_vec = activo_despues;

                if (_mm256_movemask_pd(activo_vec) == 0) {
                    break;
                }
            }

            _mm256_store_si256(reinterpret_cast<__m256i*>(iteraciones), iter_vec);

            for (int lane = 0; lane < 4; lane++) {
                pixeles[y * ancho + x + lane] = color_por_iteracion(iteraciones[lane], max_iter);
            }
        }

        for (; x < ancho; x++) {
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
