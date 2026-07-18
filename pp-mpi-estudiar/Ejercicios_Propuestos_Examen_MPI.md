# Propuesta de ejercicios de examen — MPI

> Estos son **enunciados propuestos**, no soluciones. Están pensados para que
> los resuelvas tú mismo/a como práctica de examen. Se diseñaron a partir de:
> - Los conceptos cubiertos en `Capitulo_8_MPI_completo_es.md` y
>   `Que_es_MPI_explicado_con_analogias.md`.
> - Lo que ya funciona bien en `08.ejemplo-mpi/` (Scatterv/Gatherv) y en
>   `mpi/` (Bcast/Reduce).
> - Los errores encontrados en las carpetas de ejemplo previas (código de
>   depuración que sobrescribe el resultado real, "comunicación" simulada
>   que en realidad no envía nada, deadlocks, bugs de copy-paste entre
>   variantes). Los criterios de evaluación de cada ejercicio apuntan
>   justamente a que esos errores no puedan pasar desapercibidos.

Los ejercicios 1 a 8 evitan a propósito el dominio exacto de los
proyectos ya resueltos (nada de matrices ni fractales), para que
practiques los mismos conceptos en problemas nuevos. Sin embargo, como las
matrices son un tema muy trabajado en el curso (`04_ejemplo_mpi...`,
`08.ejemplo-mpi`) es probable que también aparezcan en el examen, así que
se añadió una sección aparte (Ejercicios 9 a 14) con variantes de
matrices que no son simplemente una copia de `08.ejemplo-mpi/mpi-matrices-mult.cpp`.

---

## Tabla de cobertura de conceptos

| # | Ejercicio | Conceptos principales |
|---|---|---|
| 1 | Anillo de mensajes | `Send`/`Recv` punto a punto, orden de envío/recepción, `tag` |
| 2 | Intercambio de vecinos (bloqueante) | Deadlock, `MPI_Sendrecv` |
| 3 | Intercambio de vecinos (no bloqueante) | `MPI_Isend`/`MPI_Irecv`/`MPI_Waitall`, solapar cómputo y comunicación |
| 4 | Estadísticas de un arreglo grande | `MPI_Scatter`, `MPI_Gather`, `MPI_Reduce` |
| 5 | Configuración compartida y cronometraje justo | `MPI_Bcast`, `MPI_Barrier`, `MPI_Wtime` |
| 6 (opcional) | Difusión de calor 1D | Ghost cells / halo exchange iterativo, `Isend`/`Irecv` con vecinos |
| 7 (opcional) | Votación distribuida | `MPI_Allreduce`, `MPI_Allgather`, diferencia con `Reduce`/`Gather` |
| 8 (opcional, avanzado) | Registros de sensores con tipo de dato propio | `MPI_Type_create_struct`, `MPI_Type_commit`/`MPI_Type_free`, `Scatterv`/`Gatherv` con conteos desiguales |
| 9 | Suma de dos matrices por filas | `MPI_Scatter`, `MPI_Gather`, distribución por filas |
| 10 | Matriz × vector con verificación secuencial | `MPI_Bcast`, `MPI_Scatter`/`MPI_Scatterv`, `MPI_Gather`/`MPI_Gatherv` |
| 11 | Matriz × matriz por bloques de filas | `MPI_Bcast` de una matriz completa, `Scatter`/`Gather` de filas, cómputo local con doble ciclo |
| 12 | Suma de las diagonales de una matriz distribuida | Mapeo de índice local → global, `MPI_Reduce` |
| 13 (opcional) | Transposición de una matriz distribuida | `MPI_Alltoall`/`MPI_Alltoallv` |
| 14 (opcional, avanzado) | Producto matriz-matriz con malla 2D de procesos | Topología cartesiana (`MPI_Cart_create`), comunicadores de fila/columna |

---

## Ejercicio 1 — Anillo de mensajes (Send/Recv básico)

**Objetivo:** practicar comunicación punto a punto y el uso correcto de
`tag` para no confundir mensajes.

**Enunciado:** cada proceso `rank` debe recibir un número entero desde el
proceso `rank - 1` (el proceso 0 lo recibe del último, cerrando el anillo),
sumarle su propio `rank`, e imprimir el resultado con formato
`"Proceso X recibió Y y envía Z"`. El proceso 0 es quien "inicia" el anillo
con un valor inicial (por ejemplo, 100). Al final, el proceso 0 debe
recibir de vuelta el valor acumulado por todos los procesos y verificar
que coincide con la suma esperada (100 + suma de todos los ranks).

