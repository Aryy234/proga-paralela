// ============================================================================
// Ejercicio 9 - Suma de dos matrices por filas con MPI.
//
// Estrategia: el proceso 0 genera dos matrices cuadradas A y B de tamaño
// N x N (N multiplo de la cantidad de procesos, para que el reparto por
// filas sea parejo). Reparte A y B por bloques de filas con dos llamadas
// separadas a MPI_Scatter (una por matriz). Cada proceso suma elemento a
// elemento las filas que le tocaron (C_local = A_local + B_local) y el
// resultado se junta de vuelta en el proceso 0 con MPI_Gather, formando la
// matriz C completa.
//
// Por que dos Scatter en vez de uno: MPI_Scatter reparte un solo bloque de
// memoria contiguo. Para repartir A y B en una sola llamada habria que
// armar a mano un buffer combinado (por ejemplo, poniendo todo A seguido de
// todo B, o intercalando sus filas) y luego separar ese buffer otra vez
// dentro de cada proceso. Con dos llamadas separadas cada matriz viaja con
// su propio buffer, sin ese paso extra de armar y desarmar.
//
// Convencion: las matrices se representan como arreglos 1D en row-major
// (el elemento (fila, columna) esta en datos[fila * N + columna]), que es
// el formato que espera MPI_Scatter/MPI_Gather al repartir bloques de filas
// contiguos en memoria.
// ============================================================================

#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>

// Genera una matriz N x N (row-major) con valores aleatorios enteros entre
// 0 y 9 (numeros enteros para que la suma se pueda verificar sin
// preocuparse por errores de redondeo de punto flotante).
double* generar_matriz_aleatoria(int n) {
    double* matriz = new double[n * n];
    for (int i = 0; i < n * n; i++) {
        matriz[i] = rand() % 10;
    }
    return matriz;
}

// Suma dos matrices row-major del mismo tamaño, elemento a elemento.
// "filas" es la cantidad de filas a sumar y "columnas" el ancho de cada fila.
void sumar_matrices(const double* a, const double* b, double* resultado,
                     int filas, int columnas) {
    int total_elementos = filas * columnas;
    for (int i = 0; i < total_elementos; i++) {
        resultado[i] = a[i] + b[i];
    }
}

// Imprime una matriz N x N (row-major) con un titulo antes.
void imprimir_matriz(const char* titulo, const double* matriz, int n) {
    printf("%s\n", titulo);
    for (int fila = 0; fila < n; fila++) {
        for (int columna = 0; columna < n; columna++) {
            printf("%5.1f ", matriz[fila * n + columna]);
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Tamaño de las matrices (N x N). Solo el rank 0 lee el argumento de
    // linea de comandos; los demas procesos reciben el valor por Bcast.
    int n = 4;
    if (rank == 0 && argc > 1) {
        n = atoi(argv[1]);
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // El enunciado permite asumir que N es multiplo de la cantidad de
    // procesos para simplificar el reparto por filas. En vez de repartir
    // mal en silencio si no se cumple, se valida y se aborta con un
    // mensaje claro.
    if (n % nprocs != 0) {
        if (rank == 0) {
            printf("Error: N (%d) debe ser multiplo de la cantidad de procesos (%d)\n", n, nprocs);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int filas_por_proceso = n / nprocs;

    double* matriz_a = nullptr;
    double* matriz_b = nullptr;

    if (rank == 0) {
        srand(static_cast<unsigned int>(time(nullptr)));
        matriz_a = generar_matriz_aleatoria(n);
        matriz_b = generar_matriz_aleatoria(n);
    }

    // Cada proceso recibe "filas_por_proceso" filas completas de A y de B.
    // Una fila tiene "n" columnas, asi que cada proceso recibe
    // filas_por_proceso * n numeros de cada matriz.
    int elementos_por_proceso = filas_por_proceso * n;
    double* a_local = new double[elementos_por_proceso];
    double* b_local = new double[elementos_por_proceso];

    MPI_Scatter(matriz_a, elementos_por_proceso, MPI_DOUBLE,
                a_local, elementos_por_proceso, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    MPI_Scatter(matriz_b, elementos_por_proceso, MPI_DOUBLE,
                b_local, elementos_por_proceso, MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    double* c_local = new double[elementos_por_proceso];
    sumar_matrices(a_local, b_local, c_local, filas_por_proceso, n);

    double* matriz_c = nullptr;
    if (rank == 0) {
        matriz_c = new double[n * n];
    }

    MPI_Gather(c_local, elementos_por_proceso, MPI_DOUBLE,
               matriz_c, elementos_por_proceso, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        imprimir_matriz("Matriz A:", matriz_a, n);
        imprimir_matriz("Matriz B:", matriz_b, n);
        imprimir_matriz("Matriz C = A + B (calculada en paralelo):", matriz_c, n);

        delete[] matriz_a;
        delete[] matriz_b;
        delete[] matriz_c;
    }

    delete[] a_local;
    delete[] b_local;
    delete[] c_local;

    MPI_Finalize();
    return 0;
}
