#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* serve(void* argument) {
	printf("Thread: %d\n", (long)argument);
}

int main(int argc, char* argv[]) {
	int nthreads = atoi(argv[1]);
	pthread_t threads[nthreads];
	long thread;
	printf("Creating threads\n");
	for (thread = 0;thread < nthreads; thread++) {
		pthread_create(&threads[thread], NULL, &serve, (void *)thread);
	}
	printf("Joining threads\n");
	for (thread = 0;thread < nthreads; thread++) {
		pthread_join(threads[thread], NULL);
	}
	printf("Joined threads\n");
}
