#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define SIZE 10000000

int getPivot(int* array, int size){
	srand(time(NULL));
    return array[rand()%size];
    //return array[size/2];
}

int sort(const void *x, const void *y) {
    return (*(int*)x - *(int*)y);
}


double When()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

int main(int argc, char * argv[])
{
    int nproc, iproc;
    MPI_Status status;
    MPI_Init(&argc, &argv);

    int i = 0;
    double starttime;
    /*All for the sending / recieving */
    int* pivot = (int*)malloc(sizeof(int));
    int* send = (int*)malloc(sizeof(int));
    int* recv = (int*)malloc(sizeof(int));
    
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
    
    int mySize=SIZE/nproc;
    
    //my values
    int* vals = (int*)malloc( SIZE * sizeof(int));
    //for holding their values that come in
    int* theirs = (int*)malloc( SIZE * sizeof(int));
    //for joining the two
    int* tmp = (int*)malloc( SIZE * sizeof(int));
    
    
	srand(time(NULL));
    for (i = 0; i < mySize; i++) 
        vals[i] = rand() % 100000;

    MPI_Comm newcomm;
    MPI_Comm_split(MPI_COMM_WORLD, 1, iproc, &newcomm);    
    qsort(vals, mySize, sizeof(int), sort);

    int groupSize = nproc;
    starttime = When();

    while (groupSize > 1) {
        /* Get the new rank/size */
        MPI_Comm_size(newcomm, &nproc);
        MPI_Comm_rank(newcomm, &iproc);        
        *pivot = getPivot(vals, mySize);
        
		//send the pivot out among the group
		MPI_Bcast(pivot, 1, MPI_INT, 0, newcomm);
		//median of all in group
		if(iproc == 0){
			//we recieve them all
			for (i=1; i<nproc; i++) {
				MPI_Recv(recv, 1, MPI_INT, i, 0, newcomm, &status);
				tmp[i]=*recv;
			}
			tmp[0]=*pivot;
			qsort(tmp, nproc, sizeof(int), sort);
			*pivot=tmp[(nproc/2)];
			//printf("pivot=%i\n",*pivot);
			for (i=1; i<nproc; i++) {
				MPI_Send(pivot, 1, MPI_INT, i, 0, newcomm);
			}

		}
		else {
			//we all send it to zero and let it decide.
			MPI_Send(pivot, 1, MPI_INT, 0, 0, newcomm);
			MPI_Recv(pivot, 1, MPI_INT, 0, 0, newcomm, &status);
		}
		
        //calculate how many we will send
        *send = 0;
        for (i = 0; i < mySize; i++) {
            if(iproc < nproc / 2){
                if (vals[i] >= *pivot) {
                    tmp[*send]=vals[i];
                    (*send)++;
                }
            }
            else { 
                if (vals[i] <= *pivot) {
                    tmp[*send]=vals[i];
                    (*send)++;
                }
            }
        }
        
		//sedn the lower values first
        if(iproc < nproc/2) {
            //send the size then the values
            MPI_Send(send, 1, MPI_INT, iproc+(nproc/2), 0, newcomm);
            MPI_Send(tmp, *send, MPI_INT, iproc+(nproc/2), 0, newcomm);
            //we recieve the size and then values
            MPI_Recv(recv, 1, MPI_INT, iproc+(nproc/2), 0, newcomm, &status);
            MPI_Recv(theirs, *recv, MPI_INT, iproc+(nproc/2), 0, newcomm, &status);
        }
        else {
            //we recieve the two
            MPI_Recv(recv, 1, MPI_INT, iproc-(nproc/2), 0, newcomm, &status);
            MPI_Recv(theirs, *recv, MPI_INT, iproc-(nproc/2), 0, newcomm, &status);
            
            //send the size then the values
            MPI_Send(send, 1, MPI_INT, iproc-(nproc/2), 0, newcomm);
            MPI_Send(tmp, *send, MPI_INT, iproc-(nproc/2), 0, newcomm);
        }

        //now we combine the theirs and what is left of ours.
        if (iproc < nproc/2) {
            mySize -= *send;

            for (i=0; i<*recv; i++) {
                vals[mySize]=theirs[i];
                mySize++;
            }
        }
        else {
            int off = 0;
            for (i = 0; i < mySize; i++) {
                if(vals[i] >= *pivot){
                    theirs[*recv+off] = vals[i];
                    off++;
                }
            }
            int* temp = vals;
            vals = theirs;
            theirs = temp;
            mySize = *recv+(mySize-*send);
        }
        qsort(vals, mySize, sizeof(int), sort);
        
        //reset the size of the group
        MPI_Comm_split(newcomm, iproc < nproc/2 , iproc, &newcomm);
        groupSize /= 2;
    }
    
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
	float total = When() - starttime;
	float average;
//    printf("[%d] done. time: %f elements: %i\n", iproc, total, mySize);

	MPI_Reduce(&total, &average, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (iproc == 0)
		printf("average: %f\n", average/nproc);
//	int p;
//	for (p = 0; p < mySize; p++) {
//		printf("[%d] %d\n", iproc, vals[p]);
//	}

    free(vals);
    free(theirs);
    free(tmp);
    free(send);
    free(recv);
   
    MPI_Finalize();
   
    
    return 0;
}


