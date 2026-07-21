# Guía de compilación: SIMD, OpenMP, MPI y CUDA

Esta guía usa las rutas instaladas en este equipo:

- MinGW: `C:\tools\mingw64`
- vcpkg: `C:\tools\vcpkg-2026.03.18`
- Intel MPI: `C:\oneAPI\mpi\2021.18`
- CUDA: `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3`
- Visual Studio 2026: `C:\Program Files\Microsoft Visual Studio\18\Community`

> Los pasos de interfaz se refieren a **VS Code con la extensión CMake Tools**, porque allí aparece `Select a Kit`.

## 1. Preparación inicial

Abrir **Command Prompt (`cmd`)**, no PowerShell.

```bat
set PATH=C:\tools\mingw64\bin;%PATH%

gcc --version
g++ --version
cmake --version
ninja --version
```

### Preparar vcpkg

El `bootstrap` se ejecuta solamente la primera vez:

```bat
cd /d C:\tools\vcpkg-2026.03.18
bootstrap-vcpkg.bat
```

Dependencias para SIMD, OpenMP y MPI con MinGW:

```bat
vcpkg install fmt:x64-mingw-dynamic
vcpkg install sfml:x64-mingw-dynamic
vcpkg install freetype:x64-mingw-dynamic
```

Dependencias para CUDA con MSVC:

```bat
vcpkg install fmt:x64-windows
vcpkg install sfml:x64-windows
```

## 2. Configuración de VS Code para SIMD, OpenMP y MPI

Usar este `.vscode/settings.json`:

```json
{
  "cmake.generator": "Ninja Multi-Config",
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.configureSettings": {
    "CMAKE_C_COMPILER": "C:/tools/mingw64/bin/gcc.exe",
    "CMAKE_CXX_COMPILER": "C:/tools/mingw64/bin/g++.exe",
    "CMAKE_MAKE_PROGRAM": "C:/tools/mingw64/bin/ninja.exe",
    "CMAKE_TOOLCHAIN_FILE": "C:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake",
    "VCPKG_TARGET_TRIPLET": "x64-mingw-dynamic"
  }
}
```

En VS Code:

1. Abrir la carpeta que contiene el `CMakeLists.txt`.
2. Ejecutar `CMake: Select a Kit`.
3. Seleccionar `Unspecified`, porque los compiladores ya están definidos en `settings.json`.
4. Ejecutar `CMake: Delete Cache and Reconfigure`.
5. Seleccionar `Debug` o `Release`.
6. Pulsar `Build`.

## 3. Vectorización SIMD

Proyecto:

```text
C:\tools\proga-paralela\pp-vectorizacion-SIMD-estudiar\fractal_SIMD_vectorizacion_p2
```

### Desde `cmd`

```bat
set PATH=C:\tools\mingw64\bin;%PATH%
cd /d C:\tools\proga-paralela\pp-vectorizacion-SIMD-estudiar\fractal_SIMD_vectorizacion_p2

cmake -S . -B build -G "Ninja Multi-Config" ^
  -DCMAKE_C_COMPILER=C:/tools/mingw64/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=C:/tools/mingw64/bin/g++.exe ^
  -DCMAKE_MAKE_PROGRAM=C:/tools/mingw64/bin/ninja.exe ^
  -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

cmake --build build --config Debug
build\Debug\fractal_simd_vectorizacion_p1.exe
```

El `CMakeLists.txt` debe conservar la opción AVX2:

```cmake
target_compile_options(fractal_simd_vectorizacion_p1 PRIVATE -mavx2)
```

Archivos del fractal:

- `fractal_scalar.cpp`: versión secuencial.
- `fractal_simd.cpp`: versión vectorizada con AVX2.
- `palette.cpp`: colores del fractal.
- `arial_ttf.h`: fuente Arial embebida; no se agrega como `.cpp`.
- No utiliza `draw_text.cpp`.

## 4. OpenMP

Proyecto:

```text
C:\tools\proga-paralela\pp-openmp-estudiar\fractal_openMP_p2
```

### Desde `cmd`

```bat
set PATH=C:\tools\mingw64\bin;%PATH%
cd /d C:\tools\proga-paralela\pp-openmp-estudiar\fractal_openMP_p2

cmake -S . -B build -G "Ninja Multi-Config" ^
  -DCMAKE_C_COMPILER=C:/tools/mingw64/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=C:/tools/mingw64/bin/g++.exe ^
  -DCMAKE_MAKE_PROGRAM=C:/tools/mingw64/bin/ninja.exe ^
  -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

cmake --build build --config Debug
build\Debug\fractal_newton.exe
```

