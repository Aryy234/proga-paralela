# Guía de referencia para proyectos MPI del repositorio

Esta guía resume los patrones reales que aparecen en los proyectos MPI de este repositorio y los convierte en una plantilla reutilizable para nuevos ejercicios.

La idea central que se repite es esta:

- `rank 0` prepara, coordina y, cuando hace falta, recopila resultados.
- Los demás procesos ejecutan la misma lógica base, pero sobre una porción de trabajo propia.
- La parte paralela se concentra en distribuir datos, calcular localmente y reunir resultados.
- La parte secuencial se mantiene simple para validar el algoritmo y controlar la ejecución.

No se trata de copiar código tal cual, sino de conservar la misma forma de pensar el problema.

---

## 1. Patrones comunes detectados

### 1.1 Estructura general

En los proyectos revisados aparecen dos estilos principales:

1. Proyecto simple de un solo archivo:
   - `main.cpp`
   - `CMakeLists.txt`
   - `README.md`

2. Proyecto modular:
   - `main.cpp`
   - módulos de lógica como `fractal.cpp`, `palette.cpp`, `draw_text.cpp`
   - headers correspondientes
   - `CMakeLists.txt`
   - documentación de apoyo en `context/`

La separación más clara y reutilizable es esta:

- `main.cpp`: arranque MPI, distribución del trabajo, comunicación, ensamblado y salida.
- `*.cpp` de dominio: cálculo real del problema.
- `*.h`: contratos y constantes compartidas.
- `README.md`: compilación, ejecución y ejemplo de uso.

### 1.2 Estilo de programación

Se repiten estas convenciones:

- Variables con nombres descriptivos en español.
- Uso frecuente de `rank`, `nprocs`, `root`, `row_start`, `row_end`, `delta`, `running`.
- Cálculo explícito de tamaños locales.
- Comentarios orientados a explicar intención.
- Uso de `const` o `static` para valores que no deben cambiar.
- Memoria dinámica con `new[]` y `delete[]` cuando el tamaño depende de MPI o del tamaño del problema.

### 1.3 Lógica paralela

Los ejemplos siguen casi siempre este patrón:

1. `MPI_Init`.
2. `MPI_Comm_rank` y `MPI_Comm_size`.
3. `rank 0` lee argumentos o genera datos.
4. `MPI_Bcast` para repartir parámetros comunes.
5. División del trabajo por proceso.
6. Cálculo local independiente.
7. `MPI_Send` / `MPI_Recv` o `MPI_Gather` / `MPI_Reduce`.
8. `rank 0` imprime o presenta resultados.
9. `MPI_Finalize`.

---

## 2. Estructura recomendada para nuevos proyectos

### 2.1 Estructura mínima

```text
proyecto/
├── CMakeLists.txt
├── README.md
├── main.cpp
└── build/
```

### 2.2 Estructura modular recomendada

```text
proyecto/
├── CMakeLists.txt
├── README.md
├── main.cpp
├── algoritmo.h
├── algoritmo.cpp
├── datos.h
├── datos.cpp
├── utilidades.h
├── utilidades.cpp
└── context/
    ├── enunciado.md
    └── notas.md
```

### 2.3 Responsabilidad de cada archivo

- `main.cpp`: controla el ciclo MPI y el flujo general.
- `algoritmo.cpp`: contiene el núcleo computacional.
- `datos.cpp`: genera, valida o transforma datos.
- `utilidades.cpp`: funciones auxiliares de impresión, tiempos o validaciones.
- `*.h`: declaraciones compartidas.
- `README.md`: instrucciones para compilar y ejecutar.

Si el proyecto es muy pequeño, puedes dejar todo en `main.cpp`, pero la regla de oro es esta: si una función empieza a mezclar cálculo, comunicación y presentación, conviene separarla.

---

## 3. Convenciones de nombres

### 3.1 Ranks y procesos

Usar siempre nombres consistentes:

- `rank`: identificador del proceso actual.
- `nprocs`: número total de procesos.
- `root`: proceso raíz, normalmente `0`.
- `source` y `destination`: origen y destino en comunicaciones punto a punto.
- `tag`: etiqueta del mensaje.
- `comm` o `communicator`: comunicador usado.

