cmake_minimum_required(VERSION 3.16)

project(fractal_newton_openmp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)

set(VCPKG_ROOT "C:/tools/vcpkg-2026.03.18" CACHE PATH "Ruta de vcpkg")
set(VCPKG_TRIPLET "x64-mingw-dynamic" CACHE STRING "Triplet de vcpkg")

if (NOT DEFINED SFML_DIR)
    set(SFML_DIR "${VCPKG_ROOT}/installed/${VCPKG_TRIPLET}/share/sfml")
endif()

if (NOT DEFINED fmt_DIR)
    set(fmt_DIR "${VCPKG_ROOT}/installed/${VCPKG_TRIPLET}/share/fmt")
endif()

find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System)
find_package(fmt CONFIG REQUIRED)

add_executable(fractal_newton
    main.cpp
    interfaz.cpp
    fractal_newton.cpp
    palette.cpp
)

target_include_directories(fractal_newton PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fractal_newton PRIVATE
    OpenMP::OpenMP_CXX
    SFML::Graphics
    SFML::Window
    SFML::System
    fmt::fmt
)

if (MSVC)
    target_compile_options(fractal_newton PRIVATE /W4)
else()
    target_compile_options(fractal_newton PRIVATE -Wall -Wextra -Wpedantic)
endif()
