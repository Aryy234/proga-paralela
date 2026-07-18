## 1. Objetivo
Con MPI, implementar en C/C++ el dibujo del fractal de Newton sobre el plano complejo, combinando MPI_Send/Recv con MPI_Bcast y MPI_Reduce, todo visualizado en una interfaz gráfica construida con SFML.

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
* Intel MPI  
* Librería fmt  
* FreeType con arial_ttf.h  

## 4. Requerimientos funcionales
* Rank 0: UI de la aplicación; ensambla la imagen completa; muestra menú con opciones Up/Down (max_iter), Esc (cerrar); en cada iteración envía  max_iter, x_min, x_max, y_min, y_max, running) utilizando comunicación colectiva.

* Ranks 1..nprocs-1: calculan la porción de la imagen correspondiente y la envían utilizando comunicación colectiva; miden tiempo que toma el dibujo y el total de iteraciones ejecutadas.

* Colectivas (todas desde rank 0): MPI_Reduce para determinar el máximo sobre tiempo de cálculo; MPI_Reduce para determinar la suma de las iteraciones realizadas en cada rank

* Overlay SFML (FreeType): esquina superior con RANK, max_iter, max_compute_ms, total_iters, FPS; esquina inferior con ayuda de teclado.

## 5. Rúbrica (20 puntos)

| # | Criterio | Pts |
| :--- | :--- | :--- |
| 1 | MPI_Init/MPI_Comm_rank/MPI_Comm_size/MPI_Finalize correctos | 1 |
| 2 | Descomposición ID con padding cuando HEIGHT% nprocs $!=0$ | 3 |
| 3 | Kernel Newton-Raphson (3 raíces, sin div/0,  constante en el código) | 4 |
| 4 | MPI_Send/MPI_Recv: rank 0 recibe WIDTH*delta uint32_t y ensambla | 4 |
| 5 | MPI_Bcast de {max_iter, $c_{\_}$, c_imag, running desde rank 0 | 3 |
| 6 | MPI_Reduce con MPI_MAX+MPI_SUM mostrado en overlay | 2 |
| 7 | CMakeLists.txt correcto; mpiexec -n 4 ejecuta la aplicación | 1 |
| 8 | Programa funcionando correctamente | 2 |
| | **Total** | **20** |
