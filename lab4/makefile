MPI_HOME        = /users/faculty/snell/mpich
MPI_INCL        = $(MPI_HOME)/include
MPI_LIB         = $(MPI_HOME)/lib

mpi: linear.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm linear.c -o linear
