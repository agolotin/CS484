#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <cuda.h>

#define NUM_ROWS 8192
#define NUM_COLS 8192
#define EPSILON  0.1
#define TRUE	1.0f
#define FALSE	0.0f
#define TOTITERATIONS 359

double When()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}


__global__ void initArrays(float *from, float *to, float *locked, int size) {
	int idx = blockDim.x * blockIdx.x + threadIdx.x;
	
	if (idx < size) {
		// inner is 50s
		if ((blockIdx.x > 0) && (blockIdx.x < NUM_ROWS)){
			if ((threadIdx.x > 0) && (threadIdx.x < NUM_COLS)) {
				from[idx] = 50;
				to[idx] = 50;
				locked[idx] = FALSE;
			}
		}
		
		// sides are 0
		if ((threadIdx.x == 0) || (threadIdx.x == NUM_COLS-1)) {
			from[idx] = 0;
			to[idx] = 0;
			locked[idx] = TRUE;
		}
		
		// top is 0
		if (blockIdx.x == 0) {
			from[idx] = 0;
			to[idx] = 0;
			locked[idx] = TRUE;
		}
		
		// bottom is 100
		if (blockIdx.x == NUM_ROWS-1) {
			from[idx] = 100;
			to[idx] = 100;
			locked[idx] = TRUE;
		}
	}
}

__global__ void resetKeepgoing(int *lkeepgoing, int *keepgoing) {
	lkeepgoing[threadIdx.x] = 1;
	if (threadIdx.x == 0)
		*keepgoing = 0;
}


__global__ void calculate(float *from, float *to, float* locked, int size, int *lkeepgoing) {
	int idx = blockDim.x * blockIdx.x + threadIdx.x;
	float total, self;
	
	if (idx < size) {
		if (locked[idx] == TRUE) {
			return;
		}
		total = from[idx - NUM_COLS] + from[idx-1] + from[idx + 1] + from[idx + NUM_COLS];
		self = from[idx];
		
		to[idx] = (total + 4 * self) * 0.125;
		
		// Set the keepgoing data for the block
		if ((fabs(self - (total)/4) < EPSILON)) {
			lkeepgoing[blockIdx.x] = 0;
		}
		
	}
}

__global__ void reduceSingle(int *lkeepgoing, int *keepgoing)
{
    extern __shared__ int sdata[];
	unsigned int tid, i, s;

    // Calculate our offset into the row
	int rowStartPos = threadIdx.x * (NUM_ROWS/blockDim.x); //number of rows / number of threads in a block
    // The number of cols per thread
    int colsPerThread = NUM_ROWS/blockDim.x; //number of rows / block dimention = 8k/1024 = 8

    // perform first level of reduction,
    // reading from global memory, writing to shared memory
    tid = threadIdx.x;

	// Sum my part of 1D array and put it in shared memory
	// Method 1
	sdata[tid] = 0;
	for (i = rowStartPos; i < colsPerThread+rowStartPos; i++) {
		sdata[tid] += lkeepgoing[i];
	}
	__syncthreads();
//    i = blockIdx.x*(blockDim.x*2) + threadIdx.x;
//    sdata[tid] = lkeepgoing[i] & lkeepgoing[i+blockDim.x];
//    __syncthreads();

	if (tid < NUM_ROWS / 2) 
	{
        sdata[tid] &= sdata[tid + 4096]; __syncthreads();
        sdata[tid] &= sdata[tid + 2048]; __syncthreads();
        sdata[tid] &= sdata[tid + 1024]; __syncthreads();
        sdata[tid] &= sdata[tid +  512]; __syncthreads();
        sdata[tid] &= sdata[tid +  256]; __syncthreads();
        sdata[tid] &= sdata[tid +  128]; __syncthreads();
        sdata[tid] &= sdata[tid +   64]; __syncthreads();
        sdata[tid] &= sdata[tid +   32]; __syncthreads();
        sdata[tid] &= sdata[tid +   16]; __syncthreads();
        sdata[tid] &= sdata[tid +    8]; __syncthreads();
        sdata[tid] &= sdata[tid +    4]; __syncthreads();
        sdata[tid] &= sdata[tid +    2]; __syncthreads();
        sdata[tid] &= sdata[tid +    1]; __syncthreads();
	}

	// Method 2
//	sdata[tid] = 0;
//	for (i = tid; i < NUM_ROWS; i+=blockDim.x) { // everyone will start in a chunk together grabbing a chunk at a time and processing it later
//		sdata[threadIdx.x] += lkeepgoing[i];
//	}
//	__syncthreads();
//
//	if (tid < NUM_ROWS / 2) 
//	{
//        sdata[tid] &= sdata[tid + 4096]; __syncthreads();
//        sdata[tid] &= sdata[tid + 2048]; __syncthreads();
//        sdata[tid] &= sdata[tid + 1024]; __syncthreads();
//        sdata[tid] &= sdata[tid +  512]; __syncthreads();
//        sdata[tid] &= sdata[tid +  256]; __syncthreads();
//        sdata[tid] &= sdata[tid +  128]; __syncthreads();
//        sdata[tid] &= sdata[tid +   64]; __syncthreads();
//        sdata[tid] &= sdata[tid +   32]; __syncthreads();
//        sdata[tid] &= sdata[tid +   16]; __syncthreads();
//        sdata[tid] &= sdata[tid +    8]; __syncthreads();
//        sdata[tid] &= sdata[tid +    4]; __syncthreads();
//        sdata[tid] &= sdata[tid +    2]; __syncthreads();
//        sdata[tid] &= sdata[tid +    1]; __syncthreads();
//	}

      // Method 3
//    i = blockIdx.x*(blockDim.x*2) + threadIdx.x;
//    sdata[tid] = lkeepgoing[i] & lkeepgoing[i+blockDim.x];
//    __syncthreads();
//	
//    // do reduction in shared memory
//    for(s=blockDim.x/2; s>32; s>>=1)
//    {
//        if (tid < s)
//        {
//            sdata[tid] &= sdata[tid + s];
//        }
//        __syncthreads();
//    }
//	
//    if (tid < 32)
//    {
//        sdata[tid] &= sdata[tid + 32]; __syncthreads();
//        sdata[tid] &= sdata[tid + 16]; __syncthreads();
//        sdata[tid] &= sdata[tid +  8]; __syncthreads();
//        sdata[tid] &= sdata[tid +  4]; __syncthreads();
//        sdata[tid] &= sdata[tid +  2]; __syncthreads();
//        sdata[tid] &= sdata[tid +  1]; __syncthreads();
//    }
	
    // write result for this block to global mem
    if (tid == 0) *keepgoing = sdata[0];
}



