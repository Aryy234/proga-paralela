# Guía completa para crear plantillas de fractales con MPI y SFML

## 1. Propósito de esta guía

Este documento explica la arquitectura, las convenciones y el flujo de ejecución del proyecto de fractales. Úsalo como especificación para entender el programa actual o para generar una plantilla nueva que conserve MPI, SFML, el histograma y la interfaz, pero utilice otra fórmula fractal.

La separación principal es la siguiente:

- `main.cpp` administra MPI, distribuye filas, coordina los procesos, recibe la imagen y controla SFML.
- `fractal.cpp` transforma cada píxel en un punto matemático, aplica la fórmula fractal y llena el histograma local.
- `palette.cpp` define los colores asociados con las iteraciones.
- `draw_text.cpp` contiene el soporte directo de FreeType.
- `arial_ttf.h` contiene la fuente Arial embebida en memoria.
- `CMakeLists.txt` configura las dependencias y genera el ejecutable.

Al crear otro fractal, conserva la infraestructura de comunicación y cambia únicamente lo relacionado con la matemática cuando sea posible.

---

## 2. Tecnologías y responsabilidades

| Tecnología | Responsabilidad |
|---|---|
| C++ | Estructura general, cálculo numérico, memoria y organización por archivos. |
| MPI | Ejecutar varios procesos, difundir parámetros, enviar bloques de píxeles y reunir histogramas. |
| SFML 3 | Crear la ventana, la textura, el sprite, procesar el teclado y dibujar el overlay. |
| fmt | Construir mensajes y textos de manera legible. |
| FreeType | Cargar la fuente embebida y permitir dibujo de texto. |
| CMake | Configurar compilación, includes y bibliotecas. |
| Intel MPI | Implementación de MPI usada por el proyecto. |

MPI trabaja con **procesos independientes**, no con hilos. Cada rank tiene su propia copia de las variables globales, su propia memoria y su propio `pixel_buffer`. Modificar una variable en rank 0 no la modifica automáticamente en los demás ranks. Para sincronizarla, usa una operación MPI, en este proyecto `MPI_Bcast`.

---

## 3. Estructura de carpetas y archivos

```text
proyecto/
├── .vscode/
│   └── settings.json       Configuración local de CMake, Ninja, GCC y vcpkg
├── build/                  Archivos generados; no forma parte del código fuente
├── CMakeLists.txt          Definición del ejecutable y sus dependencias
├── README.md               Instrucciones del entorno y compilación
├── ENUNCIADO_P2.MD         Requisitos originales del ejercicio
├── contexto.txt            Convenciones didácticas para escribir el código
├── main.cpp                MPI, interfaz, eventos, ensamblado y overlay
├── fractal.h               Contrato público del módulo matemático
├── fractal.cpp             Kernel fractal y recorrido de píxeles locales
├── palette.h               Tamaño y declaración externa de la paleta
├── palette.cpp             Implementación de la paleta RGBA
├── draw_text.h             Contrato del soporte directo de FreeType
├── draw_text.cpp           Inicialización y rasterización con FreeType
├── arial.ttf               Archivo original de la fuente
└── arial_ttf.h             Bytes de la fuente embebidos en el ejecutable
```

### 3.1 Archivos fuente frente a archivos generados

No edites manualmente el contenido de `build/`. CMake y Ninja lo regeneran. Cuando quieras producir una plantilla nueva, copia los archivos fuente y deja que cada entorno cree su propia carpeta `build`.

No incluyas rutas de `build` dentro del código. El ejecutable debe depender solamente de los archivos fuente y de las bibliotecas configuradas por CMake.

---

## 4. Convenciones de codificación

### 4.1 Idioma y nombres

Escribe comentarios, mensajes y nombres nuevos en español. Usa nombres que indiquen propósito y unidad:

```cpp
int filas_locales;
int iteracion;
int histograma_local[PALETTE_SIZE];
double paso_x;
double paso_y;
uint32_t* buffer_pixeles;
```

Evita nombres que obliguen a adivinar su significado:

```cpp
int x1;
int aux;
int n;
int tmp;
```

MPI exige algunos nombres convencionales, como `rank` y `nprocs`. Puedes conservarlos porque son reconocibles en programación paralela. Para nombres nuevos, prefiere `proceso_origen`, `filas_del_proceso` o `cantidad_pixeles`.

### 4.2 Tipos

- Usa `int` para ranks, filas, conteos MPI, bins e iteraciones.
- Usa `double` para coordenadas y operaciones matemáticas del fractal.
- Usa `uint32_t` para un píxel RGBA completo.
- Usa arreglos C cuando el tamaño sea pequeño y conocido.
- Usa `new[]` y `delete[]` cuando el tamaño dependa de `nprocs`, `WIDTH`, `HEIGHT` o `delta`.
- Usa `nullptr`, nunca `NULL`.

Ejemplo:

```cpp
int histograma_local[16] = {0};
int* histogramas_reunidos = new int[nprocs * 16];

// Usar los arreglos.

delete[] histogramas_reunidos;
```

Cada `new[]` debe tener exactamente un `delete[]` ejecutado por el mismo proceso propietario.

### 4.3 Bucles y condiciones

Usa bucles explícitos:

