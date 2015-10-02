#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define ROWS 4096
#define COLS 4096

#define EPSILON 0.1
#define TRUE 1
#define FALSE 0



double when() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double)tp.tv_sec + (double)tp.tv_usec * 1e-6);
}

/* Creating plates and duplicating plates function */
float* createPlate() {
	float* plate = (float*)malloc(ROWS * COLS * sizeof(float));
	return plate;
}
char* createCharPlate() {
	char* plate = (char*)malloc(ROWS * COLS * sizeof(char));
	return plate;
}
void copy(float* main, float* result) {
	int i,j;
	#pragma omp parallel for private(i,j)
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			//*(*(result + i)+j) = *(*(main + i)+j);
			result[i + j * COLS] = main[i + j * COLS];
		}
	}
}
/* Initializing plate function */
void initPlate(float* plate, char* lockedCells) {
	int i, j;
	#pragma omp parallel for private(i,j) 
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (i == 0 || j == 0 || j == 4095) {
				plate[i + j * COLS] = 0;
				lockedCells[i + j * COLS] = '1';
			}
			else if (i == 4095) {
				plate[i + j * COLS] = 100; 
				lockedCells[i + j * COLS] = '1';
			}
			else if (i == 400 && j >= 0 && j <= 330) {
				plate[i + j * COLS] = 100;
				lockedCells[i + j * COLS] = '1';
			}
			else if (i == 200 && j == 500) {
				plate[i + j * COLS] = 100; 
				lockedCells[i + j * COLS] = '1';
			}
			else  {
				plate[i + j * COLS] = 50; // UPDATE
				lockedCells[i + j * COLS] = '0';
			}
		}
	}
}
/* Cleanup functions */
void cleanupFloat(float* plate) {
	free(plate);
}
void cleanupChar(char* plate) {
	free(plate);
}
/* Updating plate and steady state functions */
void update_plate(float* current_plate, float* prev_plate, char* lockedCells) {
	int i, j;
	#pragma omp parallel for private(i, j)
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (lockedCells[i + j * COLS] == '0') {
				//printf("i == %d, j == %d, cell == %c\n", i, j, lockedCells[i][j]);
				current_plate[i + j * COLS] = (prev_plate[i+1 + j * COLS] + prev_plate[i + (j+1)*COLS] + prev_plate[i-1 + j * COLS] 
						+ prev_plate[i + (j-1) * COLS] + 4 * prev_plate[i + j * COLS]) * 0.125;
			}
		}
	}
}
char steady(float* current_plate, char* lockedCells) {
	int i, j;
	float diff = 1;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (lockedCells[i + j * COLS] == '0') {
				diff = fabs(current_plate[i + j * COLS] - (current_plate[i+1 + j * COLS] + current_plate[i-1 + j * COLS] 
							+ current_plate[i + (j+1) * COLS] + current_plate[i + (j-1) * COLS]) * 0.25);
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
	float* plate = createPlate();
	float* prev_plate = createPlate();
	char* lockedCells = createCharPlate();// bool array of locked cells
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
	} while(steady(((flag == '0') ? plate : prev_plate), lockedCells));


	double end = when();
	printf("\nEnding time: %f\n", end);
	printf("Total execution time: %f\n", end - start);
	printf("Number of iterations: %d\n\n", iterations);
	/* Copy final array to main one and write it to the file */
	if (flag == '1') 
		copy(prev_plate, plate);
	/* write to file */
	printf("Writing to file\n");
	FILE* ofp = fopen(argv[1], "w+");
	int i, j;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			fprintf(ofp, "%f," ,plate[i + j * COLS]);
		}
		fprintf(ofp, "\n");
	}
	fclose(ofp);

	/* free memory */
	printf("Cleanup\n");
	cleanupFloat(plate);
	cleanupFloat(prev_plate);
	cleanupChar(lockedCells);

	return EXIT_SUCCESS;
}