### 3.2 Tamaños y posiciones

Reutilizar estas ideas:

- `global_size`: tamaño total del problema.
- `local_size`: tamaño asignado a un proceso.
- `row_start` / `row_end`: límites de filas.
- `col_start` / `col_end`: límites de columnas.
- `offsets`: desplazamientos.
- `counts`: cantidades por proceso.
- `remaining` o `resto`: elementos sobrantes.

### 3.3 Buffers y comunicaciones

Convenciones útiles:

- `send_buffer`
- `recv_buffer`
- `local_buffer`
- `global_buffer`
- `requests`
- `statuses`

Cuando el proyecto crece, esta coherencia evita errores al mezclar comunicación y cálculo.

---

## 4. Patrón base de ejecución MPI

### 4.1 Secuencia estándar

```cpp
MPI_Init(&argc, &argv);

MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

// Trabajo principal

MPI_Finalize();
```

### 4.2 Regla práctica

- Todo lo que cambie de un proceso a otro debe derivarse de `rank`.
- Todo lo que deba ser igual en todos los procesos debe difundirse con `MPI_Bcast` o calcularse determinísticamente.
- Toda operación colectiva debe ser llamada por todos los procesos del comunicador, en el mismo orden.

### 4.3 Root como coordinador

`rank 0` suele hacer estas tareas:

- leer argumentos
- generar datos iniciales
- difundir parámetros
- recopilar resultados
- imprimir o dibujar la salida final

Los demás procesos:

- reciben parámetros
- calculan su parte local
- envían o reducen sus resultados

---

## 5. División del trabajo

### 5.1 Reglas generales

La descomposición observada suele ser una de estas:

- por bloques
- por filas
- por rango de elementos
- por porciones iguales con resto repartido

La lógica más común es:

```cpp
int base = global_size / nprocs;
int resto = global_size % nprocs;

local_size = base;
if (rank < resto) {
    local_size++;
}
```

### 5.2 Cálculo de desplazamientos

Cuando cada proceso necesita saber dónde empieza su bloque:

```cpp
offset = rank * base + std::min(rank, resto);
```

Esto evita perder elementos cuando `global_size` no es divisible por `nprocs`.

### 5.3 Casos especiales

Si el número de procesos supera el tamaño del problema:

- algunos procesos pueden recibir `0`
- no deben acceder a posiciones inválidas
- las comunicaciones con tamaño `0` son válidas si el protocolo se respeta

---

## 6. Separación entre cálculo y comunicación

### 6.1 Regla que se repite

Primero se calcula localmente, después se comunica.

En los proyectos analizados, el orden habitual es:

1. preparar datos locales
2. ejecutar el kernel de cálculo
3. enviar o reducir el resultado parcial
4. ensamblar o imprimir en `rank 0`

### 6.2 Qué no mezclar

Evitar estas combinaciones dentro de la misma función si el proyecto ya es medianamente grande:

- cálculo matemático + envío MPI
- generación de datos + interfaz
- ensamblado final + lógica algorítmica

La separación facilita depuración y permite cambiar la estrategia MPI sin reescribir el algoritmo.

---

## 7. Distribución de datos

### 7.1 Estrategias vistas

Los proyectos usan sobre todo:

- `MPI_Bcast` para parámetros comunes
- `MPI_Scatter` para bloques iguales
- `MPI_Scatterv` para bloques desiguales
- `MPI_Gather` para juntar resultados
- `MPI_Reduce` para agregar valores parciales

### 7.2 Cuándo usar cada una

- `MPI_Bcast`: cuando todos necesitan el mismo dato.
- `MPI_Scatter`: cuando cada proceso recibe exactamente la misma cantidad.
- `MPI_Scatterv`: cuando hay restos o tamaños irregulares.
- `MPI_Gather`: cuando cada proceso devuelve un bloque.
- `MPI_Reduce`: cuando cada proceso produce una contribución escalar o agregable.

### 7.3 Regla práctica

Si la operación final es una suma, mínimo, máximo o acumulación similar, `MPI_Reduce` suele ser la opción más limpia.

