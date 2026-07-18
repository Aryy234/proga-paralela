# ¿Qué es MPI? Explicado con una analogía de la vida real

> Basado en el Capítulo 8 de *Essentials of Parallel Computing* (`Capitulo_8_MPI_completo_es.md`).

## La idea en una frase

**MPI (Message Passing Interface)** es un conjunto de reglas y funciones que permite que **muchos programas independientes**, que no comparten memoria entre sí, **se coordinen enviándose mensajes** para resolver juntos un problema demasiado grande para uno solo.

Nada más. No es un lenguaje, no es un compilador especial: es una biblioteca. Pero para entenderla de verdad, necesitamos una historia.

---

## 1. La analogía: una cadena de sucursales sin archivo compartido

Imagina que una empresa de logística recibe un pedido gigantesco: procesar **un millón de paquetes en un solo día**. Una sola oficina jamás terminaría a tiempo, así que la empresa abre **varias sucursales idénticas**, cada una con su propio equipo y su propio archivador.

Punto clave de esta empresa: **ninguna sucursal puede ver el archivador de otra**. Si la sucursal 3 necesita un dato que tiene la sucursal 7, la única forma de conseguirlo es **enviando un mensajero con una carta**. No hay pizarra compartida, no hay nube común: solo mensajes entre oficinas.

Eso es exactamente el modelo de MPI: **memoria distribuida**. Cada "proceso" es como una sucursal — dueño exclusivo de sus propios datos, y la única forma de intercambiar información con otro es mandar y recibir mensajes explícitos.

### Paso a paso de la analogía

**Paso 1 — Abrir y cerrar las sucursales el mismo día**
Cada mañana, todas las sucursales abren sus puertas al mismo tiempo, siguiendo el mismo protocolo de la empresa. Al final del día, todas cierran siguiendo también un protocolo común. Ninguna sucursal puede simplemente "aparecer" a media mañana o "desaparecer" sin avisar.

**Paso 2 — A cada sucursal le dan un número, y todas forman "la red"**
La central le asigna a cada sucursal un número de identificación (la sucursal 0, la 1, la 2…) y las agrupa en un directorio llamado "la red de la empresa". Ese número es único e inequívoco: es cómo un mensajero sabe exactamente a quién entregarle una carta.

**Paso 3 — Enviar y recibir cartas (el corazón del sistema)**
Cuando la sucursal 3 quiere mandarle un dato a la sucursal 7, escribe una carta con:
- El **contenido** (los datos),
- **cuántas unidades** de datos son y de **qué tipo**,
- y un **sobre** con la dirección: a quién va, un número de referencia (para no confundir dos cartas distintas) y el grupo al que pertenece.

Aquí hay una regla de oro: **debe haber alguien esperando el paquete en el buzón de destino**. Si el cartero llega y no hay buzón instalado (nadie "publicó" la recepción), tiene que quedarse esperando a que alguien lo instale — o, peor, guardar el paquete en un depósito temporal hasta que haya espacio. Por eso, en una oficina bien organizada, **primero se prepara el buzón de recepción y luego se despacha el envío**.

**Paso 4 — El peligro de esperar mal: el "cuelgue"**
Imagina que la sucursal A dice: "no voy a hacer nada hasta recibir el paquete de B", y al mismo tiempo la sucursal B dice: "no voy a enviar nada hasta recibir el paquete de A". **Ambas se quedan esperando para siempre.** Esto es un *deadlock* (cuelgue): dos partes bloqueadas, cada una esperando algo que la otra nunca hará porque también está esperando.

La solución elegante: en vez de que cada sucursal decida por su cuenta el orden de "enviar" y "recibir", existe un protocolo combinado — como un servicio de **mensajería certificada** — donde uno mismo dice "manda esto Y recibe aquello" en una sola operación, y es la empresa de mensajería (la biblioteca MPI) la que garantiza que nunca se traben. También existe la variante "**envía y sigue trabajando**" (no bloqueante): el mensajero sale a repartir mientras el empleado sigue con otras tareas, y solo se detiene a comprobar cuando realmente necesita el resultado.

