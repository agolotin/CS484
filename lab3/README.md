# Hypercube MPI Reduce and Broadcase
The program uses custom defined recieve and broadcast functions that are implemented using hypercube algorithm. Inside those functions only MPI_Send() and MPI_Recv() are used. The only purpose of the program is to reduce the vector to find the maximum value between processors and then broadcast it to everyone processor.
