#include "fractal_newton.h"
#include "palette.h"

#include <complex>

extern double epsilon;

const std::complex<double> w1(1,0);
const std::complex<double> w2(-0.5,0.8660254037844386);
const std::complex<double> w3(-0.5,-0.8660254037844386);

// multiplica dos números complejos (re1+im1*i) * (re2+im2*i) = (re_out+im_out*i)
inline void comp_mult(double re1, double im1, double re2, double im2, double& re_out, double& im_out) {
    re_out = re1 * re2 - im1 * im2;
    im_out = re1 * im2 + im1 * re2;
}

//divide dos números complejos (re1+im1*i) / (re2+im2*i) = (re_out+im_out*i)
inline void comp_div(double re1, double im1, double re2, double im2, double& re_out, double& im_out) {
    double denom = re2 * re2 + im2 * im2;
    re_out = (re1 * re2 + im1 * im2) / denom;
    im_out = (im1 * re2 - re1 * im2) / denom;
}

// Verificar raíces (comparando distancia al cuadrado para evitar sqrt)
inline double check_dist(double r1, double i1, double r2, double i2, int r_idx) {
    double dr = r2 - r1;
    double di = i2 - i1;
    return (dr * dr + di * di) < epsilon;
};

uint32_t acotado_1(int max_iteraciones, std::complex<double> value) {
    int iter = 1;

    std::complex<double> z = value;

    int root = -1;

     while(iter<max_iteraciones) {
        auto delta = (std::pow(z,3)-1.0) / (3.0*std::pow(z,2));
        z = z - delta;

        if(std::abs(z-w1)<epsilon) {
            root = 0;
            break;
        }
        else if(std::abs(z-w2)<epsilon) {
            root = 1;
             break;
        }
        else if(std::abs(z-w3)<epsilon) {
            root = 2;
            break;
        }

        iter++;
     }

    if(root!=-1) {
        return color_ramp[(root*5+iter ) % PALETTE_SIZE];
        // if(root==0) return 0xFFFF0000;
        // if(root==1) return 0xFF00FF00;
        // if(root==2) return 0xFF0000FF;
    }

    if(iter<max_iteraciones) {
        int index = iter % PALETTE_SIZE;
        return color_ramp[index];
    }

    return 0xFF000000; //negro
}

uint32_t acotado_2(int max_iteraciones, double re, double im) {
    // zn+1 = zn - (zn^3 - 1) / (3 * zn^2)

    int iter = 1;
    double z_re = re;
    double z_im = im;
    int root = -1;


    while (iter < max_iteraciones) {

        // Calcular z^2 y z^3
        double z2_re, z2_im;
        double z3_re, z3_im;
        comp_mult(z_re, z_im, z_re, z_im, z2_re, z2_im);
        comp_mult(z2_re, z2_im, z_re, z_im, z3_re, z3_im);

        // Numerador: z^3 - 1
        double num_re = z3_re - 1.0;
        double num_im = z3_im;

        // Denominador: 3 * z^2
        double den_re = 3.0 * z2_re;
        double den_im = 3.0 * z2_im;

        // Newton: z_next = z - (num / den)
        double delta_re, delta_im;
        comp_div(num_re, num_im, den_re, den_im, delta_re, delta_im);

        z_re -= delta_re;
        z_im -= delta_im;

        if (check_dist(z_re, z_im, w1.real(), w1.imag(), 0)) { root = 0; break; }
        if (check_dist(z_re, z_im, w2.real(), w2.imag(), 1)) { root = 1; break; }
        if (check_dist(z_re, z_im, w3.real(), w3.imag(), 2)) { root = 2; break; }

        iter++;
    }

    if(root!=-1) {
        return color_ramp[(root*5+iter ) % PALETTE_SIZE];
        // if(root==0) return 0xFFFF0000;
        // if(root==1) return 0xFF00FF00;
        // if(root==2) return 0xFF0000FF;
    }

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

            // auto color = acotado_1(max_iteraciones, std::complex<double>(x, y));
            auto color = acotado_2(max_iteraciones, x, y);

            pixel_buffer[(j-row_start)*width+i] = color;
        }
    }

    for(int i=0;i<width;i++) {
        pixel_buffer[i] = 0xFF000000; //negro
    }
}
