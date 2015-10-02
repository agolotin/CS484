typedef struct barrier_node {
	pthread_mutex_t count_lock;
	pthread_cond_t ok_to_proceed_up;
	pthread_cond_t ok_to_proceed_down;
	int count;
} mylib_barrier_t_internal;

typedef struct barrier_node mylob_logbarrier_t[MAX_THREADS];
int number_in_barrier = 0;
pthread_mutex_t logbarrier_count_lock;

void mylib_logbarrier (mylob_logbarrier_t b, int num_threads, int thread_id);
void mylib_init_barrier(mylob_logbarrier_t b);

mylob_logbarrier_t barr;


void mylib_init_barrier(mylob_logbarrier_t b)
{
	int i;
	for (i = 0; i < MAX_THREADS; i++) {
		b[i].count = 0;
		pthread_mutex_init(&(b[i].count_lock), NULL); 
		pthread_cond_init(&(b[i].ok_to_proceed_up), NULL);
		pthread_cond_init(&(b[i].ok_to_proceed_down), NULL);
	}
	pthread_mutex_init(&logbarrier_count_lock, NULL);
}

void mylib_logbarrier (mylob_logbarrier_t b, int num_threads, int thread_id)
{
	int i, q, base, index;
	float *tmp;
	i = 2;
	base = 0;

	if (nproc == 1)
		return;

	pthread_mutex_lock(&logbarrier_count_lock);
	number_in_barrier++;
	if (number_in_barrier == nproc)
	{
		/* I am the last one in */
		/* swap the new value pointer with the old value pointer */
		tmp = oplate;
		oplate = iplate;
		iplate = tmp;
	/*
	fprintf(stderr,"%d: swapping pointers\n", thread_id);
	*/

	/* set the keepgoing flag and let everybody go */
	keepgoing = 0;
	for (q = 0; q < nproc; q++)
		keepgoing += lkeepgoing[q];
	}
	pthread_mutex_unlock(&logbarrier_count_lock);

	do {
		index = base + thread_id / i;
		if (thread_id % i == 0) {
			pthread_mutex_lock(&(b[index].count_lock));
			b[index].count ++;
			while (b[index].count < 2)
				pthread_cond_wait(&(b[index].ok_to_proceed_up),
			&(b[index].count_lock));
			pthread_mutex_unlock(&(b[index].count_lock));
		}
		else {
			pthread_mutex_lock(&(b[index].count_lock));
			b[index]! .count ++;
			if (b[index].count == 2)
				pthread_cond_signal(&(b[index].ok_to_proceed_up));
	/*
	while (b[index].count != 0)
	*/
	while (pthread_cond_wait(&(b[index].ok_to_proceed_down),
				&(b[index].count_lock)) != 0);
			pthread_mutex_unlock(&(b[index].count_lock));
			break;
		}
		base = base + num_threads/i;
		i = i * 2;
	} while (i <= num_threads);

	i = i / 2;

	for (; i > 1; i = i / 2)
	{
		base = base - num_threads/i;
		index = base + thread_id / i;
		pthread_mutex_lock(&(b[index].count_lock));
		b[index].count = 0;
		pthread_cond_signal(&(b[index].ok_to_proceed_down));
		pthread_mutex_unlock(&(b[index].count_lock));
	}
	pthread_mutex_lock(&logbarrier_count_lock);
	number_in_barrier--;
	pthread_mutex_unlock(&logbarrier_count_lock);
}
