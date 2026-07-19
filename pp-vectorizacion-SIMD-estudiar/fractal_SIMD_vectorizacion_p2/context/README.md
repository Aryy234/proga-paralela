# Burning Ship SIMD

Proyecto en C++ para dibujar el fractal Burning Ship usando vectorización SIMD con AVX2, manteniendo una versión escalar de referencia para comparación y validación.

## Objetivo

El programa calcula el fractal Burning Ship sobre una ventana gráfica con SFML, usando:

- una implementación escalar,
- una implementación vectorizada SIMD,
- una paleta fija de 16 colores,
- overlay con fuente Arial embebida.

## Estructura del proyecto

- `main.cpp`: interfaz gráfica, eventos, renderizado y overlay.
- `fractal.h`: interfaz común de los kernels.
- `fractal_scalar.cpp`: versión escalar de referencia.
- `fractal_simd.cpp`: kernel SIMD con AVX2.
- `params.h`: constantes del problema.
- `palette.h` / `palette.cpp`: rampa de color de 16 tonos.
- `arial_ttf.h`: fuente Arial embebida para el overlay.

## Dependencias

- C++20
- CMake 3.16 o superior
- SFML 3
- fmt
- Compilador con soporte AVX2

## Requisitos del entorno

En este proyecto se usó:

- `Winlibs GCC 15.2.0`
- `vcpkg`
- SFML instalada mediante `vcpkg`

Ruta útil en este entorno:

- `C:/tools/vcpkg-2026.03.18/installed/x64-mingw-dynamic`

## Compilación

Si usas el mismo entorno, configura así:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/tools/vcpkg-2026.03.18/installed/x64-mingw-dynamic"
cmake --build build
```

Si CMake sigue sin encontrar SFML, asegúrate de usar también el toolchain de vcpkg:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/tools/vcpkg-2026.03.18/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic
cmake --build build
```

## Ejecución

```powershell
./build/burning_ship_simd.exe
```

## Controles

- `Up`: aumenta `max_iter`
- `Down`: disminuye `max_iter`
- `Esc`: cierra la aplicación

## Detalles de implementación

- El fractal se calcula por filas para facilitar el acceso contiguo a memoria.
- El kernel SIMD procesa bloques contiguos de píxeles y deja un tramo escalar para el remanente.
- La paleta se usa para convertir iteraciones en color.
- El overlay usa Arial desde `arial_ttf.h`, sin depender de archivos externos en ejecución.

## Verificación

El proyecto compila correctamente con la configuración indicada.

## Observación

La salida visual puede alternarse entre la versión escalar y la SIMD para comparar resultados.