Configuración esencial de `CMakeLists.txt`:

```cmake
find_package(OpenMP REQUIRED)

target_link_libraries(fractal_newton PRIVATE
    OpenMP::OpenMP_CXX
    SFML::Graphics
    SFML::Window
    SFML::System
    fmt::fmt
)
```

Archivos del fractal:

- `fractal_newton.cpp`: cálculo paralelo con OpenMP.
- `interfaz.cpp`: ventana y texto mediante SFML.
- `palette.cpp`: colores.
- `arial_ttf.h`: fuente embebida.
- No utiliza `draw_text.cpp`.

Opcionalmente se puede controlar el número de hilos antes de ejecutar:

```bat
set OMP_NUM_THREADS=4
build\Debug\fractal_newton.exe
```

## 5. MPI

### Inicializar Intel MPI

En cada `cmd` nuevo:

```bat
call C:\oneAPI\mpi\2021.18\env\vars.bat
mpiexec --version
```

No usar `C:\oneAPI\mpi\latest`, porque esa ruta no existe en este equipo.

### Compilar desde `cmd`

Ejemplo con el proyecto `mpi`:

```bat
set PATH=C:\tools\mingw64\bin;%PATH%
call C:\oneAPI\mpi\2021.18\env\vars.bat
cd /d C:\tools\proga-paralela\pp-mpi-estudiar\mpi

cmake -S . -B build -G "Ninja Multi-Config" ^
  -DCMAKE_CXX_COMPILER=C:/tools/mingw64/bin/g++.exe ^
  -DCMAKE_MAKE_PROGRAM=C:/tools/mingw64/bin/ninja.exe

cmake --build build --config Debug
mpiexec -n 4 build\Debug\mpi_pi.exe 100000
```

Para el fractal MPI se ejecuta de la misma forma:

```bat
mpiexec -n 4 build\Debug\fractal-mpi.exe
```

Configuración esencial de `CMakeLists.txt`:

```cmake
set(MPI_ROOT "C:/oneAPI/mpi/2021.18")

target_include_directories(fractal-mpi PRIVATE
    "${MPI_ROOT}/include"
)

target_link_libraries(fractal-mpi PRIVATE
    "${MPI_ROOT}/lib/impi.lib"
)
```

Para un fractal MPI con interfaz deben incluirse:

```cmake
add_executable(fractal-mpi
    main.cpp
    fractal_burning_ship.cpp  # o fractal_newton.cpp
    palette.cpp
    draw_text.cpp
)

find_package(Freetype REQUIRED)
target_link_libraries(fractal-mpi PRIVATE Freetype::Freetype)
```

Archivos del fractal:

- `fractal_burning_ship.cpp` o `fractal_newton.cpp`: cálculo del fractal.
- `palette.cpp`: colores.
- `draw_text.cpp`: dibujo manual de texto con FreeType.
- `arial_ttf.h`: datos de Arial utilizados por `draw_text.cpp`.

Correcciones necesarias en las configuraciones existentes:

- Cambiar las rutas que comienzan con `D:/tools` por `C:/tools`.
- Cambiar `C:/oneAPI/mpi/latest` por `C:/oneAPI/mpi/2021.18`.
- En `solucion-prueba2`, usar `fractal_newton.cpp`; el archivo `fractal.cpp` no existe.

## 6. CUDA

CUDA en Windows utiliza **MSVC**, no MinGW. Por eso usa el triplet `x64-windows`.

### Configuración de VS Code

Usar este `.vscode/settings.json`:

```json
{
  "cmake.generator": "Ninja",
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.configureOnOpen": true,
  "cmake.configureSettings": {
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_MAKE_PROGRAM": "C:/tools/mingw64/bin/ninja.exe",
    "CMAKE_CUDA_COMPILER": "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.3/bin/nvcc.exe",
    "CMAKE_TOOLCHAIN_FILE": "C:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake",
    "VCPKG_TARGET_TRIPLET": "x64-windows",
    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
  },
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

Abrir VS Code desde un `cmd` preparado para MSVC:

```bat
set PATH=C:\tools\mingw64\bin;%PATH%
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d C:\tools\proga-paralela\pp-cuda-estudiar\fractal_cuda
code .
```

En VS Code:

1. Ejecutar `CMake: Select a Kit`.
2. Seleccionar el kit MSVC/Visual Studio `amd64`. Si no aparece, usar `Unspecified` porque VS Code ya fue abierto después de ejecutar `vcvars64.bat`.
3. Ejecutar `CMake: Delete Cache and Reconfigure`.
4. Pulsar `Build`.
5. Ejecutar `build\fractal_cuda.exe`.

### Desde `cmd`

```bat
set PATH=C:\tools\mingw64\bin;%PATH%
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d C:\tools\proga-paralela\pp-cuda-estudiar\fractal_cuda

