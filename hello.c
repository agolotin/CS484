#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


main(int argc, char *argv[])
{
        int iproc, nproc,i;
        char host[255], message[55];
        MPI_Status status;
        MPI_Request request;

        gethostname(host,253);

        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &nproc);
        MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

        printf("I am proc %d of %d running on %s\n", iproc, nproc,host);
        system("w");
	system("touch foo");
        fflush(stdout);

        printf("Sending messages\n");
        fflush(stdout);
        sprintf(message, "%d: Hello\n", iproc);
        for (i = 0; i < nproc; i++)
        {
                if (i != iproc)
                        MPI_Isend(message, 35, MPI_CHAR, i, 0, MPI_COMM_WORLD, &request);
        }
        printf("Receiving messages\n");
        fflush(stdout);
        for (i = 0; i < nproc; i++)
        {
                if (i != iproc)
                {
                        MPI_Recv(message, 35, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
                        printf("%d: %s",iproc, message);
                        fflush(stdout);
                }
        }

        MPI_Finalize();
}