**Paso 5 — Comunicados que involucran a todas las sucursales a la vez**
Además de las cartas privadas entre dos sucursales, la empresa tiene procedimientos para cuando *todas* las sucursales necesitan participar a la vez:

- **Circular informativa (Broadcast):** la central redacta un memo una sola vez y lo copia idéntico a todas las sucursales. *(Ejemplo real del capítulo: leer un archivo de configuración una sola vez en la sucursal 0 y distribuirlo a todas, en vez de que las mil sucursales intenten abrir el mismo archivo a la vez y colapsen el sistema.)*

- **Reporte y consolidación (Reduce):** cada sucursal informa sus ventas del día, y la central las suma (o calcula el máximo, el mínimo, el promedio) para obtener **un solo número final**, que queda solo en la central. Si además la empresa quiere que **todas** las sucursales conozcan ese total consolidado, se usa la variante "para todos" (**Allreduce**).

- **Reunión de sincronización (Barrier):** un punto de control donde se avisa "nadie sigue hasta que todas las sucursales lleguen aquí". Útil para medir tiempos de forma justa (por ejemplo, cronometrar cuánto tarda el trabajo más lento), pero cuesta productividad si se abusa de ella.

- **Repartir el trabajo (Scatter) y recoger los resultados (Gather):** la central tiene una lista gigante de 1,000 clientes. En vez de mandarla completa a cada sucursal, **la corta en pedazos y le entrega un pedazo distinto a cada una** (Scatter). Cuando cada sucursal termina su parte, la central vuelve a juntar todos los resultados parciales en un solo listado ordenado (Gather). Si en cambio se necesita que **cada** sucursal tenga la lista combinada completa de todos, se usa "recoger para todos" (**Allgather**).

**Paso 6 — Vecinos que comparten frontera (el caso más avanzado)**
Supongamos que el territorio de reparto se divide en un mapa de zonas rectangulares, una por sucursal, como un tablero de ajedrez. Los clientes que viven justo en el límite entre la zona de la sucursal A y la zona de la sucursal B necesitan que **ambas sucursales conozcan lo que pasa justo al otro lado de la frontera** — quién vive ahí, qué pedidos hay pendientes cerca del límite. Por eso, las sucursales vecinas se intercambian constantemente "la información del borde" con sus vecinas inmediatas (arriba, abajo, izquierda, derecha), sin necesidad de involucrar a sucursales lejanas. Esto es exactamente lo que en computación se llama **intercambio de celdas fantasma (ghost cells)** en un cálculo sobre una malla.

---

## 2. Ahora, en computación: ¿cómo se ve todo esto en código?

Traduzcamos cada parte de la historia a lo que realmente ocurre en una computadora (o, más típicamente, en un **clúster o supercomputadora** con muchos nodos conectados en red).