```cpp
for (int bin = 0; bin < PALETTE_SIZE; bin++) {
    histograma_local[bin] = 0;
}
```

Usa variables auxiliares para aclarar índices:

```cpp
int posicion = proceso * PALETTE_SIZE + bin;
histograma_global[bin] += histogramas_reunidos[posicion];
```

Comprueba siempre tamaños iguales a cero antes de acceder a un búfer. En MPI, enviar o recibir cero elementos es válido si todos los procesos conservan el protocolo correcto.

### 4.4 Comentarios

Explica la intención, no la sintaxis evidente:

```cpp
// Reúne un histograma independiente por cada proceso.
MPI_Gather(...);
```

Evita comentarios como `// incrementa i`, porque `i++` ya lo expresa.

---

## 5. Organización de `main.cpp`

`main.cpp` contiene cinco responsabilidades:

1. Declarar el estado compartido conceptualmente por todos los ranks.
2. Inicializar MPI y calcular el bloque de filas de cada rank.
3. Ejecutar la interfaz solamente en rank 0.
4. Ejecutar el ciclo de trabajo en los ranks secundarios.
5. Liberar memoria y finalizar MPI.

### 5.1 Orden de inclusiones

Mantén las inclusiones agrupadas:

```cpp
// Biblioteca estándar.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

// Bibliotecas externas.
#include <fmt/core.h>
#include <SFML/Graphics.hpp>
#include <mpi.h>

// Módulos del proyecto.
#include "fractal.h"
#include "palette.h"
#include "draw_text.h"
```

Incluye en cada archivo aquello que realmente usa. No dependas de que otro header incluya accidentalmente una biblioteca.

### 5.2 Constantes de imagen

```cpp
#define WIDTH 1600
#define HEIGHT 900
```

- `WIDTH` es el número total de columnas.
- `HEIGHT` es el número total de filas.
- Cada píxel ocupa un `uint32_t`.
- El número total de píxeles es `WIDTH * HEIGHT`.

No confundas píxeles con bytes. Para reservar se cuenta en elementos; para `memset` se cuenta en bytes:

```cpp
pixel_buffer = new uint32_t[WIDTH * delta];
std::memset(pixel_buffer, 0, WIDTH * delta * sizeof(uint32_t));
```

### 5.3 Estado del programa

Las variables principales son:

| Variable | Significado |
|---|---|
| `pixel_buffer` | Bloque local de píxeles calculado por un rank. |
| `texture_buffer` | Imagen completa; solo rank 0 la necesita. |
| `running` | Vale 1 mientras el programa continúa y 0 cuando debe terminar. |
| `nprocs` | Número total de procesos MPI. |
| `rank` | Identificador del proceso actual, desde 0 hasta `nprocs - 1`. |
| `row_start` | Primera fila global asignada al rank. |
| `row_end` | Primera fila global no incluida en el bloque. |
| `delta` | Capacidad máxima de filas por bloque. |
| `padding` | Filas sobrantes de la división rectangular. |
| `max_iteraciones` | Límite de iteraciones del kernel. |
| `x_min`, `x_max` | Límites horizontales del plano matemático. |
| `y_min`, `y_max` | Límites verticales del plano matemático. |

Para la vista general actual:

```cpp
int max_iteraciones = 100;
double x_min = -2.0;
double x_max = 1.0;
double y_min = -1.5;
double y_max = 1.5;
```

Al cambiar de fractal, revisa siempre estos límites. Una fórmula correcta puede producir una pantalla vacía si el dominio no contiene su región interesante.

### 5.4 Variables globales y MPI

Las variables globales no son memoria compartida entre ranks. Después de `mpiexec -n 4`, existen cuatro copias independientes de `max_iteraciones`, `x_min`, etc.

Rank 0 cambia las variables por medio de la interfaz. Después las transmite:

```cpp
std::array<int, 2> estado = {max_iteraciones, running};
std::array<double, 4> plano = {x_min, y_min, x_max, y_max};

MPI_Bcast(estado.data(), 2, MPI_INT, 0, MPI_COMM_WORLD);
MPI_Bcast(plano.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```

Los ranks secundarios llaman las mismas colectivas, en el mismo orden, y copian los valores recibidos a sus variables locales.

Si el fractal nuevo necesita parámetros adicionales, por ejemplo `constante_real`, `constante_imaginaria` o `potencia`, añádelos al paquete correspondiente y actualiza el conteo de `MPI_Bcast`.

Ejemplo:

```cpp
std::array<double, 6> parametros = {
    x_min, y_min, x_max, y_max,
    constante_real, constante_imaginaria
};

MPI_Bcast(parametros.data(), 6, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```

El emisor y todos los receptores deben usar exactamente el mismo tipo, cantidad, raíz y comunicador.

---

## 6. Inicialización de MPI

El orden correcto es:

```cpp
MPI_Init(&argc, &argv);
MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
```

No uses `rank` ni `nprocs` antes de obtenerlos. Después del último uso de MPI, ejecuta:

```cpp
MPI_Finalize();
```

No llames `MPI_Bcast`, `MPI_Send`, `MPI_Recv` ni `MPI_Gather` después de `MPI_Finalize`.

---

## 7. Descomposición de la imagen por filas

La imagen se divide horizontalmente. Cada proceso calcula filas completas, lo que mantiene contiguos los píxeles enviados.

