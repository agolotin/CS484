#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mpi.h"

#define VECSIZE 66560
#define ITERATIONS 100

double When()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

typedef struct vector{
	double val;
	int rank;
} vector; 

void reduce(vector* in, int numdim)
{ 
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int curdim;
    int notparticipating = 0;
    int bitmask = 1;
    for(curdim = 0; curdim < numdim; curdim++) {
		if ((rank & notparticipating) == 0) {
			if ((rank & bitmask) != 0) {
				int msg_dest = rank ^ bitmask;
//				printf("REDUCE. Sending message from machine ranked %d to %d\n", rank, msg_dest);
				MPI_Send(in, VECSIZE, MPI_DOUBLE_INT, msg_dest, 0, MPI_COMM_WORLD); 
			} 
			else {
				vector* temp = (vector*)malloc(sizeof(vector) * VECSIZE);
				int msg_src = rank ^ bitmask;
//				printf("REDUCE. Receiving on machine ranked %d from %d\n", rank, msg_src);
				MPI_Recv(temp, VECSIZE, MPI_DOUBLE_INT, msg_src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
				//receive the whole vector here and in place change in vector values to max
				int i;
				for (i = 0; i < VECSIZE; i++) {
					if (temp[i].val > in[i].val) {
						in[i].val = temp[i].val;
						in[i].rank = temp[i].rank;
					}
				}
				free(temp);
			}
		}
		notparticipating = notparticipating ^ bitmask;
		bitmask <<= 1;
	}
}

void bcast(vector* max, int numdim)
{
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int curdim;
    int notparticipating = pow(2,numdim-1)-1;
    int bitmask = pow(2,numdim-1);
    for(curdim = 0; curdim < numdim; curdim++) {
	if ((rank & notparticipating) == 0) {
	    if ((rank & bitmask) == 0) {
			int msg_dest = rank ^ bitmask;
			MPI_Send(max, VECSIZE, MPI_DOUBLE_INT, msg_dest, 0, MPI_COMM_WORLD);
	   	} 
		else {
			int msg_src = rank ^ bitmask;
			vector* temp = (vector*)malloc(sizeof(vector) * VECSIZE);
			MPI_Recv(temp, VECSIZE, MPI_DOUBLE_INT, msg_src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			int i;
			for (i = 0; i < VECSIZE; i++) {
				max[i].val = temp[i].val;
				max[i].rank = temp[i].rank;
			}
			free(temp);
		}
	}
	notparticipating >>= 1;
	bitmask >>=1;
	}
}


int main(int argc, char* argv[]) {
	//setbuf(stdout, 0);
	int iproc, nproc, i;
	char host[255];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

	gethostname(host,253);
//	printf("I am process rank %d of %d running on %s\n", iproc, nproc,host);
	
	/* Start everything here */
	vector* max = (vector*)malloc(sizeof(vector) * VECSIZE);
	int numdimensions = log2(nproc);

	int myrank, root = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	srand(myrank+5);
	/* Start time here */
	double start;
	if (myrank == root) 
		start = When();
	/* init vector to random values */
	int q;
	for (q = 0; q < ITERATIONS; q++) {
		for(i = 0; i < VECSIZE; i++) {
			max[i].val = rand();
			max[i].rank = myrank;
		  }
		/* Process MPI reduction */
		/* Find maximum values across processors by reducing to processor 0 */
		reduce(max, numdimensions);

		 /* At this point, the answer resides in max vector on proc rank=0 */
	//	if (myrank == root) {
	//		for (i=0; i<VECSIZE; ++i) {
	//			printf("root out[%d] = %f from %d\n",i,max[i].val,max[i].rank); 
	//		}
	//	}

		/* Process MPI reduction */
		/* proc rank=0 will broadcast its max vector to everyone, 
		  and other processors will just receive it to their max vectors */
		bcast(max, numdimensions);
	//	for (i=0; i<VECSIZE; ++i) {
	//		printf("%d processor out[%d] = %f from %d\n",myrank,i,max[i].val,max[i].rank);
	//	}
	}
	free(max);
	MPI_Finalize();
	double end = When();
	if(myrank == root) {
		printf("Time: %f\n",end-start);
	}
}

