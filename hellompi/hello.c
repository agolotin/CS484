#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


main(int argc, char *argv[]) 
{
	int iproc, nproc,i;
	char host[255], message[55];
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc); //get number of processors
	MPI_Comm_rank(MPI_COMM_WORLD, &iproc); //get rank in the group of processors

	gethostname(host,253);
	printf("I am proc %d of %d running on %s\n", iproc, nproc,host);

	printf("Sending messages\n");
	sprintf(message, "%d: Hello\n", iproc);
	for (i = 0; i < nproc; i++)
	{
		if (i != iproc)
			MPI_Send(message, 35, MPI_CHAR, i, 0, MPI_COMM_WORLD);
	}
	printf("Receiving messages\n");
	for (i = 0; i < nproc; i++)
	{
		if (i != iproc)
		{
			MPI_Recv(message, 35, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
			printf("%d: %s",iproc, message);
		}
	}

	MPI_Finalize(); //has to be called
}