### 7.1 Cálculo usado por el proyecto

```cpp
delta = std::ceil(HEIGHT * 1.0 / nprocs);
row_start = rank * delta;
row_end = row_start + delta;
padding = delta * nprocs - HEIGHT;

if (row_end > HEIGHT) {
    row_end = HEIGHT;
}
```

El intervalo es semiabierto:

```text
[row_start, row_end)
```

Incluye `row_start` y excluye `row_end`. Por eso:

```cpp
int filas_locales = row_end - row_start;
```

### 7.2 Ejemplo con padding

Para `HEIGHT = 10` y `nprocs = 3`:

```text
delta = ceil(10 / 3) = 4

rank 0: filas [0, 4)  -> 4 filas
rank 1: filas [4, 8)  -> 4 filas
rank 2: filas [8, 10) -> 2 filas
padding = 4 * 3 - 10 = 2
```

El último proceso reserva capacidad para cuatro filas, pero calcula y envía únicamente dos.

### 7.3 Más procesos que filas

Si `nprocs > HEIGHT`, algunos ranks no reciben filas. Trata esos casos de forma segura:

```cpp
int filas_locales = row_end - row_start;
if (filas_locales < 0) {
    filas_locales = 0;
}
```

No escribas en `pixel_buffer[0]` si el rank tiene cero filas. La llamada `MPI_Send` con cantidad cero sigue siendo válida.

---

## 8. Conversión de píxel a coordenada matemática

El kernel no trabaja directamente con columnas y filas. Primero convierte cada píxel `(columna, fila)` en un punto `(cx, cy)`.

### 8.1 Tamaño del paso

```cpp
double paso_x = (x_max - x_min) / (width - 1.0);
double paso_y = (y_max - y_min) / (height - 1.0);
```

Se divide entre `width - 1` y `height - 1` para que el primer y el último píxel coincidan con los extremos del dominio.

### 8.2 Coordenada horizontal

```cpp
double cx = x_min + columna * paso_x;
```

- Columna 0 produce `x_min`.
- Columna `width - 1` produce `x_max`.

### 8.3 Coordenada vertical

```cpp
double cy = y_max - fila * paso_y;
```

La resta es necesaria porque en una imagen la fila 0 está arriba, mientras que en el plano cartesiano los valores positivos de `y` se representan arriba.

Si usas `y_min + fila * paso_y`, la imagen aparece reflejada verticalmente. Esto puede ser válido si la fórmula o la vista solicitada lo requiere, pero debe ser una decisión consciente.

### 8.4 Relación de aspecto

La imagen tiene relación:

```text
WIDTH / HEIGHT = 1600 / 900 ≈ 1.7778
```

El dominio actual tiene:

```text
(x_max - x_min) / (y_max - y_min) = 3 / 3 = 1
```

Por tanto, la figura matemática se ve estirada horizontalmente. Para conservar proporciones, ajusta uno de los rangos para que la relación del dominio se aproxime a la relación de la ventana. No lo hagas si el profesor exige límites concretos.

---

## 9. Layout de los búferes

### 9.1 Búfer local

Cada rank almacena su bloque desde la posición cero, aunque calcule filas globales distintas:

```cpp
int fila_local = fila_global - row_start;
int indice_local = fila_local * width + columna;
pixel_buffer[indice_local] = color;
```

No uses `fila_global * width + columna` dentro del búfer local. Eso escribiría fuera del bloque en todos los ranks excepto el cero.

### 9.2 Imagen completa en rank 0

Rank 0 copia un bloque en su posición global:

```cpp
int indice_destino = fila_inicial_del_proceso * WIDTH;
std::memcpy(texture_buffer + indice_destino,
            pixel_buffer,
            filas_del_proceso * WIDTH * sizeof(uint32_t));
```

`std::memcpy` recibe la cantidad en bytes. Por eso debe multiplicarse por `sizeof(uint32_t)`.

### 9.3 Formato del color

SFML recibe cuatro bytes por píxel en orden RGBA. El proyecto almacena el píxel en un `uint32_t` y adapta los bytes de la paleta con `bswap32` para la arquitectura usada.

El negro opaco es:

```cpp
0xFF000000
```

En la máquina little-endian usada, sus bytes en memoria quedan preparados para la actualización de la textura. Si la plantilla se mueve a otra representación o arquitectura, verifica el orden de canales con una prueba de colores sólidos.

---

## 10. El módulo matemático `fractal.h` y `fractal.cpp`

### 10.1 Contrato público

`fractal.h` declara la función que conoce `main.cpp`:

```cpp
void burning_mpi(double x_min, double y_min,
                 double x_max, double y_max,
                 uint32_t width, uint32_t height,
                 uint32_t row_start, uint32_t row_end,
                 uint32_t* pixel_buffer,
                 int* histograma_local);
```

El contrato indica:

- El dominio completo.
- Las dimensiones completas de la imagen.
- Las filas globales asignadas al rank.
- El búfer local donde escribir colores.
- El histograma local donde contar escapes.

Para una plantilla nueva, usa un nombre genérico como:

```cpp
void calcular_fractal_mpi(...);
```

Así podrás cambiar la fórmula sin renombrar llamadas en varios archivos. Si el ejercicio exige que la función lleve el nombre del fractal, conserva un nombre específico.

