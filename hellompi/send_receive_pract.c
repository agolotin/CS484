#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


int main(int argc, char *argv[]) 
{
	int taskid, numtasks, destid, srcid, recval;
	MPI_Status status;
	setvbuf(stdout, NULL, _IONBF, 0);

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks); //get number of processors
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid); //get rank in the group of processors
	destid = taskid + 1;
	if (destid >= numtasks)
		destid = 0;
	printf("MPI task %d of %d is sending to %d\n", taskid, numtasks, destid);
	MPI_Send(&taskid, 1, MPI_INT, destid, 0, MPI_COMM_WORLD);
	srcid = taskid - 1;
	if (srcid < 0)
		srcid = numtasks - 1;
	MPI_Recv(&recval, 1, MPI_INT, srcid, 0, MPI_COMM_WORLD, &status);
	printf ("MPI task %d of %d got %d\n", taskid, numtasks, recval);
	MPI_Finalize(); //has to be called
	exit(0);
}
