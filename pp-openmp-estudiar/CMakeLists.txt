cmake_minimum_required(VERSION 3.16)

project(fractal_newton_openmp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenMP REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
find_package(fmt REQUIRED)

add_executable(fractal_newton
    main.cpp
    palette.cpp
)

target_include_directories(fractal_newton PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fractal_newton PRIVATE
    OpenMP::OpenMP_CXX
    sfml-graphics
    sfml-window
    sfml-system
    fmt::fmt
)

if (MSVC)
    target_compile_options(fractal_newton PRIVATE /W4)
else()
    target_compile_options(fractal_newton PRIVATE -Wall -Wextra -Wpedantic)
endif()
