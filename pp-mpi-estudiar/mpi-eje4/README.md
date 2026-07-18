# Ejercicio 4 — Estadísticas de un arreglo grande (MPI)

Solución al Ejercicio 4 de `Ejercicios_Propuestos_Examen_MPI.md`: practicar el
patrón repartir-procesar-combinar con operaciones colectivas, sin usar
matrices.

## Qué hace

1. El proceso 0 genera un arreglo de `N` números de punto flotante aleatorios
   entre 0 y 100 (`N` = 1,000,000 por defecto).
2. Reparte el arreglo entre todos los procesos con `MPI_Scatterv` (se usa la
   variante "v" para repartir también el resto cuando `N` no es múltiplo
   exacto de la cantidad de procesos, en vez de ignorarlo).
3. Cada proceso calcula el mínimo, el máximo y la suma de su porción.
4. Se combinan los resultados parciales con tres `MPI_Reduce` (`MPI_MIN`,
   `MPI_MAX`, `MPI_SUM`) en el proceso 0, que imprime el promedio, el mínimo
   y el máximo globales.
5. Antes de repartir nada, el proceso 0 calcula las mismas estadísticas de
   forma puramente secuencial y las compara contra el resultado paralelo,
   para detectar cualquier error de cálculo.

## Estructura

```
mpi-eje4/
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

Desde la carpeta `mpi-eje4/`:

```powershell
cmake -S . -B build -G "Ninja Multi-Config" `
  -D CMAKE_CXX_COMPILER="C:/tools/mingw64/bin/g++.exe" `
  -D CMAKE_MAKE_PROGRAM="C:/tools/mingw64/bin/ninja.exe"

cmake --build build --config Release
```

Esto genera el ejecutable en `build/Release/mpi_estadisticas.exe`.

## Cómo ejecutar

```powershell
$env:PATH = "C:/oneAPI/mpi/2021.18/bin;" + $env:PATH

# Con 4 procesos, usando el valor por defecto de N (1,000,000)
& "C:/oneAPI/mpi/2021.18/bin/mpiexec.exe" -n 4 build/Release/mpi_estadisticas.exe

# O especificando N (para probar reparto no exacto, por ejemplo con -n 3)
& "C:/oneAPI/mpi/2021.18/bin/mpiexec.exe" -n 3 build/Release/mpi_estadisticas.exe 1000001
```

## Resultado esperado

```
Procesos utilizados:      4
Cantidad de numeros:      1000000

--- Resultado paralelo (MPI) ---
Minimo:    0.0002
Maximo:    99.9987
Promedio:  49.9912

--- Resultado secuencial (verificacion) ---
Minimo:    0.0002
Maximo:    99.9987
Promedio:  49.9912

Los resultados paralelo y secuencial coinciden: SI
```

El promedio se compara con una pequeña tolerancia (no con igualdad exacta)
porque la suma en punto flotante no es asociativa: sumar en orden secuencial
puede diferir en los últimos decimales de sumar por bloques y combinar
(que es lo que hace `MPI_Reduce`), aunque ambos resultados sean correctos.
