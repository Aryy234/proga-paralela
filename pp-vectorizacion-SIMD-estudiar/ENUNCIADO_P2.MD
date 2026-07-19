## 1. Objetivo
Implementar en C/C++ una versión del fractal Burning Ship sobre el plano complejo usando vectorización SIMD para acelerar el cálculo por bloques de píxeles, separando claramente la versión escalar de referencia y el kernel vectorizado. El programa debe mostrar la imagen en una interfaz gráfica con SFML y reportar el rendimiento obtenido.

## 2. Fractal Burning Ship
Se define por:

$z_{n+1}=(|Re(z_n)|+i|Im(z_n)|)^2+c$ , con $z_0=0$ y $c=x+y_i$

* Escape: $z_r^2+z_i^2>4$
* Color: rampa de 16 tonos, negro si no escapa
* Vista por defecto: `[-1.8, 1.7] x [-0.1, 0.05]`

### 2.1 Parámetros por defecto

| Parámetro | Default | Significado |
| :--- | :--- | :--- |
| WIDTH HEIGHT | `1600 x 900` | Tamaño de la imagen |
| `[xmin, xmax] [ymin, ymax]` | `[-1.8, 1.7] x [-0.1, 0.05]` | Plano complejo |
| `max_iter` | `100` | Tope del bucle |
| `NBINS` | `16` | Bins del histograma |
| Paleta | `color_ramp[16]` | 16 tonos |

## 3. Stack obligatorio
* `C/C++ (GCC >= 14)`
* `CMake >= 3.16`
* `SFML (graphics, window, system)`
* `fmt`
* `FreeType` con `arial_ttf.h`
* Extensión SIMD del compilador, preferiblemente AVX2 o la disponible en el equipo

## 4. Requerimientos funcionales
* `main`: coordina la aplicación, crea la ventana, administra eventos y llama a los kernels.
* Versión escalar: debe existir como referencia correcta para comparar la salida.
* Kernel SIMD: debe procesar varios píxeles por iteración usando intrínsecos o una abstracción equivalente.
* Descomposición: la imagen se procesa por filas o bloques contiguos de píxeles para facilitar cargas vectoriales.
* Remanentes: los píxeles que no completen el ancho del vector SIMD deben resolverse con un tramo escalar final.
* Histograma local: construir un `local_hist[16]` con `local_hist[iter * 16 / max_iter]++` por cada píxel que escape.
* Interfaz SFML: mostrar la imagen, los valores de `max_iter`, dominio, FPS, histograma y teclas de ayuda.
* Controles: `Up/Down` cambian `max_iter`, `Esc` cierra la aplicación.

## 5. Solución esperada

### 5.1 Estructura recomendada
* `main.cpp`: interfaz, eventos, render y medición.
* `params.h / params.cpp`: constantes y parámetros del fractal.
* `kernel_scalar.cpp`: cálculo de referencia.
* `kernel_simd.cpp`: cálculo vectorizado.
* `palette.cpp`: conversión de iteraciones a color.
* `utils.cpp`: apoyo para coordenadas, histograma y formateo.

### 5.2 Estrategia de cálculo
* Convertir cada píxel `(x, y)` a coordenada compleja `(c_re, c_im)`.
* Calcular el fractal con dos rutas:
  * Escalar: procesa un píxel por vez.
  * SIMD: procesa varios `x` de la misma fila en un solo bloque.
* Mantener una máscara de píxeles activos mientras iteran.
* Cuando un píxel excede el criterio de escape, guardar su iteración de salida y actualizar el histograma.
* Al final, convertir la cantidad de iteraciones en color usando la paleta de 16 tonos.

### 5.3 Idea del kernel SIMD
* Cargar varios valores de `x` en un registro vectorial.
* Reutilizar el mismo `y` para toda la fila.
* Inicializar `zr = 0` y `zi = 0`.
* En cada iteración:
  * aplicar valor absoluto por componente,
  * calcular `zr2` y `zi2`,
  * actualizar `zr` y `zi`,
  * evaluar escape con una comparación vectorial,
  * registrar qué lanes siguen activos.
* Terminar cuando todos los píxeles del bloque hayan escapado o se alcance `max_iter`.

### 5.4 Validación
* Comparar la imagen SIMD contra la versión escalar.
* Verificar bordes del dominio y remanentes.
* Probar con `WIDTH` pequeño y con tamaños grandes.
* Confirmar que el histograma sumado coincida con la cantidad de píxeles que escaparon.

## 6. Rúbrica propuesta

| # | Criterio | Pts |
| :--- | :--- | :--- |
| 1 | Inicialización correcta de la ventana, eventos y render con SFML | 2 |
| 2 | Implementación de la versión escalar correcta del Burning Ship | 3 |
| 3 | Kernel SIMD con procesamiento por bloques y uso de intrínsecos | 4 |
| 4 | Manejo correcto de remanentes y bordes | 3 |
| 5 | Histograma de 16 bins actualizado durante el cálculo | 2 |
| 6 | Overlay con `max_iter`, dominio, histograma y FPS | 3 |
| 7 | CMakeLists.txt correcto y compilación exitosa con optimización SIMD | 1 |
| 8 | Programa funcionando correctamente y salida visual coherente | 2 |
| | **Total** | **20** |

## 7. Entregables
Subir a TEAMS un archivo ZIP/RAR con nombre `prueba02_apellidol_apellido2_nombre.zip`, que contenga:
* Código fuente del proyecto
* `CMakeLists.txt`
* 1 captura de pantalla del programa ejecutándose

## 8. Restricciones
* No se receptarán proyectos fuera del horario establecido para el examen.
* No se aceptarán entregas por chat, correo electrónico u otro medio distinto a TEAMS.
* Exámenes con indicios de copia recibirán nota de 0.
* Estudiante al que se lo encuentre copiando recibirá nota de 0.
* Prohibido el uso de redes sociales durante el examen.
* Prohibido el uso de motores LLM antes, durante o después del examen.
* El uso de construcciones de C/C++ no vistas en clase implica nota de 0.

## 9. Resumen de la solución
La versión nueva del problema debe resolver el fractal Burning Ship sin MPI. La mejora paralela se logra aplicando SIMD al núcleo matemático, vectorizando el cálculo de varios píxeles por fila, dejando la interfaz y el postproceso fuera del kernel para mantener el código claro, verificable y fácil de comparar con la versión escalar.

