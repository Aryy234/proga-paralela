cmake_minimum_required(VERSION 3.16)

project(FractalJulia)

find_package(fmt CONFIG REQUIRED)
find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
find_package(Freetype REQUIRED)

#--config MPI
set(MPI_ROOT    "c:/oneAPI/mpi/latest")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB     "${MPI_ROOT}/lib")

include_directories(
    ${MPI_INCLUDE}
)

add_executable(fractal-mpi 
    main.cpp
    fractal_newton.cpp
    palette.cpp
    draw_text.cpp
)


target_link_libraries(fractal-mpi  PRIVATE 
    fmt::fmt
    SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System
    Freetype::Freetype
    ${MPI_LIB}/impi.lib
)