| En la analogía | En computación |
|---|---|
| Sucursal | **Proceso**: una unidad de cómputo independiente con su propia porción de memoria. Puede vivir en otro núcleo, otra CPU o incluso otra máquina física (nodo) distinta. |
| Número de sucursal | **Rank**: un entero de `0` a `nprocs-1` que identifica a cada proceso de forma única. |
| "La red de la empresa" | **Comunicador** (`MPI_COMM_WORLD`): el grupo de todos los procesos que pueden comunicarse entre sí. Se puede crear un comunicador más chico para un subgrupo. |
| Abrir/cerrar la sucursal | `MPI_Init(&argc, &argv)` al arrancar el programa, `MPI_Finalize()` al terminar. Son obligatorias y van al principio y al final de *todo* programa MPI. |
| Carta con contenido + sobre | Un mensaje MPI = **puntero a los datos + cantidad (`count`) + tipo (`MPI_Datatype`)**, más un sobre con **rank de destino + tag + comunicador**. |
| Buzón que debe estar listo antes de que llegue el cartero | Buena práctica: publicar el `receive` antes que el `send`, para que MPI no tenga que guardar el mensaje en un buffer temporal. |
| Dos sucursales esperándose mutuamente para siempre | **Deadlock**: dos llamadas bloqueantes (`MPI_Send`/`MPI_Recv`) mal ordenadas que nunca se cumplen. |
| Servicio de mensajería certificada que evita el enredo | `MPI_Sendrecv(...)`: combina el envío y la recepción en una sola llamada seleccionada; MPI garantiza que no se cuelgue. |
| "Manda esto y sigue trabajando" | `MPI_Isend` / `MPI_Irecv` (la `I` es de *immediate*, es decir **no bloqueante**): la llamada retorna de inmediato y luego se usa `MPI_Wait`/`MPI_Waitall` para confirmar que terminó. |
| Circular a todas las sucursales | `MPI_Bcast(buffer, count, tipo, rank_origen, comm)` |
| Consolidar reportes en un número | `MPI_Reduce(&valor_local, &resultado, 1, tipo, MPI_SUM/MPI_MAX/MPI_MIN, rank_destino, comm)` |
| Consolidar y que todos lo sepan | `MPI_Allreduce(...)` (mismo resultado, pero copiado a todos los procesos) |
| Reunión de sincronización | `MPI_Barrier(comm)` |
| Repartir un trabajo grande en pedazos | `MPI_Scatter(...)` |
| Juntar resultados parciales en uno solo | `MPI_Gather(...)` |
| Que todos tengan la lista combinada | `MPI_Allgather(...)` |
| Vecinos compartiendo el borde del mapa | Intercambio de **ghost cells / halo exchange** entre procesos vecinos en una malla 2D o 3D, usando `Isend`/`Irecv` con los vecinos de arriba, abajo, izquierda y derecha. |

### Anatomía de un mensaje: el contenido y el sobre, con mini ejemplos

Todo mensaje MPI tiene dos mitades: **qué envías** (contenido) y **a quién / cómo lo identificas** (sobre).

**El contenido = puntero + cantidad + tipo**

1. **Puntero a los datos** — la dirección de memoria de lo que envías.

   ```c
   double temperatura = 36.5;
   MPI_Send(&temperatura, ...);   // &temperatura = "aquí está el dato"

   double lista[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
   MPI_Send(lista, ...);          // un arreglo ya es un puntero, sin &
   ```

2. **Cantidad (`count`)** — cuántos elementos van, no cuántos bytes.

   ```c
   MPI_Send(lista, 5, ...);          // "van 5 elementos"
   MPI_Send(&temperatura, 1, ...);   // "va 1 solo elemento"
   ```

3. **Tipo (`MPI_Datatype`)** — de qué está hecho cada elemento (`MPI_INT`, `MPI_DOUBLE`, `MPI_CHAR`, etc.), para que MPI sepa cuántos bytes copiar y cómo convertirlos si hace falta.

   ```c
   int edad = 30;
   MPI_Send(&edad, 1, MPI_INT, ...);

   double precio = 19.99;
   MPI_Send(&precio, 1, MPI_DOUBLE, ...);
   ```

   Juntando los tres: `MPI_Send(lista, 5, MPI_DOUBLE, ...)` = *"envía, desde `lista`, 5 elementos de tipo `double`"*.

**El sobre = rank de destino + tag + comunicador**

4. **Rank de destino** — el número de la sucursal a la que va la carta.

   ```c
   int destino = 3;
   MPI_Send(lista, 5, MPI_DOUBLE, destino, ...);  // "va para el proceso #3"
   ```

5. **Tag** — el número de referencia del sobre, para no confundir mensajes distintos entre el mismo par de procesos.

   ```c
   MPI_Send(&temperatura, 1, MPI_DOUBLE, destino, 100, comm);  // tag 100: "temperatura"
   MPI_Send(&presion,     1, MPI_DOUBLE, destino, 200, comm);  // tag 200: "presión"
   ```

