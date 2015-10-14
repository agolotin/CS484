#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>


#define TRUE 1
#define FALSE 0
#define SIZE 16384
#define EPSILON  0.1

double When()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

void cleanupLocked(float** plate, int rows) {
	int i;

	for (i = 0; i < rows; i++) {
		free(plate[i]);
	}
	free(plate);
}

void cleanup(float** plate, int rows, int iproc, int nproc) {
	int i = 1;

	if (iproc == (nproc - 1)) {
		i = 3;
		rows += 2;
	}

	for (i; i < rows+1; i++) {
		free(plate[i]);
	}
}

float** createPlate(int rows_assinged) {
	float** plate = (float**)malloc((rows_assinged + 2) * sizeof(float*));

	int k;
	for (k = 0; k < SIZE; k++) {
		plate[k] = (float*)malloc(SIZE * sizeof(float));
	}
	return plate;
}

void initPlate(float** plate, float** prev_plate, float** locked_cells, int rows_assinged, int iproc) {
	int i = 1, j;
	for (i; i < rows_assinged + 1; i++) {
		/* get the actual row value based on the processor number and number of rows assinged */
		int actual_row = (i - 1) + (iproc * rows_assinged); 
		for (j = 0; j < SIZE; j++) {
			if (actual_row == 0 || j == 0 || j == SIZE-1) {
				plate[i][j] = 0;
				prev_plate[i][j] = 0;
				locked_cells[i][j] = 1;
			}
			else if (actual_row == SIZE-1) {
				plate[i][j] = 100; 
				prev_plate[i][j] = 100; 
				locked_cells[i][j] = 1;

			}
			else  { //UPDATE
				plate[i][j] = 50; 
				prev_plate[i][j] = 50; 
				locked_cells[i][j] = 0;
			}
		}
	}
}
void update_plate(int iproc, float** current_plate, float** prev_plate, float** locked, int rows_assinged, int begin, int end) {
	int i, j;
	for (i = begin; i < end; i++) {
		for (j = 0; j < SIZE; j++) {
			if (!locked[i][j]) {
				current_plate[i][j] = (prev_plate[i+1][j] + prev_plate[i][j+1] + prev_plate[i-1][j] 
					+ prev_plate[i][j-1] + 4 * prev_plate[i][j]) * 0.125;
			}
		}
	}
}

int steady(int iproc, float** current_plate, float** locked, int rows_assinged, int begin, int end) {
	int i, j;
	float main_diff = 0;

	for (i = begin; i < end; i++) {
		for (j = 0; j < SIZE; j++) {
			if (!locked[i][j]) {
				float diff = fabs(current_plate[i][j] - (current_plate[i+1][j] + current_plate[i-1][j] 
							+ current_plate[i][j+1] + current_plate[i][j-1]) * 0.25);
				if (diff > main_diff)
					main_diff = diff;
			}
		}
	}
	if (main_diff > EPSILON)
		return 0;//(FALSE);
	else 
		return 1;//(TRUE);
}

int main(int argc, char *argv[])
{
	setbuf(stdout, 0);
	float **plate, **prev_plate, **temp_plate, **locked_cells;
    int start, end, rows_assinged;

    MPI_Init(&argc, &argv);

	char host[255];
	gethostname(host,253);

    int nproc, iproc;
    MPI_Status status1;
    MPI_Status status2;
	MPI_Request request1;
	MPI_Request request2;
	MPI_Request request3;
	MPI_Request request4;

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
	
	printf("I am process rank %d of %d running on %s\n", iproc, nproc,host);
    
    /* Determine how much I should be doing and allocate the arrays*/
    rows_assinged = SIZE / nproc; // rows for a specific processor to look at
	plate = createPlate(rows_assinged);
	prev_plate = createPlate(rows_assinged);
	locked_cells = createPlate(rows_assinged);

    start = 1;
    end = rows_assinged + 1;

    /* Initialize the cells */
	initPlate(plate, prev_plate, locked_cells, rows_assinged, iproc);

	if (iproc == 0) {
		start = 2;
	}
	if (iproc == (nproc - 1)) {
		end = rows_assinged;
	}

	/* First we need to get the 0th row of locked cells from my previous neighbor */
	if (iproc != 0) {
		MPI_Isend(locked_cells[1], SIZE, MPI_FLOAT, iproc - 1, 0, MPI_COMM_WORLD, &request1);
		MPI_Recv(locked_cells[0], SIZE, MPI_FLOAT, iproc - 1, 0, MPI_COMM_WORLD, &status1);
	}
	if (iproc != (nproc - 1)) {
		MPI_Isend(locked_cells[rows_assinged], SIZE, MPI_FLOAT, iproc + 1, 0, MPI_COMM_WORLD, &request1);
		MPI_Recv(locked_cells[rows_assinged + 1], SIZE, MPI_FLOAT, iproc + 1, 0, MPI_COMM_WORLD, &status1);
	}

    /* Now run the relaxation */
    double starttime = When();

    int done = 0, reallydone = 0, iterations = 0;
    while(!reallydone)
    {
		iterations++;
        /* First, I must get my neighbors boundary values */
        if (iproc != 0) 
        {
			/* If I'm not the first processor I want to send/receive to my previous neighbour */
            MPI_Isend(prev_plate[1], SIZE, MPI_FLOAT, iproc - 1, 0, MPI_COMM_WORLD, &request1);
            MPI_Irecv(prev_plate[0], SIZE, MPI_FLOAT, iproc - 1, 0, MPI_COMM_WORLD, &request2);
			start++;
        }
        if (iproc != nproc - 1) 
        {
			/* If I'm not the last processor I want to send/receice from my next neighbor */
            MPI_Isend(prev_plate[rows_assinged], SIZE, MPI_FLOAT, iproc + 1, 0, MPI_COMM_WORLD, &request3);
            MPI_Irecv(prev_plate[rows_assinged + 1], SIZE, MPI_FLOAT, iproc + 1, 0, MPI_COMM_WORLD, &request4);
			end--;
        }
		update_plate(iproc, plate, prev_plate, locked_cells, rows_assinged, start, end);

		if (iproc != 0) {
			int temp_end = start;
			int temp_start = start - 1;
			MPI_Wait(&request2, &status1);
			update_plate(iproc, plate, prev_plate, locked_cells, rows_assinged, temp_start, temp_end);
			start--;
		}
		if (iproc != nproc - 1) {
			int temp_start = end;
			int temp_end = end + 1;
			MPI_Wait(&request4, &status2);
			update_plate(iproc, plate, prev_plate, locked_cells, rows_assinged, temp_start, temp_end);
			end++;
		}

		done = steady(iproc, plate, locked_cells, rows_assinged, start, end);
        MPI_Allreduce(&done, &reallydone, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

        /* Swap the pointers */
        temp_plate = plate;
        plate = prev_plate;
        prev_plate = temp_plate;
    }
	
    /* print out the number of iterations to relax */
    printf("%d :It took %d iterations and %lf seconds to relax the system\n", 
                                   iproc, iterations, When() - starttime);

	/* Deallocate everything */
//	cleanup(plate, rows_assinged, iproc, nproc);
//	cleanup(prev_plate, rows_assinged, iproc, nproc);
//	cleanup(locked_cells, rows_assinged, iproc, nproc);

    MPI_Finalize();
	return EXIT_SUCCESS;
}
    
