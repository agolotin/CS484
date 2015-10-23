#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define SIZE 15


double When() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

int* createArray() {
    int i, iproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

	int* values = (int*)malloc(sizeof(int) * SIZE);
	srand(time(NULL) + iproc);
	for (i = 0; i < SIZE; i++) {
		values[i] = abs(rand());
	}
	return values;
}

void swap(int* a, int* b) {
	int temp;
	temp = *b;
	*b = *a;
	*a = temp;
}

void quicksort(int l, int r, int* values) {
	int j;
	int pivot = values[l];                              
	int i = l + 1;                                      
	if (l + 1 < r)                                      
	{                                                   
		for (j = l + 1; j <= r; j++)                
		{                                               
			if (values[j] < pivot)                      
			{                                           
				swap(&values[j], &values[i]);             
				i++;                                    
			}                                           
		}                                               
		swap(&values[i - 1], &values[l]);                 
		quicksort(l, i, values);                                
		quicksort(i, r, values);                                
	}                                                   
}                                                       

void main(int argc, char *argv[]) {
	setbuf(stdout, 0);
    MPI_Init(&argc, &argv);
	char host[255];
	gethostname(host,253);

    int nproc, iproc;
	int tempNproc, tempIproc;
    MPI_Status status;
	MPI_Request request;

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
	
//	printf("I am process rank %d of %d running on %s\n", iproc, nproc,host);
//	int* values = createArray();

//	quicksort(0, SIZE - 1, values);                        

	int i;
//	for (i = 0; i < SIZE; i++) {
//		printf("%d  ", values[i]);
//	}

	int dimentions = ceil(log2(nproc));
	MPI_Comm comms[dimentions + 1];
	int mask = pow(2, (dimentions-1)) - 1; //something like 011
	int maskPlusOne = mask + 1;
	MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &comms[0]); //something like 100, int the dimention we're looking at

	int curSize = SIZE;
	for (i = dimentions; i > 0; i--) {
		MPI_Comm_size(comms[dimentions-i], &tempNproc);
		MPI_Comm_rank(comms[dimentions-i], &tempIproc);

		printf("[%d]: Nproc on the communicator ranked %d is %d\n", iproc, tempIproc, tempNproc);
		int dimentionBit;
		if ((tempIproc & maskPlusOne) == 0) {
			dimentionBit = 0;
		}
		else {
			dimentionBit = 1;
		}

		MPI_Comm_split(comms[dimentions-i], dimentionBit, 0, &comms[dimentions-i+1]);
		maskPlusOne = maskPlusOne >> 1;
		mask = mask >> 1;
	}
    MPI_Finalize();
}
