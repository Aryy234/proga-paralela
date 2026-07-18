# Guía reutilizable para crear proyectos con CUDA a partir de esta estructura

Esta guía resume los patrones que se repiten en los proyectos CUDA del repositorio, especialmente en:

- `07.cuda`
- `08.fractal-cuda`
- `Practice/practica-final-cuda`
- `Practice/practica-final-hibrida`

La idea no es memorizar un proyecto concreto, sino extraer el modelo mental que se repite:

1. La CPU organiza el flujo general.
2. La GPU ejecuta el cómputo intensivo.
3. El kernel hace una transformación por elemento, píxel, celda o muestra.
4. El host gestiona memoria, errores, tiempos y visualización.
5. La versión secuencial o la validación CPU sirven como referencia.

---

## 1. Estructura general que más se repite

La organización típica observada es:

```text
proyecto/
├── CMakeLists.txt
├── main.cpp              # flujo principal en CPU
├── kernel.cu             # kernels y wrappers CUDA
├── modulo.h / modulo.cuh  # declaraciones compartidas
├── palette.h / palette.cpp
├── recursos/             # imágenes, fuentes, datos
└── notas.txt / README.md # decisiones o instrucciones
```

### Separación habitual de responsabilidades

- `main.cpp`
  - inicializa la GPU
  - consulta propiedades del dispositivo
  - reserva memoria en CPU y GPU
  - copia datos
  - lanza kernels por medio de wrappers
  - sincroniza y valida
  - renderiza o imprime resultados

- `kernel.cu`
  - contiene `__global__` kernels
  - a veces incluye funciones `__device__`
  - expone wrappers `extern "C"` para el host

- `palette.cpp/.h`
  - almacena tablas de color
  - suele copiarse a memoria constante con `cudaMemcpyToSymbol`

- `CMakeLists.txt`
  - activa CUDA
  - enlaza dependencias como `fmt` o SFML
  - define la arquitectura CUDA cuando hace falta

---

## 2. Estilo y convenciones de programación

### Nombres

Los nombres suelen ser directos y descriptivos:

- `host_pixels`, `device_pixels`
- `h_A`, `d_A`
- `threads_per_block`
- `blocks_per_grid`
- `pixel_buffer`
- `max_iteraciones`

La convención más clara es:

- prefijo `h_` para memoria del host
- prefijo `d_` para memoria del device
- nombres funcionales para kernels y wrappers

### Tipos

Se repiten estos tipos:

- `uint32_t` para píxeles o buffers RGBA
- `float` cuando importa rendimiento y el problema lo permite
- `double` cuando se quiere más precisión
- `size_t` para tamaños y bytes
- `int` para índices, bloques, hilos y parámetros enteros

### Diferenciación de funciones

Hay tres niveles frecuentes:

1. Función de host que prepara y lanza.
2. Kernel `__global__` que ejecuta la GPU.
3. Función `__device__` o `__host__ __device__` para lógica compartida.

### Convenciones visuales

- comentarios cortos y explicativos
- bloques marcados como:
  - inicialización
  - copias
  - lanzamiento
  - sincronización
  - liberación
- uso frecuente de `fmt::println` para depuración e información del dispositivo

---

## 3. Modelo mental de paralelización CUDA

La idea dominante es el patrón **map**:

- un hilo procesa un elemento
- o un píxel
- o una celda
- o una posición del dominio

Este patrón aparece claramente en:

- suma de vectores
- fractales
- filtros de imagen
- histogramas parciales
- evaluaciones por píxel

### Regla principal

Si cada elemento puede calcularse de forma independiente, CUDA encaja muy bien.

Si hay dependencias complejas entre elementos, la solución suele requerir:

- varios kernels
- memoria intermedia
- reducción parcial
- sincronización entre fases

---

## 4. Estructura host/device recomendada

### Flujo típico en CPU

1. Seleccionar dispositivo con `cudaSetDevice`.
2. Consultar propiedades con `cudaGetDeviceProperties`.
3. Reservar memoria en host.
4. Reservar memoria en device con `cudaMalloc`.
5. Copiar entradas con `cudaMemcpy`.
6. Lanzar kernel.
7. Sincronizar o revisar errores.
8. Copiar resultados de vuelta.
9. Liberar recursos.

### Flujo típico en GPU

1. Calcular índices globales.
2. Validar límites.
3. Leer datos de entrada.
4. Calcular resultado local.
5. Escribir en la salida.

---

## 5. Cálculo de índices y distribución del trabajo

### Patrón básico 1D

En los ejemplos del repo domina esta forma:

```cpp
int index = blockIdx.x * blockDim.x + threadIdx.x;
if (index < n) {
    // trabajo
}
```

### Cálculo del número de bloques

Se usa la división redondeada hacia arriba:

```cpp
int blocks = (n + threads_per_block - 1) / threads_per_block;
```

### Patrón multidimensional

Para imágenes y matrices, el índice suele transformarse así:

- fila = `index / width`
- columna = `index % width`