**Restricciones:**
- Debe funcionar para cualquier número de procesos `nprocs >= 2` pasado por
  línea de comandos a `mpiexec -n`.
- Usa un `tag` fijo y documenta por qué ese valor no genera ambigüedad aquí.
- El programa debe terminar (ni debe quedarse esperando ni debe truncar
  el anillo).

---

## Ejercicio 2 — Intercambio de vecinos con `MPI_Sendrecv`

**Objetivo:** experimentar de primera mano el riesgo de deadlock y
resolverlo con la operación combinada de MPI.

**Enunciado:** imagina un arreglo de "estaciones meteorológicas", una por
proceso, ordenadas en línea (rank 0, 1, 2, ..., n-1). Cada estación mide una
temperatura local (puedes generarla con un valor determinístico basado en
el rank, por ejemplo `20.0 + rank`). Cada estación necesita conocer la
temperatura de su vecino de la derecha (`rank + 1`) y enviarle la suya, de
forma simultánea (excepto las estaciones en los extremos, que solo tienen
un vecino).

**Restricciones:**
- Primero escribe (en un comentario o en un archivo aparte) por qué una
  versión ingenua con `MPI_Send` seguido de `MPI_Recv` en todos los
  procesos, en ese orden, puede colgarse.
- La solución final debe usar `MPI_Sendrecv` en una sola llamada.
- Cada proceso debe imprimir su propia temperatura y la de su vecino
  recibida.

---

## Ejercicio 3 — El mismo intercambio, pero no bloqueante

**Objetivo:** comparar el enfoque bloqueante con el no bloqueante y medir
si hay diferencia de tiempo cuando se simula algo de trabajo entre medio.

**Enunciado:** repite el Ejercicio 2, pero ahora usando `MPI_Isend` y
`MPI_Irecv`. Entre el momento en que se lanza la comunicación y el momento
en que se llama a `MPI_Wait` (o `MPI_Waitall`), cada proceso debe realizar
algún cómputo local que no dependa del dato del vecino (por ejemplo, una
suma de una serie de 1 a 1,000,000). Mide el tiempo total con `MPI_Wtime`
y compara contra la versión del Ejercicio 2.

**Restricciones:**
- Debe haber exactamente una llamada a `MPI_Isend` y una a `MPI_Irecv` por
  vecino con el que se comunica cada proceso.
- No se debe leer ni escribir el buffer de comunicación antes de que el
  `Wait` correspondiente confirme que terminó (evita condiciones de carrera
  con datos "a medio llegar").
- Reporta si hubo o no una mejora de tiempo medible, y una hipótesis de
  por qué.

---

## Ejercicio 4 — Estadísticas de un arreglo grande

**Objetivo:** practicar el patrón repartir-procesar-combinar con
operaciones colectivas, sin usar matrices.

**Enunciado:** el proceso 0 genera un arreglo de `N` números de punto
flotante (`N` grande, por ejemplo 1,000,000, con valores aleatorios entre
0 y 100). Reparte el arreglo entre todos los procesos con `MPI_Scatter`
(asume que `N` es múltiplo de `nprocs` para simplificar, o maneja el resto
si quieres un reto extra). Cada proceso calcula el mínimo, el máximo y la
suma de su porción. Luego:
- Usa `MPI_Reduce` para obtener el mínimo global, el máximo global y la
  suma global (con las operaciones `MPI_MIN`, `MPI_MAX` y `MPI_SUM`
  respectivamente) en el proceso 0.
- El proceso 0 imprime el promedio global (`suma / N`), el mínimo y el
  máximo.

**Restricciones:**
- Verifica el resultado calculando también las mismas estadísticas de
  forma puramente secuencial (sin MPI, en el mismo programa, solo en el
  rank 0, antes de repartir) y compara ambos resultados; deben coincidir.
  Esto es a propósito: en proyectos anteriores del curso hubo casos donde
  el resultado paralelo impreso no era el resultado real por un error en
  el código — este chequeo debe delatar ese tipo de error si ocurre.
