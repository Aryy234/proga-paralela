# Guía reutilizable para crear proyectos con OpenMP a partir de esta estructura

---

## 1. Idea central

La estructura que más se repite en estos proyectos es la siguiente:

1. Primero se construye una versión secuencial correcta.
2. Luego se separa el problema en módulos pequeños.
3. Después se crea una versión OpenMP usando la misma interfaz.
4. Finalmente se compara la salida, el tiempo y la escalabilidad.

Eso permite que la versión paralela no cambie la lógica del problema, solo la forma de distribuir el trabajo.

---

## 2. Estructura de proyecto recomendada

La organización típica observada en el repositorio es:

```text
proyecto/
├── CMakeLists.txt
├── main.cpp
├── modulo_serial.h/.cpp
├── modulo_openmp.h/.cpp
├── modulo_simd.h/.cpp        (si aplica)
├── palette.h/.cpp             (si hay visualización)
├── args.h/.cpp                (si hay parámetros de ejecución)
├── notas.txt / PLAN.md        (diseño y decisiones)
├── recursos/                  (fuentes, imágenes, datos)
└── build/                     (generado)
```

### Patrones de organización detectados

- `main.cpp` suele contener:
  - lectura de argumentos
  - selección de modo
  - bucle principal
  - renderizado o impresión de resultados
- Los módulos de cómputo se separan por estrategia:
  - `*_serial`
  - `*_openmp`
  - `*_simd`
- Las funciones suelen tener firmas equivalentes para facilitar comparación.
- Si el proyecto tiene interfaz gráfica, el render se mantiene fuera del núcleo de cálculo.

### Regla práctica

Si el problema tiene varias implementaciones, todas deben compartir:

- mismos parámetros de entrada
- misma salida
- misma lógica matemática base
- distinta estrategia de ejecución

---

## 3. Convenciones de código observadas

### Nombres

Se usan nombres descriptivos y directos:

- `heat_serial`
- `heat_openmp`
- `producto_escalar_regiones`
- `filtro_escala_grises_simd`
- `julia_openmp_for_simd`

La convención dominante es clara: el nombre del archivo o función deja ver el algoritmo y el backend.

### Estilo

- Código simple y legible.
- Variables cortas cuando el contexto es obvio, pero no en exceso.
- Separación clara entre:
  - datos de entrada
  - variables temporales
  - acumuladores
  - parámetros globales de ejecución
- Uso frecuente de comentarios explicativos, sobre todo cuando hay paralelismo.

### Tipos y datos

Se observan estos patrones:

- `int` para índices y dimensiones pequeñas.
- `uint32_t` para buffers de píxeles.
- `double` cuando importa precisión numérica.
- `float` cuando el problema visual o vectorial lo permite.
- `std::vector<T>` cuando se prefiere gestión automática de memoria.
- punteros crudos cuando se busca control directo del buffer.

### Variables compartidas y privadas

La convención útil es esta:

- lo que describe el problema suele ser compartido
- lo que cambia por hilo debe ser privado
- los acumuladores por hilo deben ser locales
- la combinación final debe hacerse con reducción, atómico o acumulación controlada

En varios proyectos aparece la idea de declarar variables auxiliares dentro de la región paralela para hacer explícita su privacidad.

---

## 4. Lógica de programación que más se repite

### Patrón general

1. Generar o leer datos.
2. Recorrer el dominio con uno o más bucles.
3. Aplicar una transformación independiente por elemento, fila o bloque.
4. Guardar el resultado.
5. Repetir hasta terminar o cumplir un criterio.

### Qué tipo de problemas se paralelizan mejor

Los proyectos del repositorio muestran que OpenMP encaja especialmente bien cuando:

- cada iteración del bucle hace trabajo independiente
- el acceso a memoria es regular
- el cálculo por elemento no depende del resultado de otro elemento del mismo paso
- el problema tiene tamaño suficiente para amortizar el costo de crear y sincronizar hilos

### Qué se deja secuencial

Normalmente se conserva secuencial:

- inicialización
- lectura de parámetros
- validaciones
- renderizado o salida final
- comparación de resultados
- medición de tiempo fuera del trabajo real

---

## 5. Formas de paralelización con OpenMP que aparecen en el repositorio

### 5.1 Regiones paralelas manuales

Se usa este patrón cuando se quiere controlar explícitamente el reparto:

```cpp
#pragma omp parallel
{
    int tid = omp_get_thread_num();
    int nthreads = omp_get_num_threads();
    // repartir filas, bloques o rango de trabajo
}
```

Este enfoque aparece en ejemplos como el producto escalar y en versiones paralelas de fractales y calor 2D.

### 5.2 `parallel for`

Es la forma más directa para paralelizar bucles independientes:

```cpp
#pragma omp parallel for reduction(+:sum)
for (int i = 0; i < n; ++i) {
    sum += data[i];
}
```

Ventajas:

