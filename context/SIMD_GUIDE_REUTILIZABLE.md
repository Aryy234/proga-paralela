# Guía Reutilizable para Proyectos de Vectorización SIMD

## Objetivo

Esta guía resume patrones de diseño, estilo de código y decisiones técnicas observadas en varios proyectos de programación paralela que usan vectorización mediante SIMD.

La intención no es copiar implementaciones, sino extraer principios reutilizables para construir nuevos proyectos desde cero de forma ordenada, verificable y adaptable al problema concreto.

## Idea central

SIMD significa procesar varios datos con una sola instrucción. En este tipo de proyectos, la estructura general suele seguir esta secuencia:

1. Definir una versión escalar correcta.
2. Identificar la parte del algoritmo que se repite sobre muchos elementos.
3. Agrupar esos elementos en bloques del ancho del vector SIMD.
4. Aplicar intrínsecos o una abstracción equivalente.
5. Manejar los elementos sobrantes con un tramo escalar.
6. Verificar que la salida sea equivalente a la versión base.
7. Medir rendimiento y comparar.

---

## 1. Arquitectura recomendada

### Componentes típicos

- `main`: coordina el programa, carga datos, lanza el kernel y muestra resultados.
- `kernel_scalar`: implementación secuencial de referencia.
- `kernel_simd`: implementación vectorizada.
- `kernel_parallel` o `kernel_openmp`: versión paralela alternativa, si se necesita comparación.
- `params` o `config`: constantes globales y parámetros del problema.
- `utils` o `palette`: funciones de apoyo reutilizables.
- `bridge` o `wrapper`: capa de enlace si el kernel SIMD se expone desde una biblioteca nativa.

### Relación entre módulos

- El `main` no debe contener toda la lógica del problema.
- El kernel SIMD debe estar encapsulado en funciones pequeñas y bien definidas.
- Las constantes compartidas deben centralizarse.
- Si hay interfaz gráfica o interacción con el usuario, debe estar separada del cómputo pesado.

### Organización práctica

Una estructura saludable suele verse así:

```text
project/
  main.*
  kernel_scalar.*
  kernel_simd.*
  kernel_openmp.*   (opcional)
  params.*
  utils.*
  CMakeLists.txt / build.gradle / etc.
```

---

## 2. Estilo y convenciones de programación

### Convenciones frecuentes

- Nombres descriptivos y funcionales.
- Variables de coordenadas o índices con nombres cortos pero claros, como `i`, `j`, `x`, `y`, `idx`.
- Parámetros globales agrupados en una sola estructura o archivo.
- Funciones cortas, cada una con una responsabilidad.
- Comentarios explicativos, especialmente cuando el código SIMD no es obvio.

### Tipos de datos comunes

- `uint8_t` para píxeles o canales de color.
- `uint32_t` para buffers de imagen empaquetados.
- `double` cuando se busca precisión en la parte escalar.
- `float` cuando la versión SIMD prioriza rendimiento.
- Tipos vectoriales del compilador o intrínsecos, como registros de 128 o 256 bits.

### Organización de parámetros

- Límites del dominio de entrada.
- Dimensiones del problema.
- Número máximo de iteraciones.
- Constantes del modelo.
- Tamaño de la paleta o tabla de resultados.

### Formato e indentación

- Estilo consistente dentro de todo el proyecto.
- Bloques cortos y legibles.
- Comentarios colocados antes de una sección importante, no sobre cada línea.

### Gestión de errores

- Validar que los datos de entrada existan y tengan el tamaño esperado.
- Comprobar remanentes de bucles.
- Proteger accesos fuera de rango.
- En proyectos gráficos o de archivos, verificar que la carga y escritura funcionen.

---

## 3. Lógica de programación común

### Patrón general

La mayoría de estos proyectos siguen una lógica parecida:

1. Calcular derivaciones o pasos del dominio.
2. Recorrer el conjunto de datos.
3. Transformar o evaluar cada elemento.
4. Guardar el resultado.
5. Si el problema lo permite, comparar varias estrategias de ejecución.

### Patrones repetidos

- Doble bucle para recorrer una matriz o imagen.
- Reducción sobre vectores.
- Iteración matemática repetitiva.
- Mapeo entre coordenadas del problema y coordenadas del buffer.
- Separación entre cálculo y presentación.

### Estructura algorítmica deseable

- Primero una versión simple y correcta.
- Después una versión vectorizada.
- Después, si aplica, una versión paralela por hilos.
- Al final, una medición comparativa.

---

## 4. Implementación SIMD

## 4.1 Carga y almacenamiento