int main(void) {
	double timestart, timefinish, timetaken; // host data
	float *from_d, *to_d, *locked;	// device data
	float *temp_d;
	int *lkeepgoing, *keepgoing;	// more device data
	int nBytes;
	int iterations;
	int SIZE, blocks, threadsperblock;
	int *steadyState;

	SIZE = NUM_ROWS * NUM_COLS;
	blocks = 8192;
	threadsperblock = 8192;

	steadyState = (int*)malloc(sizeof(int));
	*steadyState = 0;

	nBytes = SIZE*sizeof(float);
	cudaMalloc((void **) &from_d, nBytes);
	cudaMalloc((void **) &to_d, nBytes);
	cudaMalloc((void **) &locked, nBytes);
	cudaMalloc((void **) &lkeepgoing, blocks * sizeof(int));
	cudaMalloc((void **) &keepgoing, sizeof(int));

	initArrays<<<blocks,threadsperblock>>> (from_d, to_d, locked, SIZE);
	
	iterations = 0;
	timestart = When();
	while (!*steadyState) {//&& TOTITERATIONS != iterations) {
		
		resetKeepgoing<<<1,blocks>>> (lkeepgoing, keepgoing);
		calculate<<<blocks,threadsperblock>>> (from_d, to_d, locked, SIZE, lkeepgoing);
		reduceSingle<<<1,blocks, blocks*sizeof(int)>>> (lkeepgoing, keepgoing);
	
		cudaMemcpy(steadyState, keepgoing, sizeof(int), cudaMemcpyDeviceToHost);

		iterations++;
		temp_d = from_d;
		from_d = to_d;
		to_d = temp_d;
		
		printf("Iteration %d\n", iterations);
	}

	timefinish = When();
	float* plate = (float*)malloc(sizeof(float) * SIZE);
	cudaMemcpy(plate, to_d, sizeof(float)*SIZE, cudaMemcpyDeviceToHost);

	cudaFree(from_d); 
	cudaFree(lkeepgoing);
	cudaFree(keepgoing);
	free(steadyState);

	timetaken = timefinish - timestart;

	printf("Iteration %d time %f\n", iterations, timetaken);
	int k;
	for (k = 0; k < SIZE; k++) {
		if (k % 8191 == 0) {
			printf("\n");
		}
		printf("%d\t", plate[k]);
	}
	
	cudaFree(to_d);
	free(plate);

	return 0;
}