Si el resultado final es una estructura completa repartida por partes, `MPI_Gather` suele ser mejor.

---

## 8. Comunicación punto a punto

### 8.1 Patrón observado

Cuando no se usa una colectiva, el patrón es normalmente:

- `rank 0` recibe bloques de los demás con `MPI_Recv`
- los demás envían con `MPI_Send`

### 8.2 Uso correcto

Mantener consistencia en:

- origen
- destino
- tipo MPI
- cantidad de elementos
- tag

### 8.3 Reglas de seguridad

- No recibir con un tamaño menor al enviado.
- No reutilizar el buffer de envío hasta que la operación haya terminado.
- No mezclar tags sin documentarlos.
- No usar `MPI_ANY_SOURCE` si el orden de mensajes importa y no está controlado.

---

## 9. Operaciones colectivas

### 9.1 Las más usadas en el repositorio

- `MPI_Bcast`
- `MPI_Scatter`
- `MPI_Scatterv`
- `MPI_Gather`
- `MPI_Reduce`
- `MPI_Barrier` solo cuando realmente aporta algo

### 9.2 Buen criterio

Las colectivas deben ser:

- pocas
- claras
- consistentes
- justificadas por la estructura del problema

No conviene poner barreras solo para “estar seguros”; muchas veces ocultan un diseño poco limpio.

---

## 10. Modelo de datos

### 10.1 Datos globales, locales y replicados

Clasificación útil:

- globales conceptuales: tamaño total, dominio completo, parámetros del problema
- locales: bloque de filas, subarreglo, fragmento de imagen, conteo parcial
- replicados: constantes, límites, configuración común

### 10.2 Regla práctica

Todo dato que pueda reconstruirse localmente no debe comunicarse innecesariamente.

Todo dato que cambie durante la ejecución debe sincronizarse explícitamente.

### 10.3 Representación de matrices y arreglos

En los ejemplos, las matrices suelen representarse como arreglos 1D en orden `row-major`:

```cpp
matriz[fila * n + columna]
```

Esto simplifica `Scatter`, `Gather` y acceso contiguo.

---

## 11. Manejo de tamaños no divisibles

### 11.1 Dos patrones reales

1. Repartir el resto en los primeros procesos:

```cpp
conteo = base + (rank < resto ? 1 : 0);
```

2. Exigir divisibilidad y abortar si no se cumple:

```cpp
if (n % nprocs != 0) {
    MPI_Abort(MPI_COMM_WORLD, 1);
}
```

### 11.2 Cuándo usar cada uno

- Si el ejercicio pide simplicidad matemática, la divisibilidad puede ser una restricción válida.
- Si el objetivo es robustez, conviene repartir el resto.

---

## 12. Gestión de memoria

### 12.1 Patrón observado

- Reservar en `rank 0` solo lo que realmente necesita.
- Reservar buffers locales en cada proceso.
- Liberar con el mismo proceso que reservó.

### 12.2 Reglas prácticas

- cada `new[]` debe tener su `delete[]`
- no reservar dentro de bucles si se puede evitar
- no crear buffers globales enormes si cada proceso solo usa una parte
- no usar punteros sin inicializar

### 12.3 Buffers grandes

Si el problema tiene imagen, matriz o volumen:

- separar `local_buffer`
- separar `global_buffer` solo en `rank 0` si hace falta
- calcular el tamaño en elementos, no en bytes, salvo que la API lo pida

---

## 13. Sincronización y terminación

### 13.1 Criterio general

Todos los procesos deben seguir el mismo protocolo de fases.

Si `rank 0` hace:

1. `MPI_Bcast`
2. cálculo local
3. `MPI_Gather`

entonces los demás deben hacer exactamente eso mismo, en el mismo orden.

### 13.2 Terminación distribuida

Cuando `rank 0` decide cerrar:

- difunde una bandera como `running = 0`
- los demás la reciben
- todos salen del ciclo
- todos llegan a `MPI_Finalize`

### 13.3 Riesgo típico

El error más común es que un proceso abandone antes de una colectiva obligatoria. Eso deja al resto bloqueado.

---

## 14. Tiempos y medición

### 14.1 Herramienta usada