6. **Comunicador** — la red postal a la que pertenecen emisor y receptor. Casi siempre es el grupo completo:

   ```c
   MPI_Comm comm = MPI_COMM_WORLD;   // "todos los procesos del programa"
   ```

**Ejemplo completo, pieza por pieza:**

```c
double lista[5] = {1.0, 2.0, 3.0, 4.0, 5.0};

MPI_Send(
    lista,          // 1. puntero  -> dónde están los datos
    5,              // 2. count    -> cuántos elementos
    MPI_DOUBLE,     // 3. type     -> de qué tipo es cada uno
    3,              // 4. dest     -> a qué rank va (proceso #3)
    100,            // 5. tag      -> número de referencia del mensaje
    MPI_COMM_WORLD  // 6. comm     -> grupo de procesos donde vive el destino
);
```

El `MPI_Recv` del otro lado es casi un espejo: en vez de "puntero a lo que envío" es "puntero a dónde voy a guardar lo que llegue".

```c
double recibido[5];
MPI_Recv(
    recibido,            // dónde guardar lo que llegue
    5, MPI_DOUBLE,        // cuántos y de qué tipo espero
    0,                    // de qué rank espero el mensaje (rank 0)
    100,                  // qué tag espero
    MPI_COMM_WORLD,
    MPI_STATUS_IGNORE     // no me interesa info extra del mensaje recibido
);
```

### El esqueleto mínimo de cualquier programa MPI

```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);              // "abrir la sucursal"

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // "¿cuál es mi número de sucursal?"
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs); // "¿cuántas sucursales hay en total?"

    printf("Soy la sucursal %d de %d\n", rank, nprocs);

    MPI_Finalize();                      // "cerrar la sucursal"
    return 0;
}
```

Este programa se compila con un *wrapper* especial (`mpicc programa.c -o programa`) y se lanza con un comando de arranque paralelo:

```bash
mpirun -n 4 ./programa
```

El `-n 4` le dice al sistema: "abre 4 sucursales (procesos) idénticas y ponlas a correr este mismo programa a la vez, cada una con su propio rank del 0 al 3".

### Enviar y recibir un dato entre dos procesos

```c
MPI_Sendrecv(xsend, count, MPI_DOUBLE, partner_rank, tag,
             xrecv, count, MPI_DOUBLE, partner_rank, tag,
             comm, MPI_STATUS_IGNORE);
```

Esta única llamada le pide a la biblioteca de MPI que se encargue de mandar `xsend` al `partner_rank` y, al mismo tiempo, recibir en `xrecv` lo que ese mismo compañero le envía — sin arriesgarse al "cuelgue" de la sucursal A esperando a la B y viceversa.

### Consolidar un valor de todos los procesos (Reduce)

```c
double mi_tiempo = MPI_Wtime() - inicio;
double tiempo_maximo;

MPI_Reduce(&mi_tiempo, &tiempo_maximo, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

if (rank == 0)
    printf("El proceso más lento tardó %lf segundos\n", tiempo_maximo);
```

Cada proceso (sucursal) mide cuánto tardó en su propio trabajo; `MPI_Reduce` los combina con la operación `MPI_MAX` y deja el resultado consolidado únicamente en el rank 0 — igual que la central recibiendo los reportes de ventas de todas las sucursales y quedándose con el peor (o mejor) número.

### Repartir el trabajo en pedazos: `MPI_Scatter` y `MPI_Scatterv`

Retomando el **Paso 5** de la analogía: la central tiene una lista gigante y en vez de mandarla completa a cada sucursal, **la corta en pedazos y le entrega un pedazo distinto a cada una**. `MPI_Scatter` es la versión de "pedazos todos del mismo tamaño"; `MPI_Scatterv` es la versión de "pedazos de tamaño distinto" (la `v` es de *vector*).

#### `MPI_Scatter` — reparto en partes iguales

