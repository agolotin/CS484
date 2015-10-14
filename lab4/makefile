MPI_HOME        = /users/faculty/snell/mpich
MPI_INCL        = $(MPI_HOME)/include
MPI_LIB         = $(MPI_HOME)/lib

nonblocking: nonblocking_mpi.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm nonblocking_mpi.c -o nonblocking_mpi

hyper_mpi: hot_plate_mpi_hypercube.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm hot_plate_mpi_hypercube.c -o hypercube_mpi

mpi: hot_plate_mpi.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm hot_plate_mpi.c -o hot_plate_mpi
