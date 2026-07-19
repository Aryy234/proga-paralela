cmake_minimum_required(VERSION 3.16)

project(fractal_cuda LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(SFML CONFIG REQUIRED COMPONENTS graphics window system)

add_executable(fractal_cuda
    main.cpp
    palette.cpp
    fractal_cpu.cpp
    kernel.cu
)

target_include_directories(fractal_cuda PRIVATE
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)

target_link_libraries(fractal_cuda PRIVATE
    fmt::fmt
    sfml-graphics
    sfml-window
    sfml-system
)

set_target_properties(fractal_cuda PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)
