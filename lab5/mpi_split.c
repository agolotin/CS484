#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

void main(int argc, char *argv[]) {
	setbuf(stdout, 0);
    MPI_Init(&argc, &argv);

	MPI_Comm comm1, comm2;

	int membershipKey, rank, nproc, pivot1, newrank1, newrank2, pivot2;
	
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	membershipKey = rank % 2;
	MPI_Comm_split(MPI_COMM_WORLD, membershipKey, rank, &comm1);
	MPI_Comm_rank(comm1, &newrank1);
	pivot1 = rank;
	MPI_Bcast(&pivot1, 1, MPI_INT, 0, comm1);
	printf("rank %d, newrank1 %d, pivot1 %d\n", rank, newrank1, pivot1);

	membershipKey = newrank1 % 2;
	MPI_Comm_split(comm1, membershipKey, newrank1, &comm2);
	MPI_Comm_rank(comm2, &newrank2);
	pivot2 = rank;
	MPI_Bcast(&pivot2, 1, MPI_INT, 0, comm2);
	printf("rank %d, newrank1 %d, pivot1 %d, newrank2 %d, pivot2 %d\n", 
			rank, newrank1, pivot1, newrank2, pivot2);

	MPI_Finalize();
}
