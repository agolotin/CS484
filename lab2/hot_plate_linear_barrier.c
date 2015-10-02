#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#define ROWS 16384
#define COLS 16384

#define EPSILON 0.1
#define TRUE 1
#define FALSE 0
#define MAX_THREADS 24

float** main_plate;
float** main_prev_plate;
char** main_locked_cells;
int iterations;
volatile char STOP;
volatile int threads_finished;
volatile char can_enter;

pthread_mutex_t finished_count;

typedef struct {
	pthread_mutex_t count_lock;
	pthread_cond_t ok_to_proceed;
	int count;
} barrier_node;

volatile int number_in_barrier = 0;
pthread_mutex_t linearbarrier_lock;

barrier_node* barr;

typedef struct arg_plate {
	int iproc;
	int nthreads;
	int begin;
	int end;
} arg_plate_t;


void mylib_init_barrier(barrier_node* b, int nthreads)
{
	b->count = 0;
	pthread_mutex_init(&(b->count_lock), NULL); 
	pthread_cond_init(&(b->ok_to_proceed), NULL);
	pthread_mutex_init(&linearbarrier_lock, NULL);
}
void mylib_destroy_barrier(barrier_node* b) {
	pthread_mutex_destroy(&(b->count_lock)); 
	pthread_cond_destroy(&(b->ok_to_proceed));
	pthread_mutex_destroy(&linearbarrier_lock);
}
void copy(float** main, float** result) {
	int i,j;
	for (i = 0; i < ROWS; i++) {
		for (j = 0; j < COLS; j++) {
			result[i][j] = main[i][j];
		}
	}
}
void linearbarrier(barrier_node* b, int num_threads, int thread_id)
{
	if (num_threads == 1)
		return;

	pthread_mutex_lock(&(b->count_lock));
	b->count++;
	if (b->count == num_threads) {
		b->count = 0;
		pthread_cond_broadcast(&(b->ok_to_proceed));
	}
	else {
		while (pthread_cond_wait(&(b->ok_to_proceed), &(b->count_lock)) != 0);
	}
	pthread_mutex_unlock(&(b->count_lock));

}

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

char steady(float** current_plate) {
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
	if (main_diff > EPSILON)
		return (FALSE);
	else 
		return (TRUE);
}

/* Updating plate and steady state functions */
void* update_plate(void* plate_arguments) {
	iterations = 0;
	threads_finished = 0;
	can_enter = (FALSE);
	arg_plate_t* plate_args = (arg_plate_t*)plate_arguments;
	STOP = (FALSE);

	for (;;) {

		linearbarrier(barr, plate_args->nthreads, plate_args->iproc); 

		pthread_mutex_lock(&finished_count);
		if (can_enter == (FALSE))
			can_enter = (TRUE);
		pthread_mutex_unlock(&finished_count);

		int begin = plate_args->begin;
		int end = plate_args->end;

		/* Updating */
		int i, j;
		for (i = begin; i < end; i++) {
			for (j = 0; j < COLS; j++) {
				if (main_locked_cells[i][j] == '0') {
					main_plate[i][j] = (main_prev_plate[i+1][j] + main_prev_plate[i][j+1] + main_prev_plate[i-1][j] 
							+ main_prev_plate[i][j-1] + 4 * main_prev_plate[i][j]) * 0.125;
				}
			}
		}
		/* End updating */

		linearbarrier(barr, plate_args->nthreads, plate_args->iproc); //wait for all threads to finish updating here

		pthread_mutex_lock(&finished_count);
		if (can_enter == (TRUE)) { // allow only the first thread to do main stuff
			can_enter = (FALSE);
			copy(main_plate, main_prev_plate);

			iterations++;
			//printf("Iteration: %d\n", iterations);

			STOP = steady(main_plate);
		}
		
		if (STOP == (TRUE)) {
			threads_finished++;
			pthread_mutex_unlock(&finished_count);
			break;
		}
		pthread_mutex_unlock(&finished_count);
	} 
	pthread_mutex_lock(&finished_count);
	if (threads_finished == plate_args->nthreads) //last one free all arguments
		free(plate_arguments);
	pthread_mutex_unlock(&finished_count);
}

void allocateWorkload(int nthreads, int* begin_end) {
	int step = ROWS / nthreads;
	int i;
	int begin = 0;
	for (i = 0; i < nthreads*2; i++) {
		begin_end[i] = begin;

		begin = begin+step;
		i += 1;

		begin_end[i] = begin;
	}
}

void startUpdate(int nthreads) {
	printf("Updating plate to steady state\n");

	int* begin_end = (int*)malloc((nthreads*2) * sizeof(int));
	allocateWorkload(nthreads, begin_end);

	int i;
	int j;
	pthread_t worker[nthreads];
	arg_plate_t* plate_args;
	for (i = 0; i < nthreads; i++) {
		j = i * 2;
		plate_args = (arg_plate_t*)malloc(sizeof(arg_plate_t));

		//assign arguments to every plate
		plate_args->begin = begin_end[j];
		plate_args->end = begin_end[j+1];
		plate_args->iproc = i;
		plate_args->nthreads = nthreads;

		pthread_create(&worker[i], NULL, &update_plate, (void*)plate_args);
	}

	for (i = 0; i < nthreads; i++) {
		pthread_join(worker[i], NULL);
	}
	free(begin_end);
	printf("All threads finished execution and joined together\n");
}



/* ========== MAIN ========== */
int main(int argc, char* argv[]) {
	/* Print out starting time */
	double start = when();
	printf("Starting time: %f\n", start);

	/* Create thread count */
	int nthreads = atoi(argv[1]);

	/* Create plate */
	main_plate = createPlate();
	main_prev_plate = createPlate();
	main_locked_cells = createCharPlate(); // bool array of locked cells

	/* Init barriers and mutex */
	pthread_mutex_init(&finished_count, NULL);
	barr = (barrier_node*)malloc(sizeof(barrier_node));
	mylib_init_barrier(barr, nthreads);

	/* Init main plate and make a duplicate copy */
	initPlate(main_plate, main_prev_plate);

	/* Start updating */
	startUpdate(nthreads);//, barr);

	/* Report time and cleanup */
	double end = when();
	printf("\nEnding time: %f\n", end);
	printf("Total execution time: %f\n", end - start);
	printf("Number of iterations: %d\n\n", iterations);

	/* Destroy mutex and barriers */
	pthread_mutex_destroy(&finished_count);
	mylib_destroy_barrier(barr);

	/* free memory */
	printf("Cleanup\n");
	free(barr);
	cleanupFloat(main_plate);
	cleanupFloat(main_prev_plate);
	cleanupChar(main_locked_cells);

	return EXIT_SUCCESS;
}