```c
MPI_Scatter(
    sendbuf,     // 1. buffer de origen (solo importa en el root)
    sendcount,   // 2. cuántos elementos le toca A CADA proceso
    sendtype,    // 3. tipo de dato de lo que se envía
    recvbuf,     // 4. buffer local donde cada proceso guarda su porción
    recvcount,   // 5. cuántos elementos espera recibir cada proceso
    recvtype,    // 6. tipo de dato de lo que se recibe
    root,        // 7. rank de la sucursal que reparte (la central)
    comm         // 8. comunicador (la red postal)
);
```

**Parámetro por parámetro:**

1. **`sendbuf`** — el puntero a la lista completa que se va a repartir. **Solo tiene contenido válido en el proceso `root`**; en los demás procesos este argumento se ignora (por convención suele pasarse `NULL` o el buffer local, MPI no lo lee).
2. **`sendcount`** — cuántos elementos recibe **cada** proceso, no el total de la lista. Si `root` tiene un arreglo de 100 elementos y hay 4 procesos, `sendcount = 25` (100 / 4), porque MPI corta el buffer en bloques contiguos de ese tamaño y le da uno a cada rank, en orden (rank 0 se lleva el primer bloque, rank 1 el segundo, etc.).
3. **`sendtype`** — el tipo de dato (`MPI_INT`, `MPI_DOUBLE`, …) de los elementos que salen del `root`. Solo relevante en el `root`.
4. **`recvbuf`** — el puntero al buffer **local** de cada proceso (incluido el `root`, que también se queda con su propio pedazo) donde se va a guardar el trozo que le tocó.
5. **`recvcount`** — cuántos elementos va a recibir este proceso en `recvbuf`. Casi siempre es igual a `sendcount` (cada quien recibe lo mismo que el root promete enviar por proceso).
6. **`recvtype`** — el tipo de dato de lo que se recibe. Casi siempre igual a `sendtype`.
7. **`root`** — el rank de la sucursal "central" que posee la lista original y la reparte. Todos los procesos deben indicar el mismo `root` en su llamada.
8. **`comm`** — el comunicador (normalmente `MPI_COMM_WORLD`): el grupo completo de sucursales que participa en el reparto.

**Requisito importante:** `MPI_Scatter` exige que el total (`sendcount × nprocs`) se reparta en **partes exactamente iguales**. Si la lista de la central no se divide parejo entre las sucursales (por ejemplo, 100 clientes entre 3 sucursales), `MPI_Scatter` simple no alcanza — ahí es donde entra `MPI_Scatterv`.

#### `MPI_Scatterv` — reparto en partes de tamaño distinto

```c
MPI_Scatterv(
    sendbuf,     // 1. buffer de origen completo (solo en el root)
    sendcounts,  // 2. arreglo: cuántos elementos le toca a CADA proceso (puede variar)
    displs,      // 3. arreglo: desde qué posición del buffer empieza el pedazo de cada proceso
    sendtype,    // 4. tipo de dato de lo que se envía
    recvbuf,     // 5. buffer local donde cada proceso guarda su porción
    recvcount,   // 6. cuántos elementos espera recibir ESTE proceso
    recvtype,    // 7. tipo de dato de lo que se recibe
    root,        // 8. rank de la sucursal que reparte
    comm         // 9. comunicador
);
```

**Parámetro por parámetro (solo se explican los que cambian respecto a `Scatter`):**