- Usar cargas vectoriales cuando los datos estén contiguos y bien alineados.
- Usar cargas no alineadas si la simplicidad o el layout real lo exigen.
- Guardar resultados en registros temporales cuando la conversión final no pueda hacerse directamente.

### Patrones comunes

- Cargar bloques enteros de elementos con una sola instrucción.
- Procesar varios valores homogéneos al mismo tiempo.
- Volcar registros a un arreglo temporal para terminar el trabajo de forma escalar.

## 4.2 Intrínsecos y abstracciones

Los intrínsecos más comunes en este tipo de proyectos son:

- Operaciones aritméticas:
  - suma
  - resta
  - multiplicación
- Comparaciones vectoriales.
- Creación de vectores constantes por broadcast.
- Máscaras para seleccionar o descartar elementos.
- Extracción de resultados a memoria.

### Idea práctica

La parte SIMD suele concentrarse en:

- generar valores de entrada,
- realizar el cálculo principal,
- detectar condiciones de salida,
- almacenar resultados intermedios.

La parte escalar suele concentrarse en:

- manejo de remanentes,
- conversión final de tipos,
- indexación de salida,
- selección de color o postproceso.

## 4.3 Organización de datos

La vectorización funciona mejor cuando:

- los datos están en memoria contigua,
- los elementos tienen tipo homogéneo,
- el problema se puede dividir en bloques del mismo tamaño,
- hay poca dependencia entre elementos consecutivos.

### Buenas estrategias

- Usar buffers lineales.
- Evitar estructuras complejas dentro del núcleo SIMD.
- Preparar los datos antes del bucle vectorizado si eso simplifica el kernel.

## 4.4 Alineación de memoria

Dos enfoques aparecen mucho:

- usar almacenamiento/carga no alineada para simplificar,
- o declarar buffers con alineación explícita cuando el caso lo justifica.

### Regla práctica

Si no existe una garantía fuerte de alineación, es mejor usar instrucciones seguras para acceso no alineado y mantener el código estable.

## 4.5 Elementos restantes

Siempre conviene tener un tramo escalar final para:

- datos que no completan el ancho del vector,
- bordes de imagen,
- tamaños no múltiplos del bloque SIMD,
- casos especiales que no vale la pena vectorizar.

## 4.6 Reducción de bifurcaciones

SIMD funciona mejor con pocas ramas.

### Estrategias útiles

- Convertir condiciones en máscaras.
- Desplazar decisiones fuera del bucle principal cuando sea posible.
- Agrupar cálculos comunes.
- Minimizar `if` dentro del tramo vectorizado.

### En problemas iterativos

Cuando cada elemento puede terminar en momentos distintos, se usa:

- máscara de elementos activos,
- actualización condicional,
- verificación de salida global para cortar el bucle cuando todo ha terminado.

## 4.7 Qué se vectoriza y qué no

### Normalmente se vectoriza

- la aritmética intensiva,
- el cálculo repetitivo sobre bloques de datos,
- la evaluación de fórmulas homogéneas.

### Normalmente permanece escalar

- la escritura final si requiere conversiones complejas,
- el manejo de color o formato,
- la lógica de UI,
- las excepciones o validaciones,
- el remanente de elementos no completados.

## 4.8 Estrategias de rendimiento

- Procesar varios elementos por iteración.
- Reusar constantes vectoriales.
- Evitar ramas dentro del bucle principal.
- Mantener el kernel compacto.
- Separar medición del cómputo principal.
- Usar la versión escalar como referencia de corrección.

---

## 5. Compilación, pruebas y rendimiento

### Sistema de compilación

Los proyectos SIMD suelen usar alguno de estos enfoques:

- CMake en C++.
- Gradle en Java con una biblioteca nativa para el núcleo SIMD.

### Configuración típica

- Activar extensión SIMD del compilador, por ejemplo AVX o AVX2.
- Activar optimización.
- Vincular dependencias gráficas o de soporte si el proyecto las necesita.

### Verificación funcional

- Comparar resultado SIMD contra versión escalar.
- Revisar visualmente la salida si es una imagen o fractal.
- Probar casos pequeños y casos grandes.
- Verificar bordes y residuos.

### Medición de rendimiento

- usar `chrono` o un contador similar para tiempo total,
- comparar FPS en aplicaciones interactivas,
- ejecutar sobre tamaños de entrada representativos,
- repetir varias veces si el resultado depende de variación dinámica.

---

## 6. Comparación de patrones por tipo de problema

## 6.1 Reducción lineal

Ejemplos conceptuales:

- producto escalar,
- suma acumulada,
- combinación de dos vectores.

### Rasgos

