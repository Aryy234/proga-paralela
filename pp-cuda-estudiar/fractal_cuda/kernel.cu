#include "fractal_common.h"

#include <cuda_runtime.h>

__constant__ Pixel d_palette[kPaletteSize];

__device__ Pixel burning_ship_device(double cr, double ci, int max_iter) {
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (iter < max_iter && (zr * zr + zi * zi) <= 4.0) {
        double ar = fabs(zr);
        double ai = fabs(zi);
        double next_r = ar * ar - ai * ai + cr;
        double next_i = 2.0 * ar * ai + ci;
        zr = next_r;
        zi = next_i;
        ++iter;
    }

    if (iter >= max_iter) {
        return 0x000000FFu;
    }

    return d_palette[iter % kPaletteSize];
}

__global__ void render_kernel(FractalParams params, Pixel* pixels) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int total = params.width * params.height;
    if (index >= total) {
        return;
    }

    int row = index / params.width;
    int col = index % params.width;
    double ty = static_cast<double>(row) / static_cast<double>(params.height - 1);
    double tx = static_cast<double>(col) / static_cast<double>(params.width - 1);
    double cy = params.y_max - ty * (params.y_max - params.y_min);
    double cx = params.x_min + tx * (params.x_max - params.x_min);
    pixels[index] = burning_ship_device(cx, cy, params.max_iter);
}

extern "C" void cuda_upload_palette(const Pixel* host_palette) {
    cudaMemcpyToSymbol(d_palette, host_palette, kPaletteSize * sizeof(Pixel));
}

extern "C" void cuda_render_fractal(const FractalParams& params, Pixel* device_pixels) {
    const int total = params.width * params.height;
    const int threads = 256;
    const int blocks = (total + threads - 1) / threads;
    render_kernel<<<blocks, threads>>>(params, device_pixels);
}
