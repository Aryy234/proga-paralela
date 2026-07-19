## 1. Objetivo
Con OpenMP, implementar en C/C++ el dibujo del fractal de Newton sobre el plano complejo, paralelizando el cálculo por filas y visualizando el resultado en una interfaz gráfica construida con SFML.

## 2. Fractal de Newton
Newton-Raphson sobre $z^{3}-1=0$ con $z_{0}=x+iy$:

$z_{n+1}=z_{n}-\frac{z_{n}^{3}-1}{3~z_{n}^{2}}.$


La iteración converge a una de las 3 raíces cúbicas de la unidad:

$w_{0}=1$, $w_{1}=-\frac{1}{2}+i\frac{\sqrt{3}}{2}.$ $w_{2}=-\frac{1}{2}-i\frac{\sqrt{3}}{2}$


* Convergencia: $|z_{n}-w_{k}|<\epsilon~con~\epsilon=10^{-4}.$

* Escape: $|z_{n}|>2$

* Color: una paleta color_ramp [16]. Cada pixel utiliza color_ramp $[(root*5+iter)$ % 16], donde root = 0, 1, 2 es el índice de la raíz a la que converge $(w_{0},w_{1},w_{2})$; negro si no converge o si $|z_{n}|>2.$

### 2.1 Parámetros por defecto

| Parámetro | Default | Significado |
| :--- | :--- | :--- |
| WIDTH HEIGHT | $1600\times900$ | Tamaño de la imagen |
| [xmin, xmax] [ymin, ymax] | [ 1.5,1.5]\times[-1.0,1.0] | plano complejo |
| max_iter | 50 | tope del bucle |
| ε | 1e-4 | convergencia por raíz |
| Paleta | color_ramp [16] | 16 tonos |

## 3. Stack obligatorio
* $C/C++(GCC\ge14)$  
* CMake  3.16  
* SFML (graphics, window, system)  
* OpenMP  
* Librería fmt  
* FreeType con arial_ttf.h  

## 4. Requerimientos funcionales
* Una única aplicación con UI en SFML.
* El cálculo del fractal se paraleliza con OpenMP por filas o bloques de filas.
* `Up/Down` modifican `max_iter` en tiempo real.
* `Esc` cierra la aplicación.
* Overlay SFML (FreeType): esquina superior con `OMP_THREADS`, `max_iter`, `max_compute_ms`, `total_iters`, `FPS`; esquina inferior con ayuda de teclado.

## 5. Rúbrica (20 puntos)

| # | Criterio | Pts |
| :--- | :--- | :--- |
| 1 | Inicialización y uso correcto de OpenMP | 1 |
| 2 | Descomposición ID con padding cuando HEIGHT% nprocs $!=0$ | 3 |
| 3 | Kernel Newton-Raphson (3 raíces, sin div/0,  constante en el código) | 4 |
| 4 | Paralelización por filas con OpenMP y ensamblado correcto de la imagen | 4 |
| 5 | Ajuste interactivo de `max_iter` y refresco de parámetros | 3 |
| 6 | Reducción de métricas (`max_compute_ms` y `total_iters`) mostrada en overlay | 2 |
| 7 | CMakeLists.txt correcto; ejecución con OpenMP en una sola app | 1 |
| 8 | Programa funcionando correctamente | 2 |
| | **Total** | **20** |
