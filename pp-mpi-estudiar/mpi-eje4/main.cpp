// ============================================================================
// Ejercicio 4 - Estadisticas de un arreglo grande con MPI.
//
// Estrategia: el proceso 0 genera un arreglo de N numeros aleatorios y lo
// reparte entre todos los procesos con MPI_Scatterv (se usa la variante "v"
// porque N no siempre es multiplo exacto de la cantidad de procesos: los
// primeros procesos reciben un elemento extra para no perder datos). Cada
// proceso calcula el minimo, el maximo y la suma de su propia porcion, y
// esos resultados parciales se combinan con tres MPI_Reduce (uno por cada
// estadistica) en el proceso 0, que es quien imprime el resultado final.
// ============================================================================

#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>

// Genera "cantidad" numeros aleatorios de punto flotante entre 0 y 100.
double* generar_arreglo_aleatorio(int cantidad) {
    double* arreglo = new double[cantidad];
    for (int i = 0; i < cantidad; i++) {
        arreglo[i] = (rand() / static_cast<double>(RAND_MAX)) * 100.0;
    }
    return arreglo;
}

// Calcula el minimo, el maximo y la suma de un arreglo. Cada proceso lo usa
// sobre su propia porcion.
void calcular_min_max_suma(const double* arreglo, int cantidad,
                            double& minimo, double& maximo, double& suma) {
    minimo = arreglo[0];
    maximo = arreglo[0];
    suma = 0.0;
    for (int i = 0; i < cantidad; i++) {
        if (arreglo[i] < minimo) {
            minimo = arreglo[i];
        }
        if (arreglo[i] > maximo) {
            maximo = arreglo[i];
        }
        suma += arreglo[i];
    }
}

// Calcula cuantos elementos le toca procesar a cada proceso y en que
// posicion del arreglo original empieza su porcion. El resto de la division
// (cantidad_total % nprocs) se reparte dando un elemento extra a los
// primeros procesos, para que ningun dato quede sin repartir.
void calcular_conteos_y_desplazamientos(int cantidad_total, int nprocs,
                                         int* conteos, int* desplazamientos) {
    int elementos_por_proceso = cantidad_total / nprocs;
    int resto = cantidad_total % nprocs;

    int siguiente_desplazamiento = 0;
    for (int proceso = 0; proceso < nprocs; proceso++) {
        conteos[proceso] = elementos_por_proceso;
        if (proceso < resto) {
            conteos[proceso]++;
        }
        desplazamientos[proceso] = siguiente_desplazamiento;
        siguiente_desplazamiento += conteos[proceso];
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Cantidad de numeros a generar. Solo el rank 0 lee el argumento de
    // linea de comandos; los demas procesos reciben el valor por Bcast.
    int cantidad_total = 1000000;
    if (rank == 0 && argc > 1) {
        cantidad_total = atoi(argv[1]);
    }
    MPI_Bcast(&cantidad_total, 1, MPI_INT, 0, MPI_COMM_WORLD);

    double* arreglo_completo = nullptr;

    if (rank == 0) {
        srand(static_cast<unsigned int>(time(nullptr)));
        arreglo_completo = generar_arreglo_aleatorio(cantidad_total);
    }

    // Cada proceso calcula por su cuenta cuantos elementos le tocan: es un
    // calculo determinista a partir de cantidad_total y nprocs (que ya
    // conocen todos), asi que no hace falta un Bcast adicional para esto.
    int* conteos = new int[nprocs];
    int* desplazamientos = new int[nprocs];
    calcular_conteos_y_desplazamientos(cantidad_total, nprocs, conteos, desplazamientos);

    int cantidad_local = conteos[rank];
    double* porcion_local = new double[cantidad_local];

    MPI_Scatterv(arreglo_completo, conteos, desplazamientos, MPI_DOUBLE,
                 porcion_local, cantidad_local, MPI_DOUBLE,
                 0, MPI_COMM_WORLD);

    double minimo_local, maximo_local, suma_local;
    calcular_min_max_suma(porcion_local, cantidad_local,
                           minimo_local, maximo_local, suma_local);

    // Tres reducciones, una por estadistica, cada una con la operacion que
    // corresponde: el minimo global es el MPI_MIN de los minimos locales, el
    // maximo global es el MPI_MAX de los maximos locales, y la suma global
    // es el MPI_SUM de las sumas locales. Solo el rank 0 recibe los totales.
    double minimo_global, maximo_global, suma_global;
    MPI_Reduce(&minimo_local, &minimo_global, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&maximo_local, &maximo_global, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&suma_local, &suma_global, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double promedio_global = suma_global / cantidad_total;

        printf("Procesos utilizados:      %d\n", nprocs);
        printf("Cantidad de numeros:      %d\n", cantidad_total);
        printf("\n");
        printf("--- Resultado paralelo (MPI) ---\n");
        printf("Minimo:    %.4f\n", minimo_global);
        printf("Maximo:    %.4f\n", maximo_global);
        printf("Promedio:  %.4f\n", promedio_global);

        delete[] arreglo_completo;
    }

    delete[] porcion_local;
    delete[] conteos;
    delete[] desplazamientos;

    MPI_Finalize();
    return 0;
}
