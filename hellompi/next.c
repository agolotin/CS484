#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
int main (int argc, char *argv[])
{
  int	taskid;	        /* task ID - also used as seed number */
  int	numtasks;       /* number of tasks */
  int 	destid;		// Destination id
  int 	srcid;		// Src id
  int 	recvval;
  MPI_Status status;
  setvbuf( stdout, NULL, _IONBF, 0 );

  /* Obtain number of tasks and task ID */
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
  destid = taskid+1;
  if(destid >= numtasks)
    destid = 0;
  printf ("MPI task %d of %d sending to %d\n", taskid, numtasks,destid);
  MPI_Send(&taskid, 1, MPI_INT, destid, 0, MPI_COMM_WORLD);
  srcid = taskid - 1;
  if(srcid < 0)
    srcid = numtasks-1;
  MPI_Recv(&recvval, 1, MPI_INT, srcid, 0, MPI_COMM_WORLD,&status);
  printf ("MPI task %d of %d got %d\n", taskid, numtasks,recvval);
  MPI_Finalize();
  exit(0);
}
