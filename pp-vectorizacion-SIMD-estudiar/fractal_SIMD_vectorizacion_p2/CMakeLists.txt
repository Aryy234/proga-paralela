cmake_minimum_required(VERSION 3.16)
project(fractal_simd_vectorizacion_p1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(fractal_simd_vectorizacion_p1
    main.cpp
    palette.cpp
    fractal_scalar.cpp
    fractal_simd.cpp
)

target_compile_options(fractal_simd_vectorizacion_p1 PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-mavx2>
)

target_link_libraries(fractal_simd_vectorizacion_p1 PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    fmt::fmt
)

target_include_directories(fractal_simd_vectorizacion_p1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
