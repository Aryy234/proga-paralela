

#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// Ejercicio 1 — Anillo de mensajes (Send/Recv básico)
// Cada proceso recibe de (rank-1), suma su rank y envía al (rank+1).
// El proceso 0 inicia el anillo con un valor inicial (por defecto 100)
// y al final verifica que el valor recibido coincide con la suma esperada.

int main(int argc, char** argv){
	MPI_Init(&argc, &argv);

	int nprocs, rank;
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (nprocs < 2) {
		if (rank == 0) std::cerr << "Se necesitan al menos 2 procesos.\n";
		MPI_Finalize();
		return 1;
	}

	// Valor inicial (puede pasarse como argumento al ejecutar)
	int inicial = 100;
	if (rank == 0 && argc > 1) {
		inicial = std::atoi(argv[1]);
	}

	// Compartir el valor inicial no es necesario: el rank 0 lo envía directamente.

	const int TAG = 1; // tag fijo; aquí no hay ambigüedad porque solo hay
					   // un tipo de mensaje circulando en el anillo.

	int prev = (rank - 1 + nprocs) % nprocs;
	int next = (rank + 1) % nprocs;

	MPI_Status status;

	if (rank == 0) {
		// Rank 0 inicia el anillo enviando el valor inicial al siguiente
		int valor_envio = inicial;
		MPI_Send(&valor_envio, 1, MPI_INT, next, TAG, MPI_COMM_WORLD);

		// Ahora espera a recibir el valor acumulado por el último proceso
		int valor_recibido = 0;
		MPI_Recv(&valor_recibido, 1, MPI_INT, prev, TAG, MPI_COMM_WORLD, &status);

		int valor_despues = valor_recibido + rank; // suma su propio rank (0)
		// Mostrar la línea solicitada (usamos el valor que habría enviado)
		std::cout << "Proceso " << rank << " recibió " << valor_recibido
				  << " y envía " << valor_despues << "\n";

		// Verificar que coincide con la suma esperada
		int suma_ranks = (nprocs * (nprocs - 1)) / 2; // suma 0..nprocs-1
		int esperado = inicial + suma_ranks;
		if (valor_despues == esperado) {
			std::cout << "Verificación OK: valor final = " << valor_despues << "\n";
		} else {
			std::cout << "Verificación FALLÓ: obtenido=" << valor_despues
					  << " esperado=" << esperado << "\n";
		}

	} else {
		// Procesos intermedios y último: reciben, suman y reenvían
		int valor_recibido = 0;
		MPI_Recv(&valor_recibido, 1, MPI_INT, prev, TAG, MPI_COMM_WORLD, &status);

		int valor_envio = valor_recibido + rank;
		std::cout << "Proceso " << rank << " recibió " << valor_recibido
				  << " y envía " << valor_envio << "\n";

		MPI_Send(&valor_envio, 1, MPI_INT, next, TAG, MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}