# Ejercicio 9 — Suma de dos matrices por filas (MPI)

Solución al Ejercicio 9 de `Ejercicios_Propuestos_Examen_MPI.md`: el ejemplo
más simple de repartir una matriz por filas con `MPI_Scatter` y volver a
juntarla con `MPI_Gather`.

## Qué hace

1. El proceso 0 genera dos matrices cuadradas `A` y `B` de tamaño `N x N`
   (`N` = 4 por defecto, valores enteros aleatorios entre 0 y 9).
2. Reparte `A` y `B` por bloques de filas con **dos** llamadas separadas a
   `MPI_Scatter` (una por matriz).
3. Cada proceso suma elemento a elemento las filas que le tocaron
   (`C_local = A_local + B_local`).
4. El resultado se junta de vuelta en el proceso 0 con `MPI_Gather` y se
   imprime `C` completa.

Las matrices se representan como arreglos 1D en row-major
(`datos[fila * N + columna]`), el formato que esperan `MPI_Scatter`/
`MPI_Gather` para repartir bloques de filas contiguos en memoria.

Se asume que `N` es múltiplo de la cantidad de procesos (simplificación que
permite el enunciado); si no lo es, el programa lo detecta explícitamente y
aborta con un mensaje de error en vez de repartir mal las filas en silencio.

## Estructura

```
mpi-eje9/
├── CMakeLists.txt
├── main.cpp
├── .vscode/
│   └── settings.json
└── README.md
```

## Requisitos previos

Los mismos que en `mpi/README.md`: CMake ≥ 3.16, MinGW-w64/g++, Ninja e
Intel oneAPI MPI en `C:/oneAPI/mpi/2021.18` (ajustar `MPI_ROOT` en
[CMakeLists.txt](CMakeLists.txt) si tu instalación está en otra ruta).

## Cómo compilar

Desde la carpeta `mpi-eje9/`:

```powershell
cmake -S . -B build -G "Ninja Multi-Config" `
  -D CMAKE_CXX_COMPILER="C:/tools/mingw64/bin/g++.exe" `
  -D CMAKE_MAKE_PROGRAM="C:/tools/mingw64/bin/ninja.exe"

cmake --build build --config Release
```

Esto genera el ejecutable en `build/Release/mpi_eje9.exe`.

## Cómo ejecutar

```powershell
$env:PATH = "C:/oneAPI/mpi/2021.18/bin;" + $env:PATH

# Con 2 procesos, usando el valor por defecto de N (4)
& "C:/oneAPI/mpi/2021.18/bin/mpiexec.exe" -n 2 build/Release/mpi_eje9.exe

# O especificando N (debe ser multiplo de -n)
& "C:/oneAPI/mpi/2021.18/bin/mpiexec.exe" -n 4 build/Release/mpi_eje9.exe 8
```

## Resultado esperado

```
Matriz A:
  3.0   1.0   9.0   3.0
  ...
Matriz B:
  0.0   6.0   4.0   1.0
  ...
Matriz C = A + B (calculada en paralelo):
  3.0   7.0  13.0   4.0
  ...
```

Los valores exactos varían en cada ejecución porque `A` y `B` se generan con
números aleatorios.