cmake -S . -B build -G Ninja ^
  -DCMAKE_MAKE_PROGRAM=C:/tools/mingw64/bin/ninja.exe ^
  -DCMAKE_CUDA_COMPILER="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.3/bin/nvcc.exe" ^
  -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
build\fractal_cuda.exe
```

Configuración recomendada de `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)
project(FractalCuda LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES native)

find_package(CUDAToolkit REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System)

add_executable(fractal_cuda
    main.cpp
    fractal_cpu.cpp
    kernel.cu
    palette.cpp
)

target_link_libraries(fractal_cuda PRIVATE
    fmt::fmt
    SFML::Graphics
    SFML::Window
    SFML::System
    CUDA::cudart
)
```

Archivos del fractal:

- `fractal_cpu.cpp`: versión para CPU.
- `kernel.cu`: kernel que ejecuta el cálculo en la GPU.
- `palette.cpp`: colores.
- No utiliza `draw_text.cpp`.
- La fuente se busca en `C:\Windows\Fonts\arial.ttf` durante la ejecución; no usa `arial_ttf.h`.

## 7. Resumen rápido

| Tema | Compilador | vcpkg triplet | Preparación adicional |
|---|---|---|---|
| SIMD | MinGW `g++` | `x64-mingw-dynamic` | Compilar con `-mavx2` |
| OpenMP | MinGW `g++` | `x64-mingw-dynamic` | Enlazar `OpenMP::OpenMP_CXX` |
| MPI | MinGW `g++` + Intel MPI | `x64-mingw-dynamic` | Ejecutar `vars.bat` y usar `mpiexec` |
| CUDA | MSVC + `nvcc` | `x64-windows` | Ejecutar `vcvars64.bat` |

Si se cambia de compilador, kit o triplet, ejecutar siempre `CMake: Delete Cache and Reconfigure` antes de volver a compilar.

## 8. Estructura general de carpetas y archivos

Una estructura común para los ejercicios es:

```text
nombre_proyecto/
|-- .vscode/
|   `-- settings.json
|-- CMakeLists.txt
|-- main.cpp
|-- fractal.h
|-- fractal.cpp
|-- palette.h
|-- palette.cpp
|-- interfaz.h
|-- interfaz.cpp
|-- draw_text.h
|-- draw_text.cpp
|-- arial_ttf.h
`-- build/                 # Generada por CMake; no se escribe manualmente
```

No todos los proyectos necesitan todos esos archivos. Se agregan solamente cuando cumplen una función real.

### ¿Cuándo usar `.h` y cuándo `.cpp`?

#### Archivo `.h`

El `.h` contiene las **declaraciones** que deben conocer otros archivos:

```cpp
// fractal.h
#pragma once

void calcular_fractal(int ancho, int alto);
```

Se usa cuando una función, clase, estructura o constante será compartida entre varios `.cpp`.

#### Archivo `.cpp`

El `.cpp` contiene la **implementación**:

```cpp
// fractal.cpp
#include "fractal.h"

void calcular_fractal(int ancho, int alto)
{
    // Cálculo del fractal
}
```

Los `.cpp` deben aparecer en `add_executable`:

```cmake
add_executable(fractal
    main.cpp
    fractal.cpp
    palette.cpp
)
```

Los `.h` normalmente no necesitan aparecer en `add_executable`, porque se incorporan mediante `#include`.

#### Archivo `.cu`

Se usa en CUDA para código que compila `nvcc`, especialmente kernels de GPU:

```cpp
// kernel.cu
__global__ void calcular_pixeles(/* parámetros */)
{
    // Código ejecutado en la GPU
}
```

El `.cu` sí debe aparecer en `add_executable`:

```cmake
add_executable(fractal_cuda
    main.cpp
    fractal_cpu.cpp
    kernel.cu
    palette.cpp
)
```

### Función de cada archivo

