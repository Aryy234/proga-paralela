## 1. Objetivo
Implementar en C/C++ el dibujo del fractal Burning Ship sobre el plano complejo usando exclusivamente vectorización SIMD.

El proyecto debe incluir una versión escalar de referencia y una versión SIMD equivalente, de forma que ambas produzcan la misma imagen final salvo por diferencias numéricas mínimas permitidas por la precisión de punto flotante.

---

## 2. Fractal Burning Ship

El fractal se define a partir de la iteración:

```text
z_{n+1} = (|Re(z_n)| + i|Im(z_n)|)^2 + c
```

donde:

```text
z_0 = 0
```

y `c = x + iy` representa cada punto del plano complejo asociado a un píxel de la imagen.

### 2.1 Criterio de escape

- Un punto escapa cuando `|z_n| > 2`.
- Si el punto no escapa antes de alcanzar `max_iter`, se considera interior para el color final.

### 2.2 Color

- Cada píxel se colorea a partir del número de iteraciones necesarias para escapar.
- Se usará la paleta `color_ramp[16]`.
- La asignación de color debe ser estable y determinista.
- Si un punto no escapa, se pinta en negro.

### 2.3 Parámetros por defecto

| Parámetro | Default | Significado |
| :--- | :--- | :--- |
| WIDTH HEIGHT | `1600 x 900` | Tamaño de la imagen |
| `[xmin, xmax] x [ymin, ymax]` | `[-2.2, 1.2] x [-2.0, 1.2]` | Plano complejo |
| `max_iter` | `100` | Tope del bucle |
| Paleta | `color_ramp[16]` | 16 tonos |

---

## 3. Stack obligatorio

- C/C++ con GCC 14 o superior
- CMake 3.16
- SFML para ventana, eventos y renderizado
- `fmt` para formateo de texto
- SIMD mediante intrínsecos del compilador, preferiblemente AVX2
- Fuente Arial embebida con `arial_ttf.h`

---

## 4. Requerimientos funcionales

- `main`: gestiona la ventana, los eventos, la actualización del fractal y el overlay.
- `kernel_scalar`: calcula la imagen completa como referencia correcta.
- `kernel_simd`: calcula la misma imagen usando SIMD.
- El programa debe permitir alternar entre modo escalar y modo SIMD para comparar resultados.
- El programa debe medir el tiempo de cálculo de cada modo.
- El overlay debe mostrar `modo`, `max_iter`, `tiempo_ms` y `FPS`.
- `Up` y `Down` deben modificar `max_iter`.
- `Esc` debe cerrar la aplicación.

---

## 5. Reglas de vectorización

- El kernel SIMD debe procesar bloques contiguos de píxeles.
- Debe existir un tramo escalar para los elementos restantes cuando el ancho de vector no complete la fila.
- La lógica del fractal debe mantenerse equivalente en ambas versiones.
- No se permite usar MPI ni distribución por procesos.
- No se permite delegar el cómputo principal a hilos; el foco del proyecto es SIMD.
- La implementación debe evitar mezclar UI y núcleo numérico en la misma función.

---

## 6. Rúbrica

| # | Criterio | Pts |
| :--- | :--- | :--- |
| 1 | `kernel_scalar` correcto y claro | 3 |
| 2 | `kernel_simd` equivalente al escalar | 5 |
| 3 | Uso real de SIMD con bloques vectoriales y manejo de remanentes | 4 |
| 4 | Overlay con tiempo, modo y FPS | 2 |
| 5 | Controles `Up`, `Down` y `Esc` | 2 |
| 6 | Paleta de 16 colores y negro para puntos interiores | 2 |
| 7 | CMake correcto y compilación exitosa | 1 |
| 8 | Funcionamiento general estable | 1 |
| | **Total** | **20** |

---

## 7. Criterios de calidad

- El código debe ser legible y didáctico.
- Las constantes del problema deben estar centralizadas.
- Las funciones deben tener una sola responsabilidad.
- Debe ser fácil comparar la versión escalar con la versión SIMD.
- La salida visual debe ser consistente entre ejecuciones.

---

## 8. Entrega esperada

El proyecto final debe incluir:

- una implementación escalar de referencia,
- una implementación SIMD funcional,
- una interfaz gráfica con SFML,
- medición de rendimiento,
- documentación básica de compilación y ejecución.

