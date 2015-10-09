#include <mpi.h>
#include <stdio.h>
#include <sys/time.h>


#define SIZE     1024
#define EPSILON  0.1

float ffabs(float f)
{
	    return (f > 0.0)? f : -f ;
}

double When()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

void main(int argc, char *argv[])
{
    float* nA;
	float* oA;
	float* tmp;
    int i, done, reallydone;
    int cnt;
    int start, end;
    int theSize;

    double starttime;

    int nproc, iproc;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    starttime = When();

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
    fprintf(stderr,"%d: Hello from %d of %d\n", iproc, iproc, nproc);
    
    /* Determine how much I should be doing and allocate the arrays*/
    theSize = SIZE / nproc;
	nA = (float*)malloc((theSize + 2) * sizeof(float));
    oA = (float*)malloc((theSize + 2) * sizeof(float));

    start = 1;
    end = theSize + 1;

    /* Initialize the cells */
    for (i = 0; i < theSize+2; i++)
    {
        nA[i] = oA[i] = 50;
    }

    /* Initialize the Boundaries */
    if (iproc == 0)
    {
        start = 2;
        nA[1] = oA[1] = 100;
    }
    if (iproc == nproc - 1)
    {
        end = theSize;
        nA[theSize] = oA[theSize] = 0;
    }



    /* Now run the relaxation */
    reallydone = 0;
    for(cnt = 0; !reallydone; cnt++)
    {
        /* First, I must get my neighbors boundary values */
        if (iproc != 0)
        {
            MPI_Send(&oA[1], 1, MPI_FLOAT, iproc - 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&oA[0], 1, MPI_FLOAT, iproc - 1, 0, MPI_COMM_WORLD, &status);
        }
        if (iproc != nproc - 1)
        {
            MPI_Send(&oA[theSize], 1, MPI_FLOAT, iproc + 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&oA[theSize + 1], 1, MPI_FLOAT, iproc + 1, 0, MPI_COMM_WORLD, &status);
        }


        /* Do the calculations */
        for (i = start; i < end; i++)
        {
            nA[i] = (oA[i-1] + oA[i+1] + 2 * oA[i]) / 4.0;
        }

        /* Check to see if we are done */
        done = 1;
        for (i = start; i < end; i++)
        {
            if (ffabs((nA[i-1] + nA[i+1])/ 2.0 - nA[i]) > EPSILON)
            {
                done = 0;
                break;
            }
        }
        /* Do a reduce to see if everybody is done */
        MPI_Allreduce(&done, &reallydone, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

        /* Swap the pointers */
        tmp = nA;
        nA = oA;
        oA = tmp;
    }

    /* print out the number of iterations to relax */
    fprintf(stderr, "%d:It took %d iterations and %lf seconds to relax the system\n", 
                                   iproc,cnt,When() - starttime);
    MPI_Finalize();
}
    
