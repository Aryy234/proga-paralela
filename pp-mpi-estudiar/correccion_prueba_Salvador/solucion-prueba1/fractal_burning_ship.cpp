#include "fractal_burning_ship.h"
#include "palette.h"

#include <complex>

extern int* local_hist;

uint32_t acotado_2(int max_iteraciones, double x, double y) {
    /**
     * dados: c, z0
     * Zn+1 = Zn^2 + c
     */

     int iter = 1;

    double zr = 0;
    double zi = 0;

     while(iter<max_iteraciones && (zr*zr+zi*zi)<4.0) {
        //Zn+1 = Zn^2 + c
        double zr_abs = std::abs(zr);
        double zi_abs = std::abs(zi);
        
        double dr = zr_abs*zr_abs-zi_abs*zi_abs + x;
        double di = 2.0*zr_abs*zi_abs + y;
        
        zr = dr;
        zi = di;

        iter++;
     }

     int index = iter*16/max_iteraciones;
     local_hist[iter*16/max_iteraciones]++;

     if(iter<max_iteraciones) {
        int index = iter % PALETTE_SIZE;
        return color_ramp[index];
     }

     return 0xFF000000; //negro
}

void burning_ship_mpi(int max_iteraciones, double x_min, double y_min, double x_max, double y_max, 
    uint32_t width, uint32_t height, 
    uint32_t row_start, uint32_t row_end,
    uint32_t* pixel_buffer) {
        
    double dx = (x_max - x_min) / width;
    double dy = (y_max - y_min) / height;

    for(int j=row_start;j<row_end;j++) {
        for(int i=0;i<width;i++) {
            // z = x+yi = (x,y)
            double x = x_min+i*dx;
            double y = y_max-j*dy;

            auto color = acotado_2(max_iteraciones, x, y);

            pixel_buffer[(j-row_start)*width+i] = color;
        }
    }

    for(int i=0;i<width;i++) {
        pixel_buffer[i] = 0xFF000000; //negro
    }
}