| Archivo | Función |
|---|---|
| `.vscode/settings.json` | Indica a VS Code qué compilador, generador y triplet debe usar. |
| `CMakeLists.txt` | Define el ejecutable, los `.cpp`/`.cu`, opciones y librerías. |
| `main.cpp` | Punto de entrada; crea la interfaz y llama al cálculo paralelo. |
| `fractal.h` | Declara parámetros y funciones del fractal. |
| `fractal.cpp` | Implementa el cálculo del fractal en CPU. |
| `palette.h` | Declara tipos y funciones relacionadas con los colores. |
| `palette.cpp` | Implementa la paleta de colores. |
| `interfaz.h` | Declara las funciones de la ventana o interfaz. |
| `interfaz.cpp` | Implementa la interfaz con SFML. |
| `draw_text.h` | Declara la función para dibujar texto manualmente. |
| `draw_text.cpp` | Implementa el texto con FreeType. |
| `arial_ttf.h` | Contiene la fuente Arial convertida a datos binarios. |
| `kernel.cu` | Implementa el kernel que se ejecuta en la GPU. |
| `build/` | Contiene los archivos generados y el ejecutable compilado. |

## 9. Estructura mínima por tema

### SIMD

Se separa la versión normal de la versión vectorizada:

```text
fractal_simd/
|-- .vscode/settings.json
|-- CMakeLists.txt
|-- main.cpp
|-- fractal.h
|-- fractal_scalar.cpp
|-- fractal_simd.cpp
|-- palette.h
|-- palette.cpp
|-- params.h
`-- arial_ttf.h
```

- `fractal.h` declara las dos versiones.
- `fractal_scalar.cpp` implementa la versión secuencial.
- `fractal_simd.cpp` implementa AVX2 con `<immintrin.h>`.
- `params.h` guarda constantes o dimensiones compartidas.

### OpenMP

```text
fractal_openmp/
|-- .vscode/settings.json
|-- CMakeLists.txt
|-- main.cpp
|-- fractal_newton.h
|-- fractal_newton.cpp
|-- interfaz.h
|-- interfaz.cpp
|-- palette.h
|-- palette.cpp
`-- arial_ttf.h
```

- Las directivas `#pragma omp` se colocan en el `.cpp` que realiza el cálculo.
- `fractal_newton.h` no necesita incluir `<omp.h>` si no expone tipos de OpenMP.
- `interfaz.cpp` puede consultar la cantidad de hilos con `omp_get_max_threads()`.

### MPI básico

Si el ejercicio es pequeño puede tener únicamente:

```text
mpi_ejercicio/
|-- .vscode/settings.json
|-- CMakeLists.txt
`-- main.cpp
```

Esto es suficiente cuando toda la lógica MPI cabe claramente en `main.cpp`.

### MPI con fractal

```text
fractal_mpi/
|-- .vscode/settings.json
|-- CMakeLists.txt
|-- main.cpp
|-- fractal.h
|-- fractal.cpp
|-- palette.h
|-- palette.cpp
|-- draw_text.h
|-- draw_text.cpp
`-- arial_ttf.h
```

- `main.cpp` contiene `MPI_Init`, distribución de trabajo, comunicación y `MPI_Finalize`.
- `fractal.cpp` calcula únicamente la parte de imagen asignada al proceso.
- `draw_text.cpp` y `arial_ttf.h` se agregan solamente si el fractal muestra texto con FreeType.
- El nombre real puede ser `fractal_burning_ship.cpp` o `fractal_newton.cpp`; debe coincidir exactamente con el nombre escrito en `CMakeLists.txt`.

### CUDA

```text
fractal_cuda/
|-- .vscode/settings.json
|-- CMakeLists.txt
|-- main.cpp
|-- fractal_common.h
|-- fractal_cpu.h
|-- fractal_cpu.cpp
|-- kernel.cu
`-- palette.cpp
```

- `fractal_common.h` contiene estructuras que necesitan tanto CPU como GPU.
- `fractal_cpu.h/.cpp` contienen la versión ejecutada por el procesador.
- `kernel.cu` contiene el kernel y las funciones CUDA.
- `main.cpp` reserva memoria, llama al kernel y muestra la imagen.
- Si la fuente se carga desde `C:\Windows\Fonts\arial.ttf`, no se necesita `arial_ttf.h`.

## 10. Regla práctica para separar archivos

Usar un solo `main.cpp` cuando el ejercicio es corto. Separar en `.h` y `.cpp` cuando:

- el cálculo del fractal crece;
- una función se utiliza desde varios archivos;
- se quieren comparar versiones CPU, SIMD, OpenMP o CUDA;
- la interfaz, la paleta y el cálculo tienen responsabilidades distintas.

La relación habitual es:

```text
main.cpp
   |-- incluye fractal.h     -> implementación en fractal.cpp
   |-- incluye palette.h     -> implementación en palette.cpp
   `-- incluye interfaz.h    -> implementación en interfaz.cpp
```