### 10.2 Función privada por punto

La función privada calcula un solo punto:

```cpp
static uint32_t calcular_color_burning_ship(
    double cx,
    double cy,
    int& iteracion);
```

El uso de `static` a nivel de archivo evita exponerla fuera de `fractal.cpp`. El parámetro `int& iteracion` permite devolver dos resultados:

- El color mediante `return`.
- La iteración de escape mediante la referencia.

Esta separación es importante. El color se utiliza en la imagen y la iteración se utiliza en el histograma.

### 10.3 Estructura de un kernel de escape

Todo fractal de escape puede seguir esta estructura:

```cpp
static uint32_t calcular_color(double cx, double cy, int& iteracion)
{
    // 1. Inicializa el estado matemático.
    double zr = 0.0;
    double zi = 0.0;
    iteracion = 0;

    // 2. Repite mientras no alcance el límite ni escape.
    while (iteracion < max_iteraciones && condicion_de_no_escape) {
        // 3. Calcula el nuevo estado usando valores anteriores.
        double siguiente_zr = /* fórmula real */;
        double siguiente_zi = /* fórmula imaginaria */;

        // 4. Actualiza ambas componentes después de calcularlas.
        zr = siguiente_zr;
        zi = siguiente_zi;
        iteracion++;
    }

    // 5. Distingue interior y escape.
    if (iteracion >= max_iteraciones) {
        return 0xFF000000;
    }

    // 6. Selecciona el color.
    return color_ramp[iteracion % PALETTE_SIZE];
}
```

Calcula `siguiente_zr` y `siguiente_zi` antes de modificar `zr` o `zi`. Si actualizas `zr` primero y luego lo usas para calcular `zi`, mezclas estados de iteraciones diferentes y cambias la fórmula.

### 10.4 Burning Ship actual

La recurrencia es:

```text
z(0) = 0
z(n+1) = (|Re(z(n))| + i|Im(z(n))|)^2 + c
c = cx + i cy
```

Separada en componentes:

```cpp
double valor_absoluto_real = std::abs(zr);
double valor_absoluto_imaginario = std::abs(zi);

double siguiente_zr =
    valor_absoluto_real * valor_absoluto_real
    - valor_absoluto_imaginario * valor_absoluto_imaginario
    + cx;

double siguiente_zi =
    2.0 * valor_absoluto_real * valor_absoluto_imaginario
    + cy;
```

El punto escapa cuando:

```cpp
zr * zr + zi * zi > 4.0
```

Se compara el cuadrado de la norma con 4 para evitar calcular una raíz cuadrada.

---

## 11. Cómo sustituir la fórmula fractal

### 11.1 Elementos que debes identificar primero

Antes de programar otro fractal, define por escrito:

1. Estado inicial de `z`.
2. Significado de cada píxel: ¿representa `c` o representa `z(0)`?
3. Constantes adicionales.
4. Fórmula de recurrencia.
5. Condición de escape o convergencia.
6. Número máximo de iteraciones.
7. Dominio recomendado.
8. Regla para colorear puntos que no escapan o que convergen.
9. Regla para construir el histograma.

No copies una fórmula sin aclarar si el píxel representa `c` o `z(0)`. Esa diferencia separa, por ejemplo, Mandelbrot de Julia.

### 11.2 Mandelbrot

```text
z(0) = 0
z(n+1) = z(n)^2 + c
c = punto del píxel
```

Componentes:

```cpp
double siguiente_zr = zr * zr - zi * zi + cx;
double siguiente_zi = 2.0 * zr * zi + cy;
```

Dominio típico:

```cpp
double x_min = -2.5;
double x_max = 1.0;
double y_min = -1.2;
double y_max = 1.2;
```

Para pasar de Burning Ship a Mandelbrot, elimina los valores absolutos. El resto de la infraestructura puede permanecer igual.

### 11.3 Julia cuadrático

```text
z(0) = punto del píxel
z(n+1) = z(n)^2 + k
k = constante fija
```

Inicialización:

```cpp
double zr = cx;
double zi = cy;
```

Recurrencia:

```cpp
double siguiente_zr = zr * zr - zi * zi + constante_real;
double siguiente_zi = 2.0 * zr * zi + constante_imaginaria;
```

En este caso, `constante_real` y `constante_imaginaria` deben existir en todos los ranks. Inclúyelas en un `MPI_Bcast` o mantenlas como constantes idénticas compiladas en todos los procesos.

Dominio típico:

```cpp
double x_min = -1.5;
double x_max = 1.5;
double y_min = -1.5;
double y_max = 1.5;
```

### 11.4 Tricorn o Mandelbar

```text
z(n+1) = conjugado(z(n))^2 + c
```

Componentes:

```cpp
double siguiente_zr = zr * zr - zi * zi + cx;
double siguiente_zi = -2.0 * zr * zi + cy;
```

La diferencia principal es el signo negativo de la componente imaginaria.

### 11.5 Potencias superiores

Para `z(n+1) = z(n)^p + c`, evita introducir `std::pow(std::complex)` si el curso exige operaciones vistas en clase. Implementa primero casos concretos con componentes reales e imaginarias.

Si `p = 3`:

```cpp
double siguiente_zr = zr * zr * zr - 3.0 * zr * zi * zi + cx;
double siguiente_zi = 3.0 * zr * zr * zi - zi * zi * zi + cy;
```

La condición de escape puede seguir usando norma al cuadrado mayor que 4, salvo que la especificación del fractal indique otro radio.

### 11.6 Fractales de Newton

Un fractal de Newton no usa únicamente escape. Itera hacia raíces y colorea según:

- La raíz a la que converge.
- La cantidad de iteraciones necesarias.

Para adaptarlo, cambia la función por punto para devolver también un identificador de raíz. El histograma puede contar iteraciones de convergencia. La infraestructura MPI de filas, envío de píxeles y reunión de histogramas sigue siendo válida.

Documenta claramente que el negro ya no significa necesariamente “no escapó”; puede significar “no convergió dentro del límite”.

---

## 12. Histograma de iteraciones

Cada rank mantiene:

```cpp
int histograma_local[PALETTE_SIZE] = {0};
```

El kernel debe reiniciarlo antes de calcular un cuadro:

```cpp
for (int bin = 0; bin < PALETTE_SIZE; bin++) {
    histograma_local[bin] = 0;
}
```

Para cada píxel que escapa:

```cpp
int bin = iteracion * PALETTE_SIZE / max_iteraciones;
if (bin >= PALETTE_SIZE) {
    bin = PALETTE_SIZE - 1;
}
histograma_local[bin]++;
```

No cuentes los puntos interiores si la definición exige “por cada píxel que escapó”. La condición actual es:

```cpp
if (iteracion < max_iteraciones) {
    // Contar escape.
}
```

### 12.1 Reunión con `MPI_Gather`

Rank 0 reserva:

```cpp
int* histogramas_reunidos = new int[nprocs * PALETTE_SIZE];
```

Todos los ranks llaman:

```cpp
MPI_Gather(histograma_local, PALETTE_SIZE, MPI_INT,
           histogramas_reunidos, PALETTE_SIZE, MPI_INT,
           0, MPI_COMM_WORLD);
```

En ranks distintos de cero, el búfer receptor puede ser `nullptr` porque solamente la raíz recibe:

```cpp
MPI_Gather(histograma_local, PALETTE_SIZE, MPI_INT,
           nullptr, PALETTE_SIZE, MPI_INT,
           0, MPI_COMM_WORLD);
```

El layout recibido es:

```text
[hist rank 0][hist rank 1]...[hist rank nprocs-1]
```

Rank 0 suma por bin:

```cpp
for (int bin = 0; bin < PALETTE_SIZE; bin++) {
    histograma_global[bin] = 0;
    for (int proceso = 0; proceso < nprocs; proceso++) {
        int posicion = proceso * PALETTE_SIZE + bin;
        histograma_global[bin] += histogramas_reunidos[posicion];
    }
}
```

Una variante posible es `MPI_Reduce` con `MPI_SUM`, pero no la sustituyas si el enunciado exige expresamente `MPI_Gather`.

---

## 13. Protocolo MPI de un cuadro

Todas las operaciones deben aparecer en el mismo orden lógico en todos los ranks.

### 13.1 Rank 0

```text
1. Procesa eventos SFML.
2. Prepara estado y dominio.
3. MPI_Bcast del estado.
4. MPI_Bcast del dominio.
5. Si running == 0, termina el ciclo.
6. Calcula sus filas y su histograma local.
7. Copia su bloque a la imagen completa.
8. MPI_Recv de cada rank secundario.
9. Copia cada bloque recibido en la imagen completa.
10. MPI_Gather de histogramas.
11. Suma los histogramas locales.
12. Actualiza textura, overlay y ventana.
```

### 13.2 Ranks secundarios

```text
1. MPI_Bcast del estado.
2. MPI_Bcast del dominio.
3. Si running == 0, termina el ciclo.
4. Calcula sus filas y su histograma local.
5. MPI_Send del bloque de píxeles.
6. MPI_Gather del histograma.
7. Repite.
```

### 13.3 Invariante fundamental

Nunca permitas esta secuencia:

```text
rank 0: MPI_Bcast -> MPI_Gather
rank 1: MPI_Gather -> MPI_Bcast
```

Los procesos quedarían esperando operaciones diferentes. Las colectivas deben llamarse en el mismo orden y todos los integrantes de `MPI_COMM_WORLD` deben participar.

---

## 14. Envío y ensamblado de píxeles

Cada rank secundario envía:

```cpp
int cantidad_pixeles = WIDTH * filas_locales;

MPI_Send(pixel_buffer,
         cantidad_pixeles,
         MPI_UINT32_T,
         0,
         0,
         MPI_COMM_WORLD);
```

Rank 0 recibe indicando el mismo origen, tag y tipo:

```cpp
MPI_Recv(pixel_buffer,
         WIDTH * filas_del_proceso,
         MPI_UINT32_T,
         proceso_origen,
         0,
         MPI_COMM_WORLD,
         MPI_STATUS_IGNORE);
```

La cantidad del receptor debe ser al menos la cantidad enviada. Mantén la unidad en píxeles `uint32_t`, no en bytes, porque el tipo MPI ya es `MPI_UINT32_T`.

El tag `0` identifica los mensajes de imagen. Si la plantilla añade mensajes punto a punto de otra clase, usa tags distintos y documentados.

