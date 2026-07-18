## 1. Objetivo
Con MPI, implementar en C/C++ el dibujo del fractal Burning Ship sobre el plano complejo, combinando MPI_Send/Recv con MPI_Bcast para parámetros y MPI_Gather para recolectar un histograma de iteraciones, todo visualizado en una interfaz gráfica construida con SFML.

## 2. Fractal Burning Ship
Se define por:

$z_{n+1}=(|Re(z_{n})|+i|Im(z_{n})|)^{2}+c$ $z_{0}=0$, $c=x+y_{i}$

* Escape: $z_{r}^{2}+z_{i}^{2}>4.$
* Color: Ramp de 16 tonos (paleta de colores), negro si no escapa.
* Vista por defecto: [-1.8, 1.7] x [-0.1, 0.05] (zona visible del "barco ardiente").

### 2.1 Parámetros por defecto

| Parámetro | Default | Significado |
| :--- | :--- | :--- |
| WIDTH HEIGHT | $1600\times900$ | Tamaño de la imagen |
| [xmin, xmax] [ymin, ymax] | [-1.8 $,-1.7]\times[-0.1,0.05]$ | Plano complejo |
| max_iter | 100 | tope del bucle |
| NBINS | 16 | bins del histograma |
| Paleta | color_ramp [16] | 16 tonos |

## 3. Stack obligatorio
* • $C/C++(GCC\ge14)$
* CMake ≥ 3.16
* • SFML (graphics, window, system)
* Intel MPI
* • Librería fmt
* FreeType con arial_ttf.h

## 4. Requerimientos funcionales
* • Rank 0: UI de la aplicación; ensambla la imagen completa; muestra menú con opciones Up/Down (max_iter), Esc (cerrar); en cada iteración envía {max_iter, x_min, x_max, y_min, y_max, running) utilizando comunicación colectiva.
* Ranks 1..nprocs-1: calculan la porción de la imagen correspondiente y la envían utilizando comunicación colectiva; construyen un histograma local local_hist [16] con local_hist [iter*16/max_iter] ++ por cada píxel que escapó; Envían el histograma utilizando comuniación colectiva;.
* • Colectivas (todas desde rank 0): MPI_Gather del histograma local -> global_hist [16] mostrado en la pantalla.
* • Overlay SFML (FreeType): esquina superior con RANKs, número de iteraciones, dominio, global_hist [0..15], FPS; esquina inferior con ayuda de teclado.

Página 1 de 2

UNIVERSIDAD CENTRAL DEL ECUADOR  
CARRERA DE COMPUTACIÓN  

## 5. Entregables
Subir a TEAMS un archivo ZIP/RAR con nombre prueba02_apellidol_apellido2_nombre.zip, el archivo debe contener:
* • Código fuente del proyecto
* 1 captura de pantalla del programa ejecutándose

## 6. Rúbrica (20 puntos)

| # | Criterio | Pts |
| :--- | :--- | :--- |
| 1 | MPI_Init/MPI_Comm_rank/MPI_Comm_size/MPI_Finalize correctos | 1 |
| 2 | Descomposición ID por filas con padding cuando HEIGHT % nprocs $!=0$ | 3 |
| 3 | Kernel Burning Ship con valor absoluto por componente y escape $||z||>2$ | 3 |
| 4 | MPI_Send/MPI_Recv: rank 0 recibe WIDTH*delta y ensambla la imagen completa | 4 |
| 5 | MPI_Bcast de {max_iter, x_min, x_max, y_min, y_max, running desde rank 0 | 3 |
| 6 | Histograma local de 16 bins + MPI_Gather a rank 0, mostrado en overlay | 3 |
| 7 | CMakeLists.txt correcto; mpiexec -n 4 ejecuta la aplicación | 1 |
| 8 | Programa funcionando correctamente | 2 |
| | **Total** | **20** |

## 7. Restricciones
* • No se receptarán proyectos fuera del horario establecido para el examen.
* • No se aceptarán entregas a través de chats, correo electrónico, etc. El único medio de entrega es la plataforma TEAMS.
* Exámenes en los que existan indicios de copia recibirán la nota de 0.
* Estudiante al que se lo encuentre copiando recibirá la nota de 0.
* Prohibido el uso de cualquier red social durante el examen.
* Prohibido el uso de motores LLM (ChatGPT u otros) antes, durante o después del examen.
* El uso de construcciones c/c++ no vistas en las clases implica una nota de 0 (cero).

Página 2 de 2