Solo los archivos con implementación (`.cpp` y `.cu`) se agregan obligatoriamente al ejecutable de CMake.

## 11. Convenciones para escribir los ejemplos

Aplica estas reglas al crear o modificar ejercicios de SIMD, OpenMP, MPI y CUDA.

### Idioma y estilo

- Escribe en español los comentarios, mensajes de consola y explicaciones.
- Usa un estilo imperativo y directo.
- Escribe código claro y didáctico, pensado para estudiantes y prácticas.
- Evita trucos del lenguaje, abstracciones innecesarias y soluciones difíciles de seguir.

### Nombres

- Usa nombres descriptivos y completos para variables y funciones.
- Escribe los nombres generales en español: `filas_locales`, `cantidad_local`, `calcular_desplazamientos` o `mostrar_resultado`.
- Conserva nombres como `sendcounts`, `displs`, `recvcounts` y `scatter_recvbuf` cuando ayuden a relacionar el código con los parámetros oficiales de MPI.
- Evita acrónimos propios o abreviaturas confusas.

Ejemplo:

```cpp
int cantidad_procesos;
int identificador_proceso;
int filas_locales;
int cantidad_local;
int* sendcounts = nullptr;
int* displs = nullptr;
int* scatter_recvbuf = nullptr;
```

### Tipos y contenedores

- Prefiere arreglos C explícitos en los ejemplos sencillos.
- Reserva con `new[]` y libera con `delete[]`.
- Inicializa los punteros con `nullptr`.
- Usa STL solamente cuando aporte claridad real al ejemplo.

```cpp
int* datos = nullptr;

if (cantidad_elementos > 0) {
    datos = new int[cantidad_elementos];
}

// Usa el arreglo.

delete[] datos;
```

### Bucles

- Usa bucles `for` explícitos con índices.
- Prefiere `i++` para los incrementos.
- Evita lambdas, `std::transform` y programación funcional en ejemplos didácticos.

```cpp
for (int i = 0; i < cantidad_elementos; i++) {
    datos[i] = i * 2;
}
```

### Comentarios

- Escribe comentarios breves y en español.
- Explica la intención del bloque.
- No describas línea por línea operaciones evidentes.

```cpp
// Reparte también las filas sobrantes entre los primeros procesos.
for (int proceso = 0; proceso < cantidad_procesos; proceso++) {
    filas_por_proceso[proceso] = filas_base;

    if (proceso < filas_sobrantes) {
        filas_por_proceso[proceso]++;
    }
}
```

### Seguridad

- Comprueba siempre los casos con cero elementos.
- No accedas a `buffer[0]` si el tamaño es cero.
- Pasa `nullptr` a MPI cuando el proceso no dispone de un búfer válido y la operación lo permite.
- Usa `nullptr` en lugar de `NULL`.
- Libera todos los arreglos reservados con `new[]`.

```cpp
int* buffer_local = nullptr;

if (cantidad_local > 0) {
    buffer_local = new int[cantidad_local];
}

MPI_Scatterv(
    datos_globales,
    sendcounts,
    displs,
    MPI_INT,
    buffer_local,
    cantidad_local,
    MPI_INT,
    0,
    MPI_COMM_WORLD
);

delete[] buffer_local;
```

## 12. Reparto irregular con MPI

Usa `MPI_Scatterv` y `MPI_Gatherv` cuando la cantidad de elementos no sea divisible exactamente entre los procesos.

### Calcula `sendcounts` y `displs`

Calcula primero la cantidad base y el resto. Entrega un elemento adicional a los primeros procesos hasta consumir el resto.

```cpp
int cantidad_base = cantidad_total / cantidad_procesos;
int cantidad_sobrante = cantidad_total % cantidad_procesos;

int* sendcounts = new int[cantidad_procesos];
int* displs = new int[cantidad_procesos];

int offset = 0;

for (int proceso = 0; proceso < cantidad_procesos; proceso++) {
    sendcounts[proceso] = cantidad_base;

    if (proceso < cantidad_sobrante) {
        sendcounts[proceso]++;
    }

    displs[proceso] = offset;
    offset += sendcounts[proceso];
}
```

Interpreta las unidades correctamente:

- Expresa `sendcounts` y `displs` en elementos del tipo indicado en `MPI_Scatterv`.
- Si repartes enteros con `MPI_INT`, cuenta enteros, no bytes.
- Si repartes filas completas, multiplica las filas asignadas por la cantidad de elementos de cada fila.

```cpp
int elementos_por_fila = columnas;
int offset_elementos = 0;

for (int proceso = 0; proceso < cantidad_procesos; proceso++) {
    sendcounts[proceso] = filas_por_proceso[proceso] * elementos_por_fila;
    displs[proceso] = offset_elementos;
    offset_elementos += sendcounts[proceso];
}
```

### Usa `MPI_Scatterv`

Obtén la cantidad local desde `sendcounts` y reserva solamente lo necesario.

```cpp
int cantidad_local = sendcounts[identificador_proceso];
int* scatter_recvbuf = nullptr;

if (cantidad_local > 0) {
    scatter_recvbuf = new int[cantidad_local];
}

MPI_Scatterv(
    datos_globales,
    sendcounts,
    displs,
    MPI_INT,
    scatter_recvbuf,
    cantidad_local,
    MPI_INT,
    0,
    MPI_COMM_WORLD
);
```

### Usa `MPI_Gatherv`

Usa conteos expresados en las mismas unidades que el búfer enviado.

```cpp
int* recvcounts = sendcounts;

MPI_Gatherv(
    scatter_recvbuf,
    cantidad_local,
    MPI_INT,
    datos_reunidos,
    recvcounts,
    displs,
    MPI_INT,
    0,
    MPI_COMM_WORLD
);
```

Si cada proceso envía filas, decide primero qué contiene el búfer:

- Si el búfer contiene un valor por fila, expresa `recvcounts` en filas.
- Si el búfer contiene todos los elementos de las filas, expresa `recvcounts` en elementos.

### Usa las operaciones MPI por su intención

- Usa `MPI_Bcast` para enviar el mismo dato desde el proceso raíz a todos los procesos.
- Usa `MPI_Scatterv` para repartir cantidades diferentes.
- Usa `MPI_Gatherv` para reunir cantidades diferentes.
- Usa `MPI_Reduce` para combinar resultados mediante suma, máximo, mínimo u otra operación.

## 13. Estructura de los ejemplos

Presenta cada ejemplo en este orden:

1. Incluye las bibliotecas y cabeceras propias.
2. Declara las funciones auxiliares.
3. Inicializa MPI, OpenMP, SIMD o CUDA según corresponda.
4. Obtén y valida los parámetros de entrada.
5. Reserva e inicializa los datos.
6. Distribuye el trabajo.
7. Ejecuta el cálculo local o paralelo.
8. Recopila o reduce los resultados.
9. Muestra un resultado breve y verificable.
10. Libera la memoria.
11. Finaliza el entorno paralelo.

Para MPI, conserva siempre este esqueleto:

```cpp
int main(int cantidad_argumentos, char** argumentos)
{
    MPI_Init(&cantidad_argumentos, &argumentos);

    int cantidad_procesos = 0;
    int identificador_proceso = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &cantidad_procesos);
    MPI_Comm_rank(MPI_COMM_WORLD, &identificador_proceso);

    // Prepara, distribuye y procesa los datos.

    MPI_Finalize();
    return 0;
}
```

## 14. Convenciones específicas para SIMD

Mantén nombres descriptivos para distinguir datos escalares, vectores SIMD y bloques de elementos.

```cpp
int cantidad_elementos;
int cantidad_bloques_simd;
int inicio_resto_escalar;
double* valores_entrada = nullptr;
double* valores_salida = nullptr;
__m256d vector_entrada;
__m256d vector_resultado;
```

### Estructura del cálculo SIMD

1. Comprueba que el procesador permita las instrucciones utilizadas.
2. Reserva e inicializa los arreglos.
3. Calcula cuántos elementos procesa cada vector SIMD.
4. Recorre los bloques completos con intrínsecos.
5. Procesa los elementos sobrantes con un bucle escalar.
6. Compara el resultado SIMD con el resultado escalar.
7. Libera la memoria.

Ejemplo con AVX2 y cuatro valores `double` por bloque:

```cpp
#include <immintrin.h>

void sumar_arreglos_simd(
    const double* arreglo_a,
    const double* arreglo_b,
    double* arreglo_resultado,
    int cantidad_elementos)
{
    const int elementos_por_vector = 4;
    int inicio_resto_escalar = cantidad_elementos
                              - cantidad_elementos % elementos_por_vector;

    for (int i = 0; i < inicio_resto_escalar; i += elementos_por_vector) {
        __m256d vector_a = _mm256_loadu_pd(&arreglo_a[i]);
        __m256d vector_b = _mm256_loadu_pd(&arreglo_b[i]);
        __m256d vector_resultado = _mm256_add_pd(vector_a, vector_b);

        _mm256_storeu_pd(&arreglo_resultado[i], vector_resultado);
    }

    // Procesa los elementos que no completan un vector AVX2.
    for (int i = inicio_resto_escalar; i < cantidad_elementos; i++) {
        arreglo_resultado[i] = arreglo_a[i] + arreglo_b[i];
    }
}
```

### Seguridad SIMD

- No ejecutes una carga SIMD fuera de los límites del arreglo.
- Usa `_mm256_loadu_pd` y `_mm256_storeu_pd` si no garantizas alineación de 32 bytes.
- Usa las versiones alineadas solamente cuando reserves y compruebes la alineación requerida.
- Procesa siempre el resto escalar cuando el tamaño no sea múltiplo del ancho SIMD.
- Mantén una versión escalar sencilla para comprobar los resultados.
- Activa en CMake la arquitectura que necesita el código, por ejemplo `-mavx2`.

### Organización de archivos SIMD

```text
fractal.h              # Declara las versiones escalar y SIMD
fractal_scalar.cpp     # Implementa la referencia escalar
fractal_simd.cpp       # Implementa los intrínsecos AVX2
params.h               # Comparte dimensiones y parámetros
```

No mezcles la versión escalar y la vectorizada en una sola función extensa. Usa la versión escalar como referencia de corrección.

## 15. Convenciones específicas para OpenMP

Usa nombres que indiquen claramente el trabajo, los hilos y los resultados locales.

```cpp
int cantidad_hilos;
int identificador_hilo;
int cantidad_elementos;
long long suma_total;
double resultado_local;
```

### Estructura del cálculo OpenMP

1. Reserva e inicializa los datos antes de entrar en la región paralela.
2. Selecciona el bucle que puede ejecutarse en paralelo.
3. Declara de forma explícita las variables privadas, compartidas o de reducción.
4. Ejecuta el cálculo paralelo.
5. Muestra el resultado fuera de la región paralela.
6. Libera la memoria.

Ejemplo con reducción:

```cpp
#include <omp.h>

long long sumar_elementos(const int* datos, int cantidad_elementos)
{
    long long suma_total = 0;

    #pragma omp parallel for default(none) \
        shared(datos, cantidad_elementos) reduction(+:suma_total)
    for (int i = 0; i < cantidad_elementos; i++) {
        suma_total += datos[i];
    }

    return suma_total;
}
```

Ejemplo con información de los hilos:

```cpp
#pragma omp parallel default(none)
{
    int identificador_hilo = omp_get_thread_num();
    int cantidad_hilos = omp_get_num_threads();

    #pragma omp critical
    {
        // Evita mezclar los mensajes de varios hilos.
        std::cout << "Hilo " << identificador_hilo
                  << " de " << cantidad_hilos << "\n";
    }
}
```

### Seguridad OpenMP

- Evita que varios hilos escriban simultáneamente en la misma posición.
- Usa `reduction` para sumas, máximos, mínimos y acumulaciones compatibles.
- Usa `critical` solamente cuando el bloque realmente necesite exclusión mutua.
- Evita modificar variables compartidas sin sincronización.
- Prefiere que cada iteración escriba en una posición distinta del arreglo.
- Usa `default(none)` en ejemplos didácticos para declarar el uso de cada variable.
- Comprueba también el resultado con un hilo mediante `set OMP_NUM_THREADS=1`.

### Organización de archivos OpenMP

```text
fractal_newton.h       # Declara parámetros y funciones
fractal_newton.cpp     # Contiene los bucles con directivas OpenMP
interfaz.h             # Declara la interfaz
interfaz.cpp           # Muestra resultados y cantidad de hilos
```

Coloca `#include <omp.h>` solamente en los `.cpp` que llamen funciones como `omp_get_thread_num()` u `omp_get_wtime()`. Las directivas `#pragma omp` no requieren incluir la cabecera por sí solas.

## 16. Convenciones específicas para CUDA

Distingue claramente los datos del anfitrión (CPU) y del dispositivo (GPU).

```cpp
int cantidad_elementos;
int hilos_por_bloque;
int cantidad_bloques;
float* datos_anfitrion = nullptr;
float* datos_dispositivo = nullptr;
float* resultado_dispositivo = nullptr;
```