---

## 15. Interfaz SFML

Solo rank 0 debe crear una ventana. Si todos los ranks crean una, aparecerán múltiples ventanas y se duplicará innecesariamente el uso gráfico.

### 15.1 Objetos principales

```cpp
sf::RenderWindow window(...);
sf::Texture texture;
sf::Sprite sprite(texture);
sf::Font font(...);
sf::Text text(...);
```

Flujo de dibujo:

```cpp
texture.update((const std::uint8_t*)texture_buffer);

window.clear();
window.draw(sprite);
window.draw(text);
window.draw(textOptions);
window.display();
```

Primero dibuja el sprite y después los textos. Si dibujas el sprite al final, cubrirá el overlay.

### 15.2 Eventos

SFML 3 usa eventos opcionales:

```cpp
while (auto event = window.pollEvent()) {
    // Procesar evento.
}
```

Las teclas actuales son:

- Flecha arriba: aumenta `max_iteraciones` en 10.
- Flecha abajo: disminuye `max_iteraciones` en 10, con mínimo de 10.
- Escape: establece `running = 0` y cierra la ventana.

Al cerrar, rank 0 debe difundir `running = 0` exactamente una vez. Los workers reciben el cierre y salen antes del cálculo. No añadas un segundo `MPI_Bcast` después de que los workers hayan abandonado su ciclo.

### 15.3 Overlay

El overlay muestra:

- Nombre del fractal.
- Cantidad de ranks.
- Máximo de iteraciones.
- FPS.
- Dominio.
- Histograma global.
- Ayuda de teclado.

Al cambiar la fórmula, actualiza el nombre visible. No dejes `Burning Ship` si el kernel ahora calcula Mandelbrot o Julia.

---

## 16. Paleta de colores

`palette.h` define:

```cpp
#define PALETTE_SIZE 16
extern std::vector<uint32_t> color_ramp;
```

`extern` declara que la variable existe, pero no reserva otra copia. La definición real aparece una sola vez en `palette.cpp`.

El índice actual es cíclico:

```cpp
int indice_color = iteracion % PALETTE_SIZE;
return color_ramp[indice_color];
```

Esto produce bandas repetidas. Otra opción es asociar directamente el bin normalizado:

```cpp
int indice_color = iteracion * PALETTE_SIZE / max_iteraciones;
if (indice_color >= PALETTE_SIZE) {
    indice_color = PALETTE_SIZE - 1;
}
```

No cambies simultáneamente fórmula y paleta durante una prueba inicial. Primero verifica la geometría con la paleta existente; después ajusta los colores.

---

## 17. FreeType y fuente embebida

`arial_ttf.h` define un arreglo de bytes con la fuente. Esto permite cargarla sin depender de una ruta externa durante la ejecución.

El namespace se declara en `main.cpp`:

```cpp
namespace arial_ttf
{
    extern size_t data_len;
    extern unsigned char data[];
}
```

SFML crea la fuente desde esos bytes. El módulo `draw_text.cpp` también inicializa FreeType directamente y ofrece `draw_text_to_texture`.

En el estado actual, el overlay visible usa `sf::Text`; `draw_text_to_texture` queda disponible como soporte adicional. No elimines `init_freetype()` o los archivos de fuente sin revisar el requisito de FreeType y el enlace definido por CMake.

---

## 18. CMake y dependencias

`CMakeLists.txt` construye un ejecutable llamado `prueba` con:

```text
main.cpp
palette.cpp
fractal.cpp
draw_text.cpp
```

Por tanto, si agregas un nuevo archivo `.cpp`, debes añadirlo a `add_executable`. Si solamente reemplazas la fórmula dentro de `fractal.cpp`, no necesitas modificar CMake.

Las dependencias actuales son:

- fmt
- SFML System
- SFML Graphics
- SFML Window
- FreeType
- Intel MPI

Una plantilla generada para el mismo equipo debe conservar esas dependencias y las configuraciones de `.vscode/settings.json`.

---

## 19. Elementos heredados que no forman parte del flujo principal

El código contiene elementos de prácticas anteriores:

- La variable compleja global `c`.
- El enum `runtime_type` con modos seriales, SIMD y OpenMP.
- La función vacía `dibujar_texto`.

El kernel Burning Ship actual no usa la variable `c`, porque cada píxel proporciona `cx` y `cy`. El enum tampoco decide el modo de ejecución actual. `dibujar_texto` no modifica la imagen.

Al generar una plantilla nueva, decide explícitamente entre:

1. Conservar estos elementos para mantener compatibilidad con material de clase.
2. Eliminarlos si el profesor no los exige y se desea una plantilla mínima.

No los uses accidentalmente como si fueran parte activa del algoritmo.

---

## 20. Procedimiento completo para generar otra plantilla

Sigue este orden:

### Paso 1: copia la estructura

Copia todos los archivos fuente y recursos, excluyendo `build/`.

### Paso 2: define la fórmula

Escribe en componentes reales:

```text
siguiente_zr = ...
siguiente_zi = ...
```

Define `z(0)`, constantes, escape o convergencia.

### Paso 3: define parámetros y dominio

Establece límites apropiados y el valor inicial de `max_iteraciones`. Añade parámetros adicionales a `main.cpp` si son controlables.

