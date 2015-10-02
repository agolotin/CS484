#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define ROWS 8192 
#define COLS 8192

#define EPSILON 0.1
#define TRUE 1
#define FALSE 0



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
	#pragma omp parallel for private(i,j) 
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			//*(*(result + i)+j) = *(*(main + i)+j);
			result[i][j] = main[i][j];
		}
	}
}
/* Initializing plate function */
void initPlate(float** plate, char** lockedCells) {
	int i, j;
	#pragma omp parallel for private(i,j) 
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (i == 0 || j == 0 || j == COLS-1) {
				plate[i][j] = 0;
				lockedCells[i][j] = '1';
			}
			else if (i == ROWS-1) {
				plate[i][j] = 100; 
				lockedCells[i][j] = '1';
			}
			else if (i == 400 && j >= 0 && j <= 330) {
				plate[i][j] = 100;
				lockedCells[i][j] = '1';
			}
			else if (i == 200 && j == 500) {
				plate[i][j] = 100; 
				lockedCells[i][j] = '1';
			}
			else  {
				plate[i][j] = 50; // UPDATE
				lockedCells[i][j] = '0';
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
void update_plate(float** current_plate, float** prev_plate, char** lockedCells) {
	int i, j;
	#pragma omp parallel for private(i, j)
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (lockedCells[i][j] == '0') {
				//printf("i == %d, j == %d, cell == %c\n", i, j, lockedCells[i][j]);
				current_plate[i][j] = (prev_plate[i+1][j] + prev_plate[i][j+1] + prev_plate[i-1][j] 
						+ prev_plate[i][j-1] + 4 * prev_plate[i][j]) * 0.125;
			}
		}
	}
}
char steady(float** current_plate, char** lockedCells) {
	int i, j;
	float diff = 1;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (lockedCells[i][j] == '0') {
				diff = fabs(current_plate[i][j] - (current_plate[i+1][j] + current_plate[i-1][j] 
							+ current_plate[i][j+1] + current_plate[i][j-1]) * 0.25);
				if (diff > EPSILON)
					return (TRUE);
			}
		}
	}
	return (FALSE);
}
/* ========== MAIN ========== */
int main(int argc, char* argv[]) {
	/* Print out starting time */
	double start = when();
	printf("Starting time: %f\n", start);
	/* Create plate */
	float** plate = createPlate();
	float** prev_plate = createPlate();
	char** lockedCells = createCharPlate(); // bool array of locked cells
	/* Init main plate and make a duplicate copy */
	initPlate(plate, lockedCells);
	copy(plate, prev_plate);
	/* Start updating */
	printf("Updating plate to steady state\n");
	char flag = '1';
	int iterations = 0;
	do {
		iterations++;
		switch(flag) {
		case '1':
			update_plate(plate, prev_plate, lockedCells);
			flag = '0';
			break;
		case '0':
			update_plate(prev_plate, plate, lockedCells);
			flag = '1';
			break;
		}
	}while(steady(((flag == '0') ? plate : prev_plate), lockedCells));

	/* Report time, write to file and cleanup */
	double end = when();
	printf("\nEnding time: %f\n", end);
	printf("Total execution time: %f\n", end - start);
	printf("Number of iterations: %d\n\n", iterations);
	/* Copy final array to main one and write it to the file */
//	if (flag == '1') 
//		copy(prev_plate, plate);
//	/* write to file */
//	printf("Writing to file\n");
//	FILE* ofp = fopen(argv[1], "w+");
//	int i, j;
//	#pragma omp parallel for private(i,j)
//	for (i = 0; i < ROWS; i++) {
//		for (j = 0; j < COLS; j++) {
//			fprintf(ofp, "%f," ,plate[i][j]);
//		}
//		fprintf(ofp, "\n");
//	}
//	fclose(ofp);
//
	/* free memory */
	printf("Cleanup\n");
	cleanupFloat(plate);
	cleanupFloat(prev_plate);
	cleanupChar(lockedCells);

	return EXIT_SUCCESS;
}