- menos código
- más fácil de leer
- ideal cuando el reparto estándar por iteraciones es suficiente

### 5.3 Reducciones

Se usan cuando varios hilos contribuyen a un mismo resultado:

- suma
- máximo
- mínimo
- acumulador de error
- residuo L2

Patrón:

```cpp
#pragma omp parallel for reduction(+:sum)
```

### 5.4 Sincronización puntual

Se observan mecanismos como:

- `atomic`
- `critical`
- barreras implícitas
- uso cuidadoso de `nowait`

La tendencia general es evitar sincronización innecesaria y usarla solo cuando realmente hace falta.

### 5.5 SIMD combinado con OpenMP

Cuando el problema lo permite, se combina paralelismo por hilos con vectorización:

```cpp
#pragma omp parallel for simd
```

o se implementa una versión separada con intrínsecos.

La lección práctica es:

- OpenMP divide el trabajo entre hilos
- SIMD acelera el trabajo dentro de cada hilo

Son niveles distintos de paralelismo y pueden complementarse.

---

## 6. Distribución del trabajo

### Estrategias observadas

Los proyectos usan principalmente tres formas de repartir trabajo:

1. **Por bloques manuales**
   - cada hilo recibe un rango contiguo
   - útil para filas, columnas o segmentos
2. **Por iteración con `parallel for`**
   - el compilador/OpenMP distribuye el bucle
3. **Por reducción de acumuladores**
   - cada hilo calcula una parte y luego se combina

### Criterio para elegir

- Si el problema es muy regular, `parallel for` suele ser suficiente.
- Si se necesita control fino sobre fronteras, halos o visualización, conviene reparto manual.
- Si hay fuerte dependencia por filas o bloques, el reparto manual ayuda a controlar el acceso a memoria.

### Reparto recomendado

Para matrices e imágenes:

- paralelizar por filas suele ser la opción más natural
- trabajar con filas contiguas mejora localidad de memoria
- evita fragmentar demasiado el trabajo

---

## 7. Gestión de variables en OpenMP

### Compartidas

Normalmente se comparten:

- buffers de entrada
- buffers de salida
- dimensiones
- parámetros constantes
- tablas de color

### Privadas

Conviene hacer privadas:

- índices de bucle locales
- identificadores de hilo
- acumuladores parciales
- variables temporales del cálculo

### `firstprivate`

Útil cuando cada hilo necesita una copia inicial de un valor común.

### `lastprivate`

Sirve cuando interesa conservar el valor final de la última iteración útil.

### `default(none)`

Aunque no aparece de forma uniforme en todos los proyectos, es una buena práctica para obligar a declarar explícitamente el ámbito de cada variable. Es especialmente recomendable en código nuevo.

---

## 8. Gestión de memoria y acceso concurrente

### Patrones útiles

- leer desde un buffer
- escribir en otro buffer
- intercambiar punteros al final

Ese esquema evita condiciones de carrera en algoritmos tipo Jacobi.

### Reglas prácticas

- no escribir varios hilos en la misma posición sin protección
- preferir acumuladores locales
- combinar resultados al final
- evitar estructuras compartidas que se modifiquen de forma intensa

### False sharing

Aunque no siempre se menciona explícitamente, el riesgo existe cuando varios hilos escriben elementos cercanos de memoria.

Para reducirlo:

- asignar rangos contiguos por hilo
- evitar escribir en arrays compartidos muy pequeños
- usar acumuladores locales y combinar después

### Localidad de memoria

Se favorece cuando:

- se recorren matrices por filas
- se accede de forma lineal al buffer
- se minimizan saltos irregulares

---

## 9. Compilación y configuración

### Sistema de construcción

El repositorio usa principalmente `CMake`.

### Configuración típica

- `CMAKE_CXX_STANDARD`
- flags de OpenMP como `-fopenmp`
- flags de vectorización como `-mavx2`
- `find_package(...)` para dependencias como SFML, fmt o MPI según el proyecto

### Recomendaciones prácticas

- compilar en `Release` para medir rendimiento
- usar `Debug` solo para depurar
- no fijar el número de hilos dentro del código si el proyecto debe ser configurable por entorno
- dejar la configuración en CMake y en variables de entorno cuando sea posible

### Variables de entorno frecuentes

- `OMP_NUM_THREADS`
- `OMP_SCHEDULE`
- `OMP_PROC_BIND`
- `OMP_PLACES`

---

## 10. Pruebas y validación

### Secuencia recomendada

1. Implementar la versión secuencial.
2. Validar con casos pequeños.
3. Medir tiempos base.
4. Implementar OpenMP.
5. Comparar resultados con la versión secuencial.
6. Probar varios números de hilos.
7. Medir speedup y eficiencia.

### Qué verificar siempre

- mismo resultado que la versión secuencial
- ausencia de condiciones de carrera
- estabilidad numérica razonable
- mejora real de tiempo
- comportamiento correcto con distintos tamaños de entrada

