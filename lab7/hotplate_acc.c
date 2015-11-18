#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define SIZE 8192

double When()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

void initialize_array(float** &h) {
	// initialize the hotplate
	for (int i = 0; i < SIZE; i++) {
		for (int j = 0; j < SIZE; j++) {
			if (i == SIZE - 1)
				h[i][j] = 100;
			else if (i == 0 || j == 0 || j == SIZE - 1)
				h[i][j] = 0;
			else if (firstTime)
				h[i][j] = 50;
		}
	}
}

int main() {

	int i;
	double start = When(), end;
	// initialize arrays
	float** hotplate = (float**)malloc(sizeof(float*) * SIZE);
	for (i = 0; i < SIZE; i++) {
		hotplate[i] = (float*)malloc(sizeof(float) * SIZE);
	}

	initialize_array(hotplate);
	
	float** t = (float**)malloc(sizeof(float*) * SIZE);
	for (i = 0; i < SIZE; i++) {
		t[i] = (float*)malloc(sizeof(float) * SIZE);
	}

	int iterationCounter = 0;
	int hasConverged = 1;
#pragma acc data copy(hotplate), create(t)
	while (hasConverged != 0) {

#pragma acc kernels 
		for (int i = 0; i < SIZE; i++) {
			for (int j = 0; j < SIZE; j++) {
				if (i == 0 || i == SIZE-1)
					continue;
				else if (j == 0 || j == SIZE-1)
					continue;
				else if (i == 200 && j == 500)
					continue;
				else {
					t[i][j] = 
						(hotplate[i+1][j] + hotplate[i-1][j] + 
						 hotplate[i][j+1] + hotplate[i][j-1] +
						 hotplate[i][j] * 4) / 8;
				}
			}
		}

		float** x = NULL;
		x = hotplate;
		hotplate = t;
		t = x;
		hasConverged = 0;

		//initialize_array(hotplate, false);
#pragma acc kernels 
		for (int i = 0; i < SIZE; i++) {
			for (int j = 0; j < SIZE; j++) {
				if (i == 0 || i == SIZE-1) 
					continue;
				else if (j == 0 || j == SIZE-1) 
					continue;
				else if (i == 200 && j == 500)
					continue;
				else {
					float average = (hotplate[i+1][j] + hotplate[i-1][j] + hotplate[i][j+1] + hotplate[i][j-1]) / 4;
					if (abs(hotplate[i][j] - average) >= 0.1)
						hasConverged = 1;
				}
			}
		}

		if (hasConverged != 1) {
			hasConverged = 0;
		}

		iterationCounter++;
	}
	end = When();
	printf("Total iterations: %d\n", iterationCounter);
	printf("Total time: %d\n", end - start);

	free(t);
	free(hotplate);
}
