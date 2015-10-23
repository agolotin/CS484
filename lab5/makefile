MPI_HOME        = /users/faculty/snell/mpich
MPI_INCL        = $(MPI_HOME)/include
MPI_LIB         = $(MPI_HOME)/lib

sort: quicksort.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm quicksort.c -o sort 

sort1: quicksort_working.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm quicksort_working.c -o sort 

sort2: quicksort_not_working.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm quicksort_not_working.c -o sort_not_working

split: mpi_split.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm mpi_split.c -o split