### Cómo medir bien

- medir solo el cómputo, no la UI
- repetir varias veces
- usar compilación optimizada
- comparar contra una línea base

### Errores comunes que hay que evitar

- paralelizar bucles demasiado pequeños
- usar regiones críticas de más
- crear regiones paralelas repetidamente dentro de bucles cortos
- ignorar dependencias entre iteraciones
- olvidar que el costo de sincronización puede superar la ganancia

---

## 11. Plantilla mental para un nuevo problema

Cuando aparezca un nuevo problema, la secuencia de diseño debería ser esta:

### Paso 1: entender el problema

- qué entra
- qué sale
- qué parte es costosa
- qué depende de qué

### Paso 2: construir la versión secuencial

- primero simple
- primero correcta
- primero verificable

### Paso 3: decidir el tipo de paralelismo

- por datos
- por bloques
- por reducción
- por tareas, solo si realmente hay irregularidad o dependencias asimétricas

### Paso 4: elegir la estructura del proyecto

- `main.cpp`
- módulo secuencial
- módulo OpenMP
- archivos de soporte
- CMake

### Paso 5: diseñar la versión OpenMP

- identificar variables compartidas y privadas
- minimizar sincronización
- escoger `parallel for`, `parallel`, `reduction`, `atomic` o `critical` según el caso

### Paso 6: validar

- comparar con secuencial
- probar diferentes tamaños
- probar diferentes hilos
- medir rendimiento

---

## 12. Patrones concretos aprendidos de los proyectos del repo

### 12.1 Fractales

Patrón general:

- calcular una imagen píxel por píxel
- cada píxel es independiente
- la paralelización por filas funciona muy bien
- la visualización se mantiene separada del cálculo

Esto es ideal para:

- `parallel for`
- SIMD
- reparto manual por regiones

### 12.2 Producto escalar

Patrón general:

- acumulación de una suma total
- cada elemento es independiente
- reducción es la forma natural de paralelizar

Esto favorece:

- `reduction(+:sum)`
- acumuladores locales

### 12.3 Filtros de imagen

Patrón general:

- cada píxel se transforma de forma independiente
- el acceso a memoria es lineal
- la vectorización SIMD es muy natural

Esto favorece:

- paralelización por píxel o por bloque
- combinación OpenMP + SIMD

### 12.4 Ecuación de calor 2D

Patrón general:

- cálculo por celdas interiores
- lectura de un buffer y escritura en otro
- conservación de bordes
- reducción para residuos o métricas

Esto favorece:

- filas contiguas por hilo
- dos buffers
- reducción del residuo
- verificación contra la solución secuencial

---

## 13. Buenas prácticas que conviene conservar

- escribir primero la versión secuencial
- usar nombres claros
- mantener la misma interfaz entre backends
- comparar contra una referencia
- medir en `Release`
- documentar el reparto de trabajo
- limitar la sincronización
- evitar copias innecesarias
- revisar la localidad de memoria
- justificar cada decisión de paralelización

---

## 14. Prácticas que conviene evitar

- paralelizar por inercia
- usar `critical` para todo
- mezclar UI y cómputo sin necesidad
- modificar buffers compartidos sin protección
- fijar hilos desde el código sin motivo
- medir tiempos incluyendo render o carga de archivos
- asumir que más hilos siempre significa más velocidad
- copiar el mismo patrón sin revisar dependencias reales

---

## 15. Modelo mental final

La idea más importante que dejan estos proyectos es esta:

> OpenMP no se usa para "paralelizar todo", sino para paralelizar lo que es realmente independiente, con el menor costo de sincronización posible y manteniendo una versión secuencial de referencia.

Si el nuevo problema es regular, por datos, con bucles independientes, OpenMP suele ser una muy buena opción.  
Si el problema tiene dependencias complejas, la decisión correcta puede ser:

- dejar una parte secuencial
- usar reducción
- dividir por bloques
- o incluso no paralelizar una sección concreta

Contexto de estilo y convenciones de código (usar en todas las respuestas):

- Idioma: escribe todo en español (comentarios, mensajes, explicaciones breves).
- Estilo general: imperativo y directo. Código claro y didáctico, pensado para estudiantes/práctica.
- Nombres: variables y funciones en español, descriptivos y completos (por ejemplo: filas_local, sendcounts, displs, cantidad_local, scatter_recvbuf). Evitar acrónimos confusos.
- Tipos y contenedores: uso preferente de arreglos C explícitos (`int*` con `new[]/delete[]`) para ejemplos sencillos; si se usa STL, hacerlo sólo cuando aporte claridad.
- Bucles: usar `for` explícitos con índices (`for (int i = 0; i < n; i++)`), evitar programación funcional (lambdas, std::transform) en ejemplos didácticos.

Esa decisión debe salir del análisis del problema, no de una plantilla rígida.