Los proyectos usan `MPI_Wtime` para medir tiempo paralelo.

### 14.2 Recomendación

Medir:

- tiempo total
- tiempo de cómputo local
- tiempo de comunicación
- tiempo de ensamblado o salida

### 14.3 Buena práctica

El tiempo global paralelo normalmente debe tomarse como el máximo entre procesos, no como la media.

---

## 15. Modelo mental para diseñar un nuevo proyecto MPI

Cuando te propongan un problema nuevo, seguir este orden:

### 15.1 Análisis inicial

1. Identificar el trabajo secuencial.
2. Identificar el trabajo paralelizable.
3. Decidir si el problema merece MPI.
4. Clasificar los datos.
5. Determinar el tamaño local por proceso.

### 15.2 Diseño

6. Elegir la descomposición.
7. Definir qué ve cada proceso.
8. Definir qué comunica cada proceso.
9. Elegir colectivas o punto a punto.
10. Diseñar la salida final.

### 15.3 Implementación

11. Escribir primero la versión secuencial.
12. Pasar a la versión MPI con una estructura clara.
13. Añadir validaciones y manejo de errores.
14. Probar con 1 proceso.
15. Probar con varios procesos.
16. Probar tamaños no divisibles si aplica.

---

## 16. Plantilla de arquitectura recomendada

### 16.1 `main.cpp`

Responsabilidades:

- `MPI_Init` / `MPI_Finalize`
- lectura de argumentos
- difusión de parámetros
- división del dominio
- bucle principal
- recepción o reducción de resultados
- impresión final

### 16.2 `algoritmo.cpp`

Responsabilidades:

- kernel principal
- cálculo local
- transformación de datos
- validación matemática del resultado

### 16.3 `datos.cpp`

Responsabilidades:

- generación de entradas
- lectura de archivos
- preparación de arreglos
- validación de rangos

### 16.4 `utilidades.cpp`

Responsabilidades:

- impresión
- formateo
- temporización
- mensajes de error

---

## 17. Convenciones prácticas para escribir el código

### 17.1 Reglas de claridad

- Un proceso = una responsabilidad clara.
- Una función = una tarea concreta.
- Un buffer = una finalidad concreta.

### 17.2 Reglas de robustez

- comprobar argumentos de entrada
- validar tamaños y divisibilidad cuando sea necesario
- no asumir que `argc` tiene lo esperado
- no asumir que todos los procesos comparten estado implícito

### 17.3 Reglas MPI

- usar siempre el mismo comunicador para una fase
- respetar el orden de las colectivas
- verificar tipos MPI y conteos
- no dejar solicitudes activas sin completar

---

## 18. Recomendación de estilo para nuevos ejercicios

La forma más sana de programar estos ejercicios es:

- empezar simple
- conservar una versión secuencial correcta
- hacer explícita la distribución
- reducir la comunicación al mínimo necesario
- centralizar solo lo que realmente debe centralizar `rank 0`
- documentar cada decisión de paralelización

Si el problema cambia, la estrategia también puede cambiar. Lo que debe permanecer estable es el método:

1. entender el problema
2. detectar paralelismo
3. repartir trabajo
4. comunicar solo lo necesario
5. verificar resultados

---

## 19. Guía corta reutilizable

Cuando me pidas un nuevo proyecto MPI, esta será la secuencia que seguiré:

1. analizar el problema
2. decidir si MPI conviene
3. definir la descomposición
4. distinguir datos locales y globales
5. diseñar la comunicación
6. elegir comunicadores y etiquetas
7. escribir primero una versión secuencial
8. implementar la distribución MPI
9. validar con `1` y varios procesos
10. medir y corregir cuellos de botella

---

## 20. Conclusión

Los proyectos del repositorio siguen una misma filosofía:

- `rank 0` coordina
- los demás procesos trabajan en paralelo
- la distribución se hace con bloques claros
- la comunicación se mantiene simple
- la lógica matemática se separa cuando el problema crece

Esa es la estructura que conviene conservar. A partir de aquí, cada nuevo problema debe adaptar esa base a su propia geometría, tamaño de datos, coste de comunicación y necesidades de sincronización.
