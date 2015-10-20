MPI_HOME        = /users/faculty/snell/mpich
MPI_INCL        = $(MPI_HOME)/include
MPI_LIB         = $(MPI_HOME)/lib

split: mpi_split.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm mpi_split.c -o split

sort: quicksort.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm quicksort.c -o sort 