Usa nombres completos para kernels y funciones auxiliares:

```cpp
__global__ void calcular_fractal_cuda(/* parámetros */);
void comprobar_error_cuda(cudaError_t codigo_error, const char* operacion);
void copiar_resultado_al_anfitrion(/* parámetros */);
```

### Estructura del cálculo CUDA

1. Comprueba que exista una GPU CUDA disponible.
2. Reserva e inicializa la memoria del anfitrión.
3. Reserva la memoria del dispositivo con `cudaMalloc`.
4. Copia las entradas con `cudaMemcpy`.
5. Calcula la cantidad de bloques.
6. Ejecuta el kernel.
7. Comprueba el lanzamiento y sincroniza.
8. Copia el resultado al anfitrión.
9. Verifica el resultado con una versión CPU.
10. Libera la memoria del dispositivo y del anfitrión.

Ejemplo de kernel con comprobación de límites:

```cpp
#include <cuda_runtime.h>

__global__ void sumar_arreglos_cuda(
    const float* arreglo_a,
    const float* arreglo_b,
    float* arreglo_resultado,
    int cantidad_elementos)
{
    int indice_global = blockIdx.x * blockDim.x + threadIdx.x;

    if (indice_global < cantidad_elementos) {
        arreglo_resultado[indice_global]
            = arreglo_a[indice_global] + arreglo_b[indice_global];
    }
}
```

Calcula la cuadrícula sin perder elementos:

```cpp
int hilos_por_bloque = 256;
int cantidad_bloques =
    (cantidad_elementos + hilos_por_bloque - 1) / hilos_por_bloque;

sumar_arreglos_cuda<<<cantidad_bloques, hilos_por_bloque>>>(
    arreglo_a_dispositivo,
    arreglo_b_dispositivo,
    arreglo_resultado_dispositivo,
    cantidad_elementos
);

cudaError_t error_lanzamiento = cudaGetLastError();
cudaError_t error_ejecucion = cudaDeviceSynchronize();
```

### Seguridad CUDA

- Comprueba `indice_global < cantidad_elementos` dentro del kernel.
- Comprueba el resultado de `cudaMalloc`, `cudaMemcpy` y `cudaDeviceSynchronize`.
- Llama a `cudaGetLastError()` después de lanzar cada kernel.
- No uses un puntero de CPU dentro del kernel ni un puntero de GPU desde código normal de CPU.
- Llama a `cudaFree` para cada reserva realizada con `cudaMalloc`.
- No llames a `cudaMalloc` cuando la cantidad de bytes sea cero.
- Usa `nullptr` para los punteros todavía no reservados.
- Mantén una versión CPU para comprobar que la GPU produce resultados correctos.

```cpp
float* datos_dispositivo = nullptr;
std::size_t cantidad_bytes =
    static_cast<std::size_t>(cantidad_elementos) * sizeof(float);

if (cantidad_bytes > 0) {
    cudaError_t codigo_error =
        cudaMalloc(reinterpret_cast<void**>(&datos_dispositivo), cantidad_bytes);

    if (codigo_error != cudaSuccess) {
        std::cerr << "No se pudo reservar memoria en la GPU: "
                  << cudaGetErrorString(codigo_error) << "\n";
    }
}

cudaFree(datos_dispositivo);
```

### Organización de archivos CUDA

```text
fractal_common.h       # Comparte estructuras simples entre CPU y GPU
fractal_cpu.h          # Declara la versión CPU
fractal_cpu.cpp        # Implementa la versión CPU de referencia
kernel.cu              # Implementa kernels y llamadas CUDA
main.cpp               # Reserva memoria, ejecuta y muestra resultados
```

Evita colocar dependencias de SFML dentro de `kernel.cu`. Pasa al kernel estructuras simples, números y punteros a memoria del dispositivo.

## 17. Resumen de reglas por tecnología

| Tema | División del trabajo | Comprobación principal | Riesgo que debes evitar |
|---|---|---|---|
| SIMD | Bloques de 4, 8 o más elementos según el tipo vectorial | Compara con la versión escalar | Leer fuera del arreglo o ignorar el resto |
| OpenMP | Iteraciones repartidas entre hilos | Ejecuta con 1 y varios hilos | Condiciones de carrera |
| MPI | Datos repartidos entre procesos | Prueba cantidades no divisibles y procesos sin datos | Conteos o desplazamientos con unidades incorrectas |
| CUDA | Bloques e hilos de GPU | Compara con la versión CPU | Acceso fuera de rango y errores CUDA no comprobados |
