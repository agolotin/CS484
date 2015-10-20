#Hot Plate MPI
hot_plate_mpi.c uses MPI_Send and MPI_Recv in order to get the boundary values from the other processors for a single processor. nonblocking_mpi.c follows the same pattern, except non-blocking MPI_Isend and MPI_Irecv are used.
