// ============================================================================
// Estimación de Pi con el método de Monte Carlo, paralelizado con MPI.
//
// Idea del algoritmo:
//   Generamos puntos aleatorios dentro de un cuadrado de lado 2 (de -1 a 1
//   en x e y) y contamos cuántos caen dentro del círculo unitario inscrito
//   en ese cuadrado. La proporción (puntos dentro del círculo / puntos
//   totales) se aproxima a pi/4, así que multiplicándola por 4 obtenemos
//   una estimación de pi. Cuantos más puntos generemos, mejor la precisión.
//
// Por qué es un buen ejemplo de MPI:
//   - Cada proceso puede generar y evaluar su propia porción de puntos de
//     forma totalmente independiente (no necesitan datos de otros procesos
//     mientras calculan), así que el trabajo se reparte de forma pareja.
//   - Al final solo se necesita combinar UN número por proceso (su conteo
//     parcial), lo cual es el caso ideal para una reducción (MPI_Reduce).
//   - No requiere matrices, imágenes ni estructuras de datos complejas:
//     el foco queda puesto en Init/Finalize, rank/size, Bcast y Reduce.
// ============================================================================

#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>

int main(int argc, char** argv)
{
    // Todo programa MPI arranca con Init y termina con Finalize.
    // Entre medio es donde vive la lógica paralela.
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // ¿quién soy yo? (0..nprocs-1)
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs); // ¿cuántos procesos hay en total?

    // Número total de puntos a generar. Solo el proceso 0 lee el argumento
    // de línea de comandos (si el usuario lo pasó); los demás procesos no
    // tienen acceso directo a argv en todos los lanzadores de MPI, así que
    // NO deben leerlo por su cuenta.
    long long total_puntos = 100000000; // 100 millones, valor por defecto
    if (rank == 0 && argc > 1) {
        total_puntos = atoll(argv[1]);
    }

    // Broadcast: el rank 0 "reparte" el mismo valor de total_puntos a todos
    // los demás procesos en una sola llamada colectiva. Sin esto, cada
    // proceso se quedaría con su propio valor por defecto y podrían acabar
    // trabajando con cantidades distintas sin darse cuenta.
    MPI_Bcast(&total_puntos, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    // Reparto del trabajo: cada proceso genera una parte de los puntos.
    // Si total_puntos no es múltiplo exacto de nprocs, el rank 0 se queda
    // con el resto para que no se pierdan puntos por el redondeo.
    long long puntos_por_proceso = total_puntos / nprocs;
    if (rank == 0) {
        puntos_por_proceso += total_puntos % nprocs;
    }

    // Semilla distinta por proceso. Si todos usaran la misma semilla,
    // generarían exactamente la misma secuencia de números aleatorios y
    // el resultado combinado equivaldría a un solo proceso repitiendo su
    // trabajo N veces, no a un muestreo real más grande.
    unsigned int semilla = static_cast<unsigned int>(time(nullptr)) + rank * 7919u;
    srand(semilla);

    double inicio = MPI_Wtime(); // reloj de MPI, apto para medir tiempos paralelos

    long long puntos_dentro_local = 0;
    for (long long i = 0; i < puntos_por_proceso; i++) {
        double x = (rand() / static_cast<double>(RAND_MAX)) * 2.0 - 1.0; // rango [-1, 1]
        double y = (rand() / static_cast<double>(RAND_MAX)) * 2.0 - 1.0;
        if (x * x + y * y <= 1.0) {
            puntos_dentro_local++;
        }
    }

    // Reducción: cada proceso aporta su conteo parcial y MPI_Reduce los
    // suma todos en una sola operación colectiva, dejando el total
    // combinado únicamente en el rank 0 (destino indicado en el último
    // parámetro antes del comunicador).
    long long puntos_dentro_total = 0;
    MPI_Reduce(&puntos_dentro_local, &puntos_dentro_total, 1, MPI_LONG_LONG,
               MPI_SUM, 0, MPI_COMM_WORLD);

    double fin = MPI_Wtime();

    // Solo el rank 0 recibió el total combinado, así que solo él imprime
    // el resultado final. Los demás procesos no tienen nada útil que reportar.
    if (rank == 0) {
        double pi_estimado = 4.0 * static_cast<double>(puntos_dentro_total)
                                  / static_cast<double>(total_puntos);

        printf("Procesos utilizados:        %d\n", nprocs);
        printf("Puntos totales generados:   %lld\n", total_puntos);
        printf("Puntos dentro del circulo:  %lld\n", puntos_dentro_total);
        printf("Estimacion de Pi:           %.6f\n", pi_estimado);
        printf("Valor real de Pi:           3.141593\n");
        printf("Tiempo transcurrido:        %.4f segundos\n", fin - inicio);
    }

    MPI_Finalize();
    return 0;
}