1. **`sendbuf`** — igual que en `Scatter`: la lista completa, válida solo en el `root`.
2. **`sendcounts[]`** — a diferencia de `Scatter`, aquí **ya no es un solo número sino un arreglo** de tamaño `nprocs`: `sendcounts[i]` dice cuántos elementos le tocan al proceso de rank `i`. Permite que, por ejemplo, la sucursal 0 reciba 34 clientes, la sucursal 1 reciba 33 y la sucursal 2 reciba 33 (100 clientes entre 3, sin que sobre ni falte nadie).
3. **`displs[]`** (*displacements*, desplazamientos) — otro arreglo de tamaño `nprocs`, donde `displs[i]` indica **en qué posición del `sendbuf`** empieza el bloque que le corresponde al proceso `i`. Esto es lo que le da flexibilidad total: los pedazos ni siquiera tienen que ser contiguos ni estar en orden de rank (aunque casi siempre se usan contiguos, con `displs[i] = displs[i-1] + sendcounts[i-1]`).
4. **`sendtype`** — igual que en `Scatter`.
5. **`recvbuf`** — igual que en `Scatter`: el buffer local del proceso.
6. **`recvcount`** — aquí sí vuelve a ser **un solo número** (no un arreglo), porque cada proceso solo necesita saber cuánto le toca recibir **a sí mismo**; ese valor debe coincidir con el `sendcounts[mi_rank]` que puso el `root`.
7. **`recvtype`** — igual que en `Scatter`.
8. **`root`** — igual que en `Scatter`.
9. **`comm`** — igual que en `Scatter`.

**Ejemplo: repartir 100 clientes entre 3 sucursales sin que la división sea exacta**

```c
int counts[3] = {34, 33, 33};        // cuántos clientes le toca a cada sucursal
int displs[3] = {0, 34, 67};         // dónde empieza el bloque de cada una en la lista completa

int mi_cantidad = counts[rank];      // cuántos me tocan a mí
int *mi_pedazo = malloc(mi_cantidad * sizeof(int));

MPI_Scatterv(
    lista_completa,   // solo válido en el root
    counts,           // {34, 33, 33}
    displs,           // {0, 34, 67}
    MPI_INT,
    mi_pedazo,         // aquí cae mi trozo
    mi_cantidad,       // cuántos espero recibir
    MPI_INT,
    0,                 // la central (rank 0) es quien reparte
    MPI_COMM_WORLD
);
```

**En una frase:** `MPI_Scatter` es la circular donde todas las sucursales reciben paquetes del mismo tamaño; `MPI_Scatterv` es la versión donde la central puede armar paquetes de tamaño distinto para cada sucursal, indicando además de dónde exactamente saca cada uno (`displs`) dentro de la lista original.

---

## 3. Por qué existe MPI y por qué importa

Una sola computadora tiene un límite de memoria y de núcleos. Si el problema es enorme —simular el clima, el choque de galaxias, entrenar un modelo gigante— **hace falta repartirlo entre cientos o miles de nodos** de una supercomputadora, cada uno con su propia memoria física, conectados por red. MPI es el estándar (desde 1994) que permite escribir ese reparto de trabajo de forma **portable**: el mismo código corre igual en un clúster pequeño de oficina que en una de las supercomputadoras más grandes del mundo.

Esto contrasta con otro modelo de paralelismo, **OpenMP** (mencionado en el mismo capítulo), que es más parecido a *varios empleados trabajando en la misma sucursal, compartiendo el mismo archivador* (memoria compartida) — no necesitan mandarse cartas porque están en la misma oficina, pero por eso mismo no pueden escalar a mil sucursales distintas. De hecho, los programas más avanzados combinan ambos: **MPI entre sucursales + OpenMP dentro de cada sucursal**, para aprovechar tanto el clúster completo como cada máquina individual al máximo.

---

## Resumen en una tabla mental

1. **Cada proceso es una isla de memoria** (una sucursal con su propio archivador).
2. **Un rank identifica a cada isla**, y un comunicador agrupa las islas que pueden hablar entre sí.
3. **Send/Receive** es la carta con remitente, destinatario y contenido — hay que tener cuidado con el orden para no colgarse esperándose mutuamente.
4. **Las operaciones colectivas** (Broadcast, Reduce, Scatter, Gather, Barrier) son protocolos donde participa *todo el grupo a la vez*, y son más seguras y eficientes que reinventar la comunicación a mano.
5. **MPI existe para escalar**: repartir un problema gigante entre muchísimas máquinas físicas distintas, algo que no se puede lograr solo con hilos dentro de una misma computadora.
