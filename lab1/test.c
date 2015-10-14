#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define ROWS 4096
#define COLS 4096

double when() {
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double)tp.tv_sec + (double)tp.tv_usec * 1e-6);
}

void initPlate(double** plate) {
	int i, j;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			if (i == 0 || j == 0 || j == 4095)
				plate[i][j] = 0; // always fixed
			else if (i == 4095)
				plate[i][j] = 100; // always fixed
			else if (i == 400 && j >= 0 && j <= 330) //might have to change 400 to 399 and 330 to 329
				plate[i][j] = 100; // always fixed
			else if (i == 200 && j == 500) 
				plate[i][j] = 100; // always fixed
			else 
				plate[i][j] = 50; // UPDATE
		}
	}
}


int main(void) {
	printf("HERE");
	//Print out starting time
	double start = when();
	printf("Starting time: %f\n", start);

	double** plate = (double**)malloc(ROWS * sizeof(double*));
	int k;
	for (k = 0; k < ROWS; k++)
		plate[k] = (double*)malloc(COLS * sizeof(double));
	initPlate(plate);





	double end = when();
	printf("Ending time: %f\n", end);
	printf("\nTotal execution time: %f\n", end - start);

	//write to file
	FILE* ofp = fopen("output.csv", "w");
	if (ofp == NULL)
		ofp = fopen("output.csv", "w+");
	

	int i, j;
	for (i = 0; i < 4096; i++) {
		for (j = 0; j < 4096; j++) {
			fprintf(ofp, "%f," ,plate[i][j]);
		}
		fprintf(ofp, "\n");
	}
	fclose(ofp);

	return EXIT_SUCCESS;
}
