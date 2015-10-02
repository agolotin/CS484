MPI_HOME        = /users/faculty/snell/mpich
MPI_INCL        = $(MPI_HOME)/include
MPI_LIB         = $(MPI_HOME)/lib

mpi: mpi_lab.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm mpi_lab.c -o mpi
