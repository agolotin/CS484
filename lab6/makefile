MPI_HOME        = /users/faculty/snell/mpich
MPI_INCL        = $(MPI_HOME)/include
MPI_LIB         = $(MPI_HOME)/lib


mandelbrot: mandelbrot_set.c
	$(MPI_HOME)/bin/mpicc -O -I $(MPI_INCL) -lm mandelbrot_set.c -o mandelbrot

mand: mand2.c
	g++ -g -o mandelbrot mand2.c