- No uses `MPI_Gather` de todo el arreglo de vuelta (no hace falta para
  este ejercicio); la combinación debe hacerse con reducciones, no
  reuniendo todos los datos crudos.

---

## Ejercicio 5 — Configuración compartida y cronometraje justo

**Objetivo:** practicar `Bcast` para evitar que todos los procesos lean el
mismo archivo/dato por separado, y `Barrier` para medir tiempos de forma
justa.

**Enunciado:** el proceso 0 "lee" una configuración (puede ser simplemente
un struct o un par de variables: `int iteraciones` y `double factor`,
puestos directamente en el código o leídos de un argumento). Distribúyela a
todos los procesos con una sola llamada a `MPI_Bcast` (si son varios
valores, agrúpalos en un arreglo o un `struct` en vez de hacer varios
`Bcast` sueltos). Luego, cada proceso ejecuta un bucle de `iteraciones`
pasos haciendo algo de cómputo dependiente de `factor` (por ejemplo, ir
acumulando `acumulador += factor * i`), donde el número de iteraciones que
le toca a cada proceso varía artificialmente según su rank (por ejemplo,
`iteraciones * (rank + 1)`), para simular que unos procesos tardan más que
otros.

Antes de cronometrar, sincroniza a todos los procesos con `MPI_Barrier` y
usa `MPI_Wtime` inmediatamente después de la barrera. Al final, usa
`MPI_Reduce` con `MPI_MAX` sobre el tiempo local de cada proceso para
reportar cuánto tardó el proceso más lento (igual que el ejemplo de la
Sección 8.3.1 del capítulo).

**Restricciones:**
- Explica en un comentario qué pasaría con la medición de tiempo si no se
  pusiera el `Barrier` antes de arrancar el cronómetro.
- Todos los procesos deben usar el `factor` y las `iteraciones` recibidas
  por `Bcast`, no un valor propio.

---

## Ejercicios opcionales (para cobertura completa)

Estos tres son opcionales — resuélvelos solo si quieres practicar también
los conceptos más avanzados del capítulo 8 (secciones 8.4 a 8.6).

### Ejercicio 6 (opcional) — Difusión de calor en una barra 1D (ghost cells)

