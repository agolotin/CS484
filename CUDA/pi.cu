#include <stdio.h>
#include <assert.h>
#include <cuda.h>

#define BLOCK_SIZE 5 //number of thread blocks
#define THREAD_NUM 5 //number of threads per block
#define STEP_SIZE 10000

__global__ void calculate(float* sum, int nbin, int step, int nthreads, int nblocks) {
	float x;
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	int i;
	for (i = idx; i < nbin; i += nthreads*nblocks) {
		x = (i+0.5)*step;
		sum[idx] += 4.0/(1.0+x*x);
	}
}

int main(void)
{
	float *sum_device, *sum_host;
	float step = 1.0/STEP_SIZE;

	size_t size = THREAD_NUM * BLOCK_SIZE * sizeof(float);
	sum_host = (float*)malloc(size);
	cudaMalloc((void**) &sum_device, size);
	//set the sum_device to 0
	cudaMemset(sum_device, 0, size);
	int block_size = BLOCK_SIZE;
	int grid_size = THREAD_NUM;
	calculate<<<block_size, grid_size>>>(sum_device, STEP_SIZE, step, THREAD_NUM, BLOCK_SIZE);
	//copy from device to host
	cudaMemcpy(sum_host, sum_device, size, cudaMemcpyDeviceToHost);
	int tid;
	float pi = 0;
	for (tid = 0; tid < THREAD_NUM * BLOCK_SIZE; tid++) 
		pi += sum_host[tid];
	pi *= step;

	printf("PI = %f\n", pi);
	free(sum_host);
	cudaFree(sum_device);

	return EXIT_SUCCESS;
}
