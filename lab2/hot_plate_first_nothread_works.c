#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define ROWS 16384
#define COLS 16384

#define EPSILON 0.1
#define TRUE 1
#define FALSE 0

float** main_plate;
float** main_prev_plate;
char** main_locked_cells;
FILE* ofp;

struct arg_plate_t {
	float** (*palte);
	float** (*prev_plate);
};

double when() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double)tp.tv_sec + (double)tp.tv_usec * 1e-6);
}

/* Creating plates and duplicating plates function */
float** createPlate() {
	float** plate = (float**)malloc(ROWS * sizeof(float*));
	int k;
	for (k = 0; k < ROWS; k++) {
		plate[k] = (float*)malloc(COLS * sizeof(float));
	}
	return plate;
}
char** createCharPlate() {
	char** plate = (char**)malloc(ROWS * sizeof(char*));
	int k;
	for (k = 0; k < ROWS; k++) {
		plate[k] = (char*)malloc(COLS * sizeof(char));
	}
	return plate;
}
void copy(float** main, float** result) {
	int i,j;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			//*(*(result + i)+j) = *(*(main + i)+j);
			result[i][j] = main[i][j];
		}
	}
}
/* Initializing plate function */
void initPlate(float** plate, float** prev_plate) {
	int i, j;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (i == 0 || j == 0 || j == COLS-1) {
				plate[i][j] = 0;
				prev_plate[i][j] = 0;
				main_locked_cells[i][j] = '1';
			}
			else if (i == ROWS-1) {
				plate[i][j] = 100; 
				prev_plate[i][j] = 100; 
				main_locked_cells[i][j] = '1';

			}
			else if (i == 400 && j >= 0 && j <= 330) {
				plate[i][j] = 100;
				prev_plate[i][j] = 100;
				main_locked_cells[i][j] = '1';
			}
			else if (i == 200 && j == 500) {
				plate[i][j] = 100; 
				prev_plate[i][j] = 100; 
				main_locked_cells[i][j] = '1';
			}
			else  {
				plate[i][j] = 50; // UPDATE
				prev_plate[i][j] = 50; // UPDATE
				main_locked_cells[i][j] = '0';
			}
		}
	}
	for (i = 0; i < ROWS; i++) {
		if ((i % 20) == 0) { //every 20th row to 100
			for (j = 0; j < COLS; j++) {
				plate[i][j] = 100; 
				prev_plate[i][j] = 100; 
				main_locked_cells[i][j] = '1';
			}
		}
	}
	for (j = 0; j < COLS; j++) {
		if ((j % 20) == 0) { //every 20th column to 0
			for (i = 0; i < COLS; i++) {
				plate[i][j] = 0; 
				prev_plate[i][j] = 0; 
				main_locked_cells[i][j] = '1';
			}
		}
	}
}
/* Cleanup functions */
void cleanupFloat(float** plate) {
	int i, j;
	for (i = 0; i < ROWS; i++) {
		free(plate[i]);
	}
	free(plate);
}
void cleanupChar(char** plate) {
	int i, j;
	for (i = 0; i < ROWS; i++) {
		free(plate[i]);
	}
	free(plate);
}
/* Updating plate and steady state functions */
void update_plate(float** current_plate, float** prev_plate) {
	int i, j;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (main_locked_cells[i][j] == '0') {
				//printf("i == %d, j == %d, cell == %c\n", i, j, lockedCells[i][j]);
				current_plate[i][j] = (prev_plate[i+1][j] + prev_plate[i][j+1] + prev_plate[i-1][j] 
						+ prev_plate[i][j-1] + 4 * prev_plate[i][j]) * 0.125;
			}
		}
	}
}
char steady(float** current_plate, int iteration) {
	int count = 0;
	int i, j;
	float main_diff = 0;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (main_locked_cells[i][j] == '0') {
				if (current_plate[i][j] > 50) 
					count++;

				float diff = fabs(current_plate[i][j] - (current_plate[i+1][j] + current_plate[i-1][j] 
							+ current_plate[i][j+1] + current_plate[i][j-1]) * 0.25);
				if (diff > main_diff)
					main_diff = diff;
			}
		}
	}
	printf("Num cells over 50: %d\n", count);
	fprintf(ofp, "%d,%d\n", iteration, count); //write out what iteration it is and its count
	if (main_diff > EPSILON)
		return (TRUE);
	else 
		return (FALSE);
}

int startUpdate(pthread_t* threads, int nthreads) {
	printf("Updating plate to steady state\n");

	char flag = '1';
	int iterations = 0;
	long thread;
	struct arg_plate_t palte_args;
	do {
		iterations++;
		printf("Itreation %d\n", iterations);
	//	for (thread = 0; thread < nthreads; thread++) {
			switch(flag) {
			case '1':
				//init a barrier here 
				update_plate(main_plate, main_prev_plate);
				flag = '0';
				//release the barrier here
				break;
			case '0':
				//init a barrier here 
				update_plate(main_prev_plate, main_plate);
				flag = '1';
				//release the barrier here
				break;
			}
	//	}
	} while(steady(((flag == '0') ? main_plate : main_prev_plate), 
				iterations));
	return iterations;
}

/* ========== MAIN ========== */
int main(int argc, char* argv[]) {
	/* Print out starting time */
	double start = when();
	printf("Starting time: %f\n", start);

	/* Write first line in output csv file */
	ofp = fopen(argv[2], "w+"); // where to write out csv with over 50 degrees
	fprintf(ofp, "Iteration,Num cells over 50\n");

	/* Create threads */
	int nthreads = atoi(argv[1]);
	pthread_t threads[nthreads];

	/* Create plate */
	main_plate = createPlate();
	main_prev_plate = createPlate();
	main_locked_cells = createCharPlate(); // bool array of locked cells

	/* Init main plate and make a duplicate copy */
	initPlate(main_plate, main_prev_plate);

	/* Start updating */
	int iterations = startUpdate(threads, nthreads);

	/* Close file buffer, report time and cleanup */
	fclose(ofp);
	double end = when();
	printf("\nEnding time: %f\n", end);
	printf("Total execution time: %f\n", end - start);
	printf("Number of iterations: %d\n\n", iterations);

	/* free memory */
	printf("Cleanup\n");
	cleanupFloat(main_plate);
	cleanupFloat(main_prev_plate);
	cleanupChar(main_locked_cells);

	return EXIT_SUCCESS;
}