Divide una barra de `N` celdas entre los procesos (cada proceso posee un
tramo contiguo). En cada iteración, cada celda actualiza su valor como el
promedio de sus dos vecinas (`nuevo[i] = (viejo[i-1] + viejo[i+1]) / 2`).
Para que esto funcione en los bordes de cada tramo, cada proceso necesita
conocer el último valor de la celda del proceso vecino (una "celda
fantasma" a cada lado), que debe actualizarse en cada iteración con
`Isend`/`Irecv` antes de recalcular. Los extremos absolutos de la barra
(rank 0 por la izquierda, último rank por la derecha) no tienen vecino de
ese lado y deben mantener un valor fijo (condición de frontera). Corre
varias iteraciones e imprime cómo evoluciona la temperatura.

### Ejercicio 7 (opcional) — Votación distribuida (Allreduce/Allgather)

Cada proceso "vota" por un candidato representado con un entero (genera el
voto con una regla determinística basada en el rank, para poder verificar
el resultado a mano). Usa `MPI_Allreduce` para que **todos** los procesos
conozcan el total de votos de un candidato específico sin que haga falta
un `Bcast` posterior (a diferencia de `MPI_Reduce`, que solo lo entrega al
rank 0). Luego, usa `MPI_Allgather` para que todos los procesos tengan la
lista completa de votos individuales de todos los demás, y que cada uno
imprima el ganador calculado localmente. Verifica que todos los procesos
calculan el mismo ganador.

### Ejercicio 8 (opcional, avanzado) — Registros de sensores con tipo de dato MPI personalizado

Define un `struct` en C++ con varios campos de distinto tipo (por ejemplo,
`int id_sensor; double temperatura; double humedad;`). Crea un
`MPI_Datatype` con `MPI_Type_create_struct` que describa ese struct, y
recuerda llamar a `MPI_Type_commit` antes de usarlo y a `MPI_Type_free` al
terminar. El proceso 0 genera un arreglo de registros (cantidad no
necesariamente múltiplo de `nprocs`) y los reparte con `MPI_Scatterv`
usando tu tipo personalizado; cada proceso calcula el promedio de
temperatura de los registros que recibió y lo devuelve con `MPI_Gatherv`.

---

## Ejercicios con matrices

Estos ejercicios reutilizan el mismo patrón de `08.ejemplo-mpi` (repartir
filas, calcular localmente, reunir resultados), pero en problemas distintos
a los que ya están resueltos en el curso, para que no puedas simplemente
copiar el código existente sin entenderlo.

**Convención común para todos:** representa la matriz como un arreglo 1D
en *row-major* (`double* datos = new double[filas * columnas]`, donde el
elemento `(i, j)` está en `datos[i * columnas + j]`), en vez de usar un
arreglo de punteros o un `std::vector<std::vector<double>>`. Es la forma en
que `MPI_Scatter`/`MPI_Gather` esperan los datos: contiguos en memoria.

### Ejercicio 9 — Suma de dos matrices por filas

**Objetivo:** el ejemplo más simple posible de repartir una matriz por
filas con `MPI_Scatter` y volver a juntarla con `MPI_Gather`.

**Enunciado:** el proceso 0 genera dos matrices cuadradas `A` y `B` de
tamaño `N x N` (con `N` múltiplo de `nprocs`, para simplificar). Reparte
ambas por filas (un bloque de `N/nprocs` filas por proceso) usando
`MPI_Scatter`. Cada proceso calcula `C_local = A_local + B_local`
(suma elemento a elemento de sus filas) y el resultado se reúne en el
proceso 0 con `MPI_Gather` para formar `C` completa.

**Restricciones:**
- El proceso 0 debe imprimir `C` y verificar contra una suma secuencial de
  `A + B` calculada antes de repartir.
- No repartas `A` y `B` juntas en un solo `Scatter`; hazlo con dos
  llamadas separadas y explica por qué mezclarlas en una sola sería más
  complicado (tendrías que armar un buffer combinado a mano).

### Ejercicio 10 — Matriz × vector con verificación secuencial

**Objetivo:** repetir el patrón de multiplicación matriz-vector, pero esta
vez con foco en la **verificación** del resultado, no solo en que "corra".

**Enunciado:** genera una matriz `A` de tamaño `M x N` y un vector `x` de
tamaño `N`. Distribuye las filas de `A` con `MPI_Scatter` (o `Scatterv` si
`M` no es múltiplo de `nprocs`) y distribuye `x` completo a todos los
procesos con `MPI_Bcast` (todos lo necesitan completo, no repartido). Cada
proceso calcula el producto de sus filas por `x` y los resultados
parciales se juntan con `Gather`/`Gatherv` en el vector resultado `y`.

**Restricciones:**
- Antes de repartir nada, calcula `y` de forma puramente secuencial en el
  rank 0 y guárdalo aparte.
- Después de la versión paralela, compara elemento por elemento `y`
  paralelo contra el secuencial e imprime explícitamente si coinciden o
  no (no asumas que "si no truena, está bien" — ese fue exactamente el
  bug de `04_ejemplo_mpi_Programacion_Paralela/matrices_mult.cpp`, donde
  se imprimía un valor de depuración en vez del resultado real).

### Ejercicio 11 — Matriz × matriz por bloques de filas

**Objetivo:** dar el salto de matriz-vector a matriz-matriz, que es
computacionalmente más pesado y expone mejor los errores de indexado.

**Enunciado:** calcula `C = A x B`, con `A` de tamaño `M x K` y `B` de
tamaño `K x N`. Como `B` la necesita completa cada proceso para poder
calcular cualquier fila de `C`, distribúyela entera con `MPI_Bcast` a
todos. Reparte solo las filas de `A` con `Scatter`/`Scatterv`. Cada
proceso calcula las filas de `C` que le corresponden (un triple ciclo:
filas propias × columnas de `B` × la dimensión `K` que se recorre para
cada producto punto), y junta el resultado con `Gather`/`Gatherv`.

**Restricciones:**
- Verifica el resultado contra una versión secuencial completa, igual que
  en el Ejercicio 10.
- Reporta el tiempo con `MPI_Wtime` para `M`, `K`, `N` grandes (por
  ejemplo 512 o 1024) y compara el tiempo con 1, 2 y 4 procesos.

### Ejercicio 12 — Suma de las diagonales de una matriz distribuida

**Objetivo:** practicar el mapeo entre índice **local** (dentro de las
filas que le tocaron a un proceso) e índice **global** (la fila real
dentro de la matriz completa), una fuente común de bugs sutiles.

**Enunciado:** una matriz cuadrada `N x N` se reparte por filas entre los
procesos (igual que en los ejercicios anteriores). Cada proceso debe
identificar cuáles de sus filas locales contienen un elemento de la
diagonal principal (`fila_global == columna`) y de la diagonal
secundaria/antidiagonal (`fila_global + columna == N - 1`), sumar esos
valores, y combinar ambas sumas de todos los procesos con dos
`MPI_Reduce` (uno por cada diagonal) en el proceso 0.

**Restricciones:**
- Cada proceso debe calcular explícitamente su desplazamiento
  (`fila_global = rank * filas_por_proceso + fila_local`) — no asumas que
  el índice local y el global son el mismo número.
- Verifica contra el cálculo secuencial de ambas sumas antes de repartir.

### Ejercicio 13 (opcional) — Transposición de una matriz distribuida

**Objetivo:** conocer `MPI_Alltoall`, la operación colectiva donde **cada**
proceso envía datos distintos a **cada** otro proceso (no cubierta en los
ejercicios 1 a 12).

**Enunciado:** una matriz cuadrada `N x N` (con `N` múltiplo de `nprocs`)
se reparte por bloques de filas. Calcula su transposición **distribuida**:
al terminar, el proceso `r` debe tener las filas que le corresponden de la
matriz transpuesta, sin que el proceso 0 llegue a tener nunca la matriz
completa en memoria. Investiga cómo `MPI_Alltoall` (con bloques de tamaño
fijo) o `MPI_Alltoallv` (si necesitas tamaños distintos) permiten que cada
proceso reciba, de cada uno de los demás, exactamente el sub-bloque que le
corresponde.

**Restricciones:**
- Verifica el resultado reuniendo (solo para la verificación, con
  `Gather`) la matriz transpuesta distribuida y comparándola contra la
  transposición secuencial.
- Explica en un comentario la diferencia entre este ejercicio y usar
  `Gather` + transponer en un solo proceso + `Scatter` (¿por qué la
  versión con `Alltoall` es preferible cuando la matriz es muy grande?).

### Ejercicio 14 (opcional, avanzado) — Producto matriz-matriz con malla 2D de procesos

**Objetivo:** combinar comunicación colectiva con topología cartesiana
(sección 8.5.2 del capítulo), yendo más allá de la distribución simple
por filas.

**Enunciado:** en vez de repartir `A` solo por filas, organiza los
procesos en una malla 2D (por ejemplo `2 x 2` para 4 procesos) usando
`MPI_Cart_create`, y reparte tanto `A` como `B` en bloques (submatrices)
según esa malla, no solo por filas. Cada proceso calcula el producto de su
bloque de `A` por el bloque correspondiente de `B` y acumula su
contribución al bloque de `C` que le toca. (No hace falta implementar el
algoritmo de Cannon completo con rotación de bloques; alcanza con que cada
proceso conozca su posición `(fila, columna)` en la malla vía
`MPI_Cart_coords` y calcule su bloque correctamente).

**Restricciones:**
- Usa `MPI_Cart_create` para crear la topología y `MPI_Cart_coords` para
  que cada proceso determine su posición en la malla.
- Verifica el resultado contra la multiplicación secuencial completa.

---

## Checklist de buenas prácticas a autoevaluar al terminar cada ejercicio

- [ ] `MPI_Init` y `MPI_Finalize` están presentes y son las únicas llamadas
      de ese tipo en el programa.
- [ ] El resultado impreso es el resultado **real** calculado por MPI, no
      un valor de prueba/depuración que quedó pisando el cálculo correcto.
- [ ] No queda ningún `MPI_Send`/`MPI_Recv` cuyo orden dependa de que "por
      suerte" no se cuelgue (revisar si aplica `Sendrecv` o no bloqueante).
- [ ] Si el número de procesos no divide exacto el tamaño del problema, el
      programa lo maneja explícitamente (no lo ignora ni trunca datos).
- [ ] Cada variante/archivo del ejercicio que se entrega realmente está
      referenciada en el `CMakeLists.txt` y compila.
- [ ] Los `MPI_Datatype` personalizados (si los usaste) se liberan con
      `MPI_Type_free`.
