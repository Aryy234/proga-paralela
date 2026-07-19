cmake_minimum_required(VERSION 3.16)

project(FractalCuda)

set(CMAKE_CUDA_ARCHITECTURES native)
enable_language(CXX CUDA)

find_package(CUDAToolkit REQUIRED)
find_package(fmt CONFIG REQUIRED)

find_package(SFML COMPONENTS Graphics Window System CONFIG REQUIRED)

#--
add_executable(fractal_cuda
    main.cpp
    fractal_cpu.cpp
    kernel.cu
    palette.cpp
    )

target_link_libraries(fractal_cuda  PRIVATE
    fmt::fmt
    SFML::Graphics
    SFML::Window
    SFML::System
    CUDA::cudart
)