o bien usando dos dimensiones de bloque y malla cuando el problema lo justifica.

### Tamaño del bloque

Los proyectos del repositorio usan mucho `256` o `1024` hilos por bloque, pero la elección correcta depende de:

- ocupación
- presión de registros
- memoria compartida
- arquitectura de la GPU
- tamaño real del problema

No hay un valor universal.

---

## 6. Diseño de kernels

### Kernel simple

El patrón más frecuente es:

```cpp
__global__ void kernel(...){
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) {
        ...
    }
}
```

### Kernel con función auxiliar

Cuando la lógica es más compleja, se separa el cálculo en:

- una función `__device__` que evalúa un elemento
- un kernel que recorre el dominio

Eso mejora legibilidad y permite reutilizar la lógica.

### Reglas prácticas

- cada hilo debe hacer una cantidad razonable de trabajo
- evitar kernels gigantes con demasiada lógica
- minimizar bifurcaciones internas
- chequear límites siempre
- no asumir que el tamaño de entrada coincide con la malla

### Bifurcaciones y divergencia

Los ejemplos muestran una tendencia clara:

- se permite una validación mínima de límites
- se intenta mantener la lógica principal simple
- se evita meter ramas innecesarias dentro del kernel

---

## 7. Gestión de memoria

### Memoria del host

Se usan reservaciones como:

- `new[]`
- `malloc`

### Memoria del device

Se usa:

- `cudaMalloc`
- `cudaFree`

### Transferencias

Se emplea:

- `cudaMemcpyHostToDevice`
- `cudaMemcpyDeviceToHost`
- en algunos casos `cudaMemcpyToSymbol`

### Reglas que se repiten

- no copiar más de lo necesario
- no reservar dentro de bucles
- liberar todo al final
- usar tamaños en bytes correctamente
- sincronizar antes de leer resultados asíncronos

### Memoria constante

En proyectos de fractales aparece un patrón útil:

- la paleta se copia una vez a `__constant__`
- el kernel la consulta muchas veces

Esto es apropiado cuando:

- los datos son pequeños
- todos los hilos leen la misma tabla
- la tabla no cambia durante el cómputo

### Memoria unificada o pinned

No es la convención dominante en estos ejemplos, pero sí conviene recordar:

- pinned memory ayuda con transferencias asíncronas
- unified memory simplifica el código, pero no siempre da el mejor rendimiento

---

## 8. Gestión de errores

En los proyectos más completos aparece un patrón de macro `CHECK(...)`:

```cpp
#define CHECK(expr) { \
    auto err = (expr); \
    if (err != cudaSuccess) { \
        ... \
    } \
}
```

### Qué conviene comprobar

- `cudaSetDevice`
- `cudaMalloc`
- `cudaMemcpy`
- `cudaGetLastError`
- `cudaDeviceSynchronize`

### Idea práctica

No basta con lanzar el kernel:

- hay que revisar errores de lanzamiento
- hay que revisar errores asíncronos
- hay que sincronizar cuando se valida el resultado

---

## 9. Sincronización

### Qué se usa más

- `cudaDeviceSynchronize`
- `cudaGetLastError`
- sincronización implícita al copiar de vuelta

### Cuándo hace falta

- antes de medir tiempo real de ejecución
- antes de leer datos producidos por el kernel
- cuando se depura

### Cuándo evitarla

- no conviene poner `cudaDeviceSynchronize` después de cada pequeña operación si no es necesario
- tampoco conviene bloquear más de lo debido si luego se puede usar otro mecanismo de espera o eventos

---

## 10. Organización del código por fases

La estructura más útil para un nuevo proyecto CUDA es:

1. **Entrada y configuración**
   - leer parámetros
   - detectar dispositivo
   - mostrar propiedades

2. **Preparación**
   - generar datos
   - reservar memoria
   - copiar datos al device

3. **Ejecución**
   - lanzar uno o varios kernels
   - sincronizar si hace falta

4. **Validación**
   - copiar resultados
   - comparar con referencia CPU

5. **Salida**
   - mostrar métricas
   - renderizar
   - liberar recursos

---

## 11. Patrones de proyectos del repositorio

### 11.1 Suma de vectores

Patrón:

- un hilo por elemento
- acceso lineal
- kernel pequeño
- ideal como plantilla base

Lección:

- es el caso más limpio para empezar
- sirve para validar instalación, compilación y flujo host/device

### 11.2 Fractales

Patrón:

- un hilo por píxel
- cálculo iterativo independiente
- uso de paleta en memoria constante
- visualización con SFML

Lección:

- funciona muy bien con la GPU porque el paralelismo es masivo
- la salida gráfica se mantiene en CPU

### 11.3 Burning Ship / Newton y variantes

Patrón:

- cálculo por píxel
- lógica matemática más compleja
- posible acumulación de métricas o histogramas
- combinación de kernel principal con auxiliares

Lección:

- CUDA permite separar cálculo y visualización
- las funciones auxiliares `__device__` mejoran legibilidad

### 11.4 Proyecto híbrido

Patrón:

- la CPU sigue controlando la UI y el flujo
- la GPU hace la parte intensiva
- la lógica se reparte en fases

Lección:

- CUDA no reemplaza siempre toda la aplicación
- suele integrarse como backend de cómputo

---

## 12. Compilación y configuración

### CMake

La forma más común es:

```cmake
enable_language(CXX CUDA)
add_executable(app main.cpp kernel.cu)
```

### Dependencias

Se enlazan bibliotecas como:

- `fmt`
- SFML

### Recomendaciones

- definir la arquitectura CUDA cuando sea posible
- compilar en `Release` para medir
- mantener `Debug` para depuración
- usar el compilador CUDA compatible con el host

### Parámetros importantes

- arquitectura de GPU
- versión de CUDA
- soporte de `sm_xx`
- flags de optimización y depuración

---

## 13. Medición de rendimiento

### Qué medir

- tiempo total
- tiempo del kernel
- tiempo de transferencia
- tiempo de inicialización
- tiempo de render, si existe

### Qué no mezclar

- no mezclar UI con tiempo de cómputo si se quiere medir GPU pura
- no comparar contra una CPU sin optimización

### Instrumentación recomendada

- temporizadores de CPU para el flujo general
- eventos CUDA para el tiempo del kernel
- múltiples corridas para evitar ruido

### Interpretación correcta

CUDA produce ventaja real cuando:

- el tamaño del problema es suficientemente grande
- hay mucho paralelismo
- las transferencias no dominan el costo
- el kernel no depende de demasiada sincronización

---

## 14. Validación y corrección

### Patrón de validación

1. Crear una referencia secuencial.
2. Ejecutar una versión CUDA sobre los mismos datos.
3. Comparar con tolerancia si hay punto flotante.
4. Probar tamaños pequeños y grandes.
5. Probar casos límite.

### Qué revisar

- accesos fuera de rango
- tamaño de copia correcto
- índices correctos
- sincronización antes de comparar
- errores de lanzamiento

### Importante en punto flotante

En muchas transformaciones numéricas:

- GPU y CPU pueden diferir levemente
- el orden de operaciones cambia
- la comparación debe permitir pequeñas diferencias

---

## 15. Errores típicos a evitar

- lanzar muchos kernels diminutos
- copiar datos innecesariamente entre host y device
- olvidar liberar memoria de GPU
- usar memoria compartida o atómica sin necesidad real
- no revisar errores de CUDA
- suponer que la GPU mejora todo automáticamente
- ignorar la divergencia de warps
- usar índices incorrectos para imágenes o matrices
- no validar tamaños no múltiplos del bloque

---

## 16. Cuándo CUDA sí suele ser buena idea

CUDA encaja especialmente bien cuando el problema:

- tiene paralelismo masivo
- es regular
- trabaja sobre arreglos, matrices, imágenes o volúmenes
- puede mantenerse en GPU durante varias fases
- reutiliza datos en el dispositivo
- tolera algo de complejidad de implementación a cambio de rendimiento

---

## 17. Cuándo CUDA puede no compensar

CUDA puede no ser la mejor opción si:

- el problema es muy pequeño
- hay poca carga por elemento
- las transferencias dominan
- la lógica es muy irregular
- hay dependencias globales frecuentes
- el código sería mucho más complejo sin una ganancia clara

---

## 18. Plantilla mental para un nuevo problema CUDA

Cuando aparezca un problema nuevo, la secuencia de diseño debería ser esta:

### Paso 1: entender el problema

- qué se calcula
- qué se transforma
- qué depende de qué
- cuál es la entrada y la salida

### Paso 2: construir una referencia secuencial

- primero simple
- primero correcta
- primero verificable

### Paso 3: decidir la estrategia CUDA

- un hilo por elemento
- un hilo por píxel
- un hilo por celda
- un hilo por tarea
- varios kernels por fases si hace falta

### Paso 4: diseñar la memoria

- qué vive en host
- qué vive en device
- qué se copia una vez
- qué se reutiliza
- qué conviene poner en constante o compartida

### Paso 5: diseñar el kernel

- calcular índices
- validar límites
- minimizar ramas
- controlar accesos a memoria
- evitar writes en conflicto

### Paso 6: lanzar y validar

- elegir bloques y malla
- comprobar errores
- sincronizar cuando haga falta
- comparar con referencia

### Paso 7: medir

- tiempo total
- tiempo de kernel
- tiempo de transferencia
- speedup real

---

## 19. Modelo mental final

La idea más importante que dejan estos proyectos es esta:

> CUDA no consiste en "pasar el código a la GPU", sino en reorganizar el problema para que la GPU haga bien aquello que es masivamente paralelo, con memoria bien gestionada, índices correctos, validación clara y coste de transferencia justificado.

En la práctica, el diseño correcto suele seguir este orden:

- primero versión CPU
- luego kernel simple
- después memoria y errores bien controlados
- luego optimización
- y al final medición real

Ese orden evita construir soluciones complicadas que no escalan o que no se pueden validar.

