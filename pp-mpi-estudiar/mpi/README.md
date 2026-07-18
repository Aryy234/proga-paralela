# Estimación de Pi con MPI (método de Monte Carlo)

Ejemplo básico de MPI en C++ pensado para aprender los conceptos esenciales
sin la complejidad de un problema real (matrices, imágenes, mallas, etc.).

## Objetivo

Estimar el valor de **Pi** usando el método de Monte Carlo, repartiendo el
trabajo entre varios procesos con MPI.

La idea del algoritmo es simple:

1. Generamos puntos aleatorios `(x, y)` dentro de un cuadrado de lado 2
   (coordenadas entre -1 y 1).
2. Contamos cuántos de esos puntos caen dentro del círculo unitario
   inscrito en ese cuadrado (`x² + y² ≤ 1`).
3. La proporción `puntos_dentro / puntos_totales` se aproxima a `Pi / 4`.
   Multiplicándola por 4 obtenemos una estimación de Pi. Cuantos más puntos
   generemos, más precisa es la estimación.

Este problema es un buen ejemplo introductorio de MPI porque:

- Cada proceso puede generar y evaluar su propia porción de puntos de forma
  **totalmente independiente**, sin necesitar datos de otros procesos
  mientras calcula (paralelismo "embarazosamente paralelo").
- Al final solo hace falta combinar **un número por proceso** (su conteo
  parcial de puntos dentro del círculo), el caso ideal para una operación
  de **reducción** (`MPI_Reduce`).
- Cubre los conceptos esenciales de MPI (inicialización, identificación de
  procesos, broadcast, reducción, medición de tiempo) sin necesitar
  estructuras de datos complejas como matrices o mallas 2D.

## Conceptos de MPI que se ponen en práctica

| Concepto | Dónde se usa en `main.cpp` |
|---|---|
| `MPI_Init` / `MPI_Finalize` | Arranque y cierre obligatorios de todo programa MPI |
| `MPI_Comm_rank` / `MPI_Comm_size` | Cada proceso descubre su identidad (`rank`) y cuántos procesos hay en total |
| `MPI_Bcast` | El proceso 0 distribuye a todos el número total de puntos a generar |
| Reparto de trabajo por rank | Cada proceso calcula cuántos puntos le tocan a él (incluyendo el resto si la división no es exacta) |
| Semillas aleatorias distintas por proceso | Evita que todos los procesos generen la misma secuencia de números aleatorios |
| `MPI_Reduce` con `MPI_SUM` | Combina los conteos parciales de todos los procesos en un solo total, disponible solo en el rank 0 |
| `MPI_Wtime` | Medición de tiempo apta para programas paralelos |

## Estructura del proyecto

```
mpi/
├── CMakeLists.txt   # Configuración de compilación (CMake + MPI)
├── main.cpp         # Único archivo fuente: todo el programa está aquí
├── .vscode/
│   └── settings.json  # Configuración de compilador/generador para VS Code
└── README.md        # Este archivo
```

Al compilar se genera además una carpeta `build/` (no versionada) con los
archivos intermedios y el ejecutable.

## Requisitos previos

- **CMake** ≥ 3.16
- Un compilador de C++ (probado con **MinGW-w64 / g++**)
- **Ninja** como generador de compilación
- **MPI** instalado — este proyecto está configurado para **Intel oneAPI
  MPI**, con `MPI_ROOT` apuntando a `C:/oneAPI/mpi/2021.18` en
  [CMakeLists.txt](CMakeLists.txt). Si tu instalación de MPI está en otra
  ruta, ajusta esa variable (o usa `find_package(MPI REQUIRED)` en su
  lugar).

## Cómo compilar

Desde la carpeta `mpi/`:

```powershell
cmake -S . -B build -G "Ninja Multi-Config" `
  -D CMAKE_CXX_COMPILER="C:/tools/mingw64/bin/g++.exe" `
  -D CMAKE_MAKE_PROGRAM="C:/tools/mingw64/bin/ninja.exe"

cmake --build build --config Release
```

Esto genera el ejecutable en `build/Release/mpi_pi.exe`.

## Cómo ejecutar

MPI necesita que las DLL de su instalación estén en el `PATH` al ejecutar.
En PowerShell:

```powershell
$env:PATH = "C:/oneAPI/mpi/2021.18/bin;" + $env:PATH

# Ejecutar con 4 procesos, usando el valor por defecto de puntos (100 millones)
& "C:/oneAPI/mpi/2021.18/bin/mpiexec.exe" -n 4 build/Release/mpi_pi.exe

# O especificando un número de puntos distinto (segundo argumento)
& "C:/oneAPI/mpi/2021.18/bin/mpiexec.exe" -n 4 build/Release/mpi_pi.exe 20000000
```

El parámetro `-n 4` indica cuántos procesos MPI se deben lanzar. Puedes
probar con distintos valores (`-n 1`, `-n 2`, `-n 8`, …) para observar cómo
cambia el tiempo de ejecución.

## Resultado esperado

El programa imprime un resumen **una sola vez**, desde el proceso 0 (que es
el único que recibe el total combinado tras el `MPI_Reduce`):

```
Procesos utilizados:        4
Puntos totales generados:   20000000
Puntos dentro del circulo:  15706441
Estimacion de Pi:           3.141288
Valor real de Pi:           3.141593
Tiempo transcurrido:        0.0536 segundos
```

La estimación de Pi variará ligeramente en cada ejecución (porque se basa
en números aleatorios), pero debería acercarse cada vez más a `3.141593`
cuantos más puntos totales se generen. Al aumentar el número de procesos
con la misma cantidad de puntos, el tiempo transcurrido debería reducirse,
mostrando el beneficio de repartir el trabajo entre varios procesos.