- ideal para SIMD,
- control de flujo mínimo,
- remanente fácil de manejar,
- reducción final manual.

## 6.2 Transformación por elemento

Ejemplos conceptuales:

- filtros de imagen,
- conversión de color,
- operaciones por píxel.

### Rasgos

- se agrupan varios elementos homogéneos,
- el layout de datos importa mucho,
- a veces hay que reempaquetar datos manualmente,
- el postproceso puede seguir siendo escalar.

## 6.3 Algoritmos iterativos con escape

Ejemplos conceptuales:

- fractales,
- simulaciones de evolución por punto,
- cálculos con condición de salida variable.

### Rasgos

- vectorización más difícil,
- uso de máscaras,
- divergencia entre elementos,
- el kernel SIMD requiere más cuidado.

---

## 7. Diferencias importantes que conviene entender

### SIMD no es lo mismo que:

- **OpenMP**: paralelismo por hilos.
- **MPI**: paralelismo por procesos.
- **CUDA**: cómputo en GPU.
- **DLL**: formato de biblioteca, no técnica de paralelismo.

### Relación entre conceptos

- SIMD puede vivir dentro de un programa normal, una DLL o una librería.
- Una DLL puede contener código SIMD, pero la DLL no define la técnica.
- Un proyecto Java puede usar SIMD de forma indirecta a través de una biblioteca nativa.

---

## 8. Buenas prácticas que sí conviene conservar

- Crear primero una versión correcta y fácil de entender.
- Separar la lógica en archivos distintos.
- Mantener una interfaz clara entre el código vectorizado y el resto del sistema.
- Usar buffers contiguos.
- Incluir un caso residual para elementos sobrantes.
- Medir tiempo o FPS.
- Documentar los supuestos del kernel SIMD.
- Mantener una versión de referencia para pruebas.

---

## 9. Errores o limitaciones que conviene evitar

- Mezclar UI, lectura de archivos y kernel numérico en la misma función.
- No validar tamaños de entrada.
- Suponer que los datos siempre son múltiplos del ancho SIMD.
- Introducir demasiadas ramas dentro del bucle vectorizado.
- No comprobar la equivalencia con una versión escalar.
- Repetir la misma lógica en varios archivos sin necesidad.
- No dejar claro qué parte del problema está vectorizada y cuál no.

---

## 10. Plantilla reutilizable para futuros problemas

Cuando aparezca un nuevo problema, el proceso recomendado es:

1. Definir el objetivo numérico o de transformación.
2. Elegir el formato de datos de entrada y salida.
3. Diseñar primero una solución escalar.
4. Identificar el bloque repetitivo apto para SIMD.
5. Elegir el ancho vectorial según hardware y tipo de dato.
6. Implementar el kernel SIMD.
7. Manejar remanentes y bordes.
8. Validar contra la versión escalar.
9. Medir rendimiento.
10. Ajustar la organización del proyecto si se necesita interfaz, archivos o integración externa.

### Estructura mínima sugerida

```text
src/
  main.*
  params.*
  kernel_scalar.*
  kernel_simd.*
  utils.*
```

### Si hay interfaz gráfica o archivos

Agregar módulos separados para:

- carga de datos,
- presentación,
- exportación de resultados.

### Si hay integración nativa

Agregar:

- una biblioteca nativa,
- una interfaz de enlace simple,
- buffers directos o contiguos,
- documentación de cómo llamarla desde el lenguaje principal.

---

## 11. Regla final de diseño

No copiar una solución anterior de forma literal.

Lo correcto es:

- identificar el patrón útil,
- adaptar el patrón al problema nuevo,
- respetar el layout real de datos,
- respetar el lenguaje y el compilador,
- elegir una estrategia SIMD acorde al tipo de cómputo.

Contexto de estilo y convenciones de código (usar en todas las respuestas):

- Idioma: escribe todo en español (comentarios, mensajes, explicaciones breves).
- Estilo general: imperativo y directo. Código claro y didáctico, pensado para estudiantes/práctica.
- Nombres: variables y funciones en español, descriptivos y completos (por ejemplo: filas_local, sendcounts, displs, cantidad_local, scatter_recvbuf). Evitar acrónimos confusos.
- Tipos y contenedores: uso preferente de arreglos C explícitos (`int*` con `new[]/delete[]`) para ejemplos sencillos; si se usa STL, hacerlo sólo cuando aporte claridad.
- Bucles: usar `for` explícitos con índices (`for (int i = 0; i < n; i++)`), evitar programación funcional (lambdas, std::transform) en ejemplos didácticos.


Esa es la diferencia entre reutilizar conocimiento y repetir código.