### Paso 4: sincroniza parámetros

Incluye todo parámetro modificable en `MPI_Bcast`. Actualiza tanto rank 0 como workers con el mismo tipo y cantidad.

### Paso 5: reemplaza la función por punto

Modifica solamente la inicialización, recurrencia y condición matemática. Conserva la devolución de iteración para el histograma.

### Paso 6: conserva el recorrido local

Mantén:

- Reinicio del histograma.
- Cálculo de pasos.
- Límite de `row_end`.
- Índice local `fila - row_start`.
- Conteo del histograma.

### Paso 7: actualiza nombres visibles

Cambia el título de la ventana, el texto del overlay y, si corresponde, el nombre de la función pública.

### Paso 8: compila

Configura CMake y construye el proyecto. Corrige primero errores de tipos, firmas o enlace.

### Paso 9: prueba con un proceso

Ejecuta con `mpiexec -n 1`. Verifica fórmula, dominio, color y cierre.

### Paso 10: prueba con cuatro procesos

Ejecuta con `mpiexec -n 4`. Verifica que:

- No aparezcan líneas horizontales entre bloques.
- La imagen coincida con la ejecución de un proceso.
- El histograma sea coherente.
- Escape cierre todos los ranks.
- `MPI_Finalize` se alcance sin bloqueo.

### Paso 11: prueba casos límite

- `max_iteraciones = 10`.
- Un valor alto de iteraciones.
- Dominio donde casi todos los puntos escapen.
- Dominio donde casi ningún punto escape.
- Un número de procesos que no divida exactamente `HEIGHT`.

---

## 21. Plantilla genérica del módulo fractal

Usa este esquema como punto de partida. Sustituye solamente los bloques marcados como específicos de la fórmula.

```cpp
#include "fractal.h"
#include "palette.h"

#include <algorithm>
#include <cstdint>

extern int max_iteraciones;

static uint32_t calcular_color_fractal(double cx, double cy, int& iteracion)
{
    // Estado inicial específico de la fórmula.
    double zr = 0.0;
    double zi = 0.0;
    iteracion = 0;

    while (iteracion < max_iteraciones &&
           (zr * zr + zi * zi) <= 4.0) {

        // Recurrencia específica de la fórmula.
        double siguiente_zr = zr * zr - zi * zi + cx;
        double siguiente_zi = 2.0 * zr * zi + cy;

        zr = siguiente_zr;
        zi = siguiente_zi;
        iteracion++;
    }

    if (iteracion >= max_iteraciones) {
        return 0xFF000000;
    }

    return color_ramp[iteracion % PALETTE_SIZE];
}

void calcular_fractal_mpi(double x_min, double y_min,
                          double x_max, double y_max,
                          uint32_t width, uint32_t height,
                          uint32_t row_start, uint32_t row_end,
                          uint32_t* pixel_buffer,
                          int* histograma_local)
{
    for (int bin = 0; bin < PALETTE_SIZE; bin++) {
        histograma_local[bin] = 0;
    }

    double paso_x = 0.0;
    double paso_y = 0.0;

    if (width > 1) {
        paso_x = (x_max - x_min) / (width - 1.0);
    }
    if (height > 1) {
        paso_y = (y_max - y_min) / (height - 1.0);
    }

    row_end = std::min(row_end, height);

    for (uint32_t fila = row_start; fila < row_end; fila++) {
        double cy = y_max - fila * paso_y;

        for (uint32_t columna = 0; columna < width; columna++) {
            double cx = x_min + columna * paso_x;
            int iteracion = 0;

            uint32_t color = calcular_color_fractal(cx, cy, iteracion);

            int fila_local = fila - row_start;
            int indice_local = fila_local * width + columna;
            pixel_buffer[indice_local] = color;

            if (iteracion < max_iteraciones) {
                int bin = iteracion * PALETTE_SIZE / max_iteraciones;
                if (bin >= PALETTE_SIZE) {
                    bin = PALETTE_SIZE - 1;
                }
                histograma_local[bin]++;
            }
        }
    }
}
```

---

## 22. Errores frecuentes y diagnóstico

### Pantalla completamente negra

Comprueba:

- Dominio incorrecto.
- Todos los puntos alcanzan `max_iteraciones`.
- Estado inicial equivocado.
- Constante Julia no transmitida.
- Condición de escape invertida.

### Pantalla de un solo color

Comprueba:

- Índice de paleta constante.
- `iteracion` no se incrementa.
- El dominio está demasiado lejos del conjunto.
- Los canales RGBA están en orden incorrecto.

### Figura reflejada verticalmente

Comprueba la fórmula de `cy`. Usa `y_max - fila * paso_y` para orientación cartesiana convencional.

### Líneas horizontales o bloques desplazados

Comprueba:

- `row_start` de cada rank.
- `filas_del_proceso` en `MPI_Recv`.
- Posición destino `src_start * WIDTH`.
- Índice local `(fila - row_start) * width + columna`.

### Bloqueo al cerrar

Comprueba:

- Todos los ranks reciben `running = 0`.
- No hay una colectiva extra en rank 0.
- Todos llaman `MPI_Bcast` y `MPI_Gather` en el mismo orden.
- Ningún worker abandona el ciclo antes de completar una colectiva que rank 0 todavía espera.

