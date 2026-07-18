cmake_minimum_required(VERSION 3.16)

project(mpi_practice CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -- Configuración de MPI --
# Este proyecto usa Intel oneAPI MPI instalado en el equipo. Si en tu máquina
# MPI está en otra ruta, o registrado en el sistema, puedes reemplazar este
# bloque por `find_package(MPI REQUIRED)` y enlazar contra `MPI::MPI_CXX`.
set(MPI_ROOT "C:/oneAPI/mpi/2021.18")
set(MPI_INCLUDE "${MPI_ROOT}/include")
set(MPI_LIB "${MPI_ROOT}/lib")

include_directories(${MPI_INCLUDE})

add_executable(mpi_practice main.cpp)

target_link_libraries(mpi_practice PRIVATE
    ${MPI_LIB}/impi.lib
)