### Histograma demasiado grande

La suma de todos los bins debe ser menor o igual que:

```text
WIDTH * HEIGHT
```

Es menor cuando existen puntos que no escaparon. Si es mayor, probablemente el histograma no se reinicia en cada cuadro o se cuenta un píxel más de una vez.

### La imagen cambia al variar `nprocs`

La matemática debe ser independiente de la partición. Comprueba índices globales para `cy`, índices locales para el búfer y ensamblado en rank 0.

---

## 23. Lista de verificación de una plantilla nueva

### Estructura

- [ ] `main.cpp` conserva inicialización y finalización MPI.
- [ ] La fórmula está aislada en `fractal.cpp`.
- [ ] `fractal.h` coincide exactamente con la definición y la llamada.
- [ ] La paleta se define una sola vez.
- [ ] Los recursos de fuente permanecen disponibles.

### Matemática

- [ ] Está definido `z(0)`.
- [ ] Está claro si el píxel representa `c` o `z(0)`.
- [ ] Las componentes siguientes se calculan antes de actualizar el estado.
- [ ] La condición de escape o convergencia es correcta.
- [ ] El dominio corresponde al fractal.
- [ ] Los puntos interiores o no convergentes tienen un color definido.

### MPI

- [ ] Todos los ranks llaman las colectivas en el mismo orden.
- [ ] Rank 0 es la raíz de los `MPI_Bcast` y `MPI_Gather`.
- [ ] Todos los parámetros variables se transmiten.
- [ ] Las cantidades MPI están expresadas en elementos del tipo declarado.
- [ ] Los ranks con cero filas no acceden a datos inexistentes.
- [ ] El cierre se transmite exactamente una vez.

### Imagen

- [ ] El búfer local usa `fila - row_start`.
- [ ] Rank 0 copia cada bloque en su fila global.
- [ ] La textura usa `WIDTH * HEIGHT` píxeles.
- [ ] El orden RGBA es correcto.
- [ ] El overlay se dibuja después del sprite.

### Histograma

- [ ] Se reinicia en cada cuadro.
- [ ] Solo cuenta los puntos definidos por el requisito.
- [ ] Tiene exactamente `PALETTE_SIZE` bins.
- [ ] `MPI_Gather` recibe `nprocs * PALETTE_SIZE` enteros en rank 0.
- [ ] Rank 0 suma todos los histogramas.

### Memoria

- [ ] Cada `new[]` tiene su `delete[]`.
- [ ] `texture_buffer` se reserva y libera únicamente en rank 0.
- [ ] No se accede fuera de los límites.
- [ ] Se usa `nullptr` para punteros sin destino válido.

---

## 24. Especificación para solicitar la generación automática de otra plantilla

Para generar otro proyecto a partir de esta arquitectura, proporciona una solicitud con este formato:

```text
Genera una variante de este proyecto MPI + SFML para el fractal [NOMBRE].

Fórmula:
- Estado inicial: [DEFINICIÓN DE z(0)].
- Recurrencia: [FÓRMULA COMPLETA].
- El píxel representa: [c / z(0) / otro].
- Condición de escape o convergencia: [CONDICIÓN].
- Constantes adicionales: [LISTA].

Visualización:
- Dominio: [x_min, x_max] x [y_min, y_max].
- Máximo de iteraciones: [VALOR].
- Color de puntos interiores/no convergentes: [COLOR].
- Regla de paleta: [MÓDULO / NORMALIZADA / POR RAÍZ].

Infraestructura que debe conservarse:
- División MPI por filas.
- MPI_Bcast de parámetros y estado.
- MPI_Send/MPI_Recv de píxeles.
- Histograma local de 16 bins.
- MPI_Gather del histograma.
- Ventana y overlay SFML.
- Fuente embebida con FreeType.
- Convenciones descritas en contexto.txt.

No modificar:
- [LISTA DE ARCHIVOS PROTEGIDOS].
```

Si falta cualquiera de los datos matemáticos, la plantilla generada tendrá que asumirlo. Para obtener una reproducción exacta, especifica todos los puntos anteriores.

---

## 25. Resumen de la frontera de cambio

Para cambiar solamente la fórmula:

| Componente | ¿Cambiar? | Motivo |
|---|---:|---|
| Inicialización MPI | No | Es independiente del fractal. |
| División por filas | No | Es independiente de la fórmula. |
| Envío y recepción | No | Transporta píxeles de cualquier fractal. |
| `MPI_Gather` | Normalmente no | Reúne iteraciones de cualquier kernel de escape. |
| Función por punto | Sí | Contiene la recurrencia matemática. |
| Estado inicial | Sí | Depende de la familia fractal. |
| Criterio de escape | Posiblemente | Depende del fractal. |
| Dominio | Sí | Cada fractal tiene regiones de interés diferentes. |
| Parámetros difundidos | Posiblemente | Añade constantes requeridas por la fórmula. |
| Paleta | Opcional | Afecta apariencia, no geometría. |
| Texto del overlay | Sí | Debe identificar correctamente el fractal. |
| CMake | No, si no agregas archivos | Las dependencias permanecen iguales. |

La regla central es: **mantén estable la infraestructura paralela y gráfica; reemplaza de manera localizada la definición matemática y sus parámetros**.
