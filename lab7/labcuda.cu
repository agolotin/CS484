#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 
#define BLOCKSIZE 1024
#define MAXIT 1
#define TOTROWS		(BLOCKSIZE*8)
#define TOTCOLS		(BLOCKSIZE*8)
#define NOTSETLOC       -1010101 // for cells that are not fixed
#define SETLOC			-1000110
#define EPSILON		0.1

#define QMAX(x,y) (((x) > (y))? (x): (y))


int *lkeepgoing;
float *iplate;
float *oplate;
float *fixed;
float *tmp;
int ncols, nrows;

double When();
void *Compute();


int main(int argc, char *argv[])
{
	double t0, tottime;
	ncols = TOTCOLS;
	nrows = TOTROWS;

	
	cudaMalloc((void **) &lkeepgoing, nrows * ncols * sizeof(int));
	cudaMalloc((void **) &iplate, nrows * ncols * sizeof(float));
	cudaMalloc((void **) &oplate, nrows * ncols * sizeof(float));
	cudaMalloc((void **) &fixed,  nrows * ncols * sizeof(float));
	printf("Memory allocated\n");

	t0 = When();
	/* Now proceed with the Jacobi algorithm */
	Compute();

	tottime = When() - t0;
	printf("Total Time is: %lf sec.\n", tottime);

	return 0;
}

__global__ void InitArrays(float *ip, float *op, float *fp, int *kp, int ncols, int nrows)
{
	int i;
	float *fppos, *oppos, *ippos;
	int *kppos;
	int blockOffset;
	int rowStartPos;
	int colsPerThread;

	// Calculate the offset of the row
    blockOffset = blockIdx.x * ncols; //what block I correspond to
    // Calculate our offset into the row
	rowStartPos = threadIdx.x * (ncols/blockDim.x);
    // The number of cols per thread
    colsPerThread = ncols/blockDim.x;

	ippos = ip + blockOffset + rowStartPos;
	fppos = fp + blockOffset + rowStartPos;
	oppos = op + blockOffset + rowStartPos;
	kppos = kp + blockOffset + rowStartPos;

	for (i = 0; i < colsPerThread; i++) {
		fppos[i] = NOTSETLOC; // Not Fixed
		ippos[i] = 50;
		oppos[i] = 50;
        kppos[i] = 1; // Keep Going
	}

	int idx = blockDim.x * blockIdx.x + threadIdx.x;
    // Insert code to set the rest of the boundary and fixed positions

	//Left and right columns have to be set to 0 
	fp[idx * ncols] = SETLOC;
	ip[idx * ncols] = 0;
	op[idx * ncols] = 0;
	kp[idx * ncols] = 0;

	fp[idx * ncols + (ncols - 1)] = SETLOC;
	ip[idx * ncols + (ncols - 1)] = 0;
	op[idx * ncols + (ncols - 1)] = 0;
	kp[idx * ncols + (ncols - 1)] = 0;

	//top row has to be set to 0
	if (idx == 0) {
		for (i = 0; i < colsPerThread; i++) {
			fppos[i] = SETLOC; // Fixed
			ippos[i] = 0;
			oppos[i] = 0;
			kppos[i] = 0; // Not Keep Going
		}
	}

	//bottom row has to be set to 100
	if (idx == nrows - 1) {
		for (i = 0; i < colsPerThread; i++) {
			fppos[i] = SETLOC; // Fixed
			ippos[i] = 100;
			oppos[i] = 100;
			kppos[i] = 0; // Not Keep Going
		}
	}
}
__global__ void doCalc(float *iplate, float *oplate, int ncols)
{
	/* Compute the 5 point stencil for my region */
	extern __shared__ float matrix[];
	
	int i, j, row, top_row, bottom_row;
	int begin, end;

	int iproc = blockIdx.x;
	int nproc = gridDim.x;

	int nrows = TOTROWS;

	begin = iproc * nrows/nproc;
	end = begin + nrows/nproc;

	float* shared_rowup = &matrix[0];
	float* shared_rowcur = &matrix[TOTCOLS];
	float* shared_rowdown = &matrix[2*TOTCOLS];

	// Adjust boundary values
	if (iproc == 0) {
		begin = begin + 1;
	}
	if (iproc == (nproc - 1)) {
		end = nrows - 1;
	}

	// Load top and current prev row to global memory
	// Bottom row can only be accessed later
	for (j = threadIdx.x; j < ncols; j += blockDim.x) {
		shared_rowup[j] = oplate[(begin-1)*ncols + j];
		shared_rowcur[j] = oplate[begin * ncols + j];
	}


	for (i = begin; i < end; i++) {
		row = i * ncols;
		top_row = (i-1) * ncols;
		bottom_row = (i+1) * ncols;
		for (j = threadIdx.x; j < ncols; j += blockDim.x) {
			shared_rowdown[j] = oplate[bottom_row + j];
		}
		__syncthreads();

		//perform calculation from shared memory
		for (j = threadIdx.x; j < ncols; j += blockDim.x) {
			if (j > 0 && j < ncols - 1) {
				iplate[row + j] = (4 * shared_rowcur[j] + shared_rowup[j] + shared_rowdown[j] 
								+ shared_rowcur[j-1] + shared_rowcur[j+1]) * 0.125;
			}
		}
	}
}

__global__ void doCheck(float *iplate, float *oplate, float *fixed, int *lkeepgoing, int ncols)
{
	// Calculate keepgoing array
	int i, j, row, top_row, bottom_row;
	float *cur_pos_val, *up_val, *down_val;
	int begin, end;
	int nrows;

	float maxerror, error;

	extern __shared__ float* maxerrorlist;

	int iproc = blockIdx.x;
	int nproc = gridDim.x;

	ncols = TOTCOLS;
	nrows = TOTROWS;

	begin = iproc * nrows/nproc;
	end = begin + nrows/nproc;

	// Adjust boundary values
	if (iproc == 0)
		begin = begin + 1;
	if (iproc == (nproc - 1))
		end = nrows - 1;

	for (i = begin; i < end; i++)
	{
		row = i * ncols;
		top_row = (i-1) * ncols;
		bottom_row = (i+1) * ncols;


		cur_pos_val = &(iplate[row + threadIdx.x]);
		up_val =  &(iplate[top_row+ threadIdx.x]);
		down_val =  &(iplate[bottom_row + threadIdx.x]);

		for (j = threadIdx.x; j < ncols && maxerror <= EPSILON; j+=blockDim.x)
		{
			if (fixed[row + j] == NOTSETLOC)
			{
				error = *currpos - (*upptr + *dnptr + *(currpos -1) + *(currpos +1)) * 0.25;
				
				if (maxerrorlist[threadIdx.x] < error) {
					maxerrorist[threadIdx.x] = error;
				{
			}


		}
	}
	// do reduction in shared mem
	int q;
	for(q = blockDim.x/2; q > 0; q >>= 1) //I don't know if it's gonna work...
	{
		if (threadIdx.x < q)
		{
			if (maxerrorlist[threadIdx.x] > maxerrorlist[threadIdx.x+q]) {
				maxerrorlist[threadIdx.x] = maxerrorlist[threadIdx.x+q];
			}
		}
		__syncthreads();
	}
	maxerror = maxerrorlist[0];

	if (threadIdx.x == 0)
	{
		/* If the maxerror > MAXERROR allowed, I must keep going */
		if (maxerror > EPSILON)
			lkeepgoing[iproc] = 1;
		else 
			lkeepgoing[iproc] = 0;
	
		//fprintf(stderr,"%d: maxerror %f -- stopped checking at [%d, %d]\n", iproc, maxerror, i, j);
	}
}

//__global__ void reduceSingle(int *idata, int *single, int nrows) //bad 
//{
//	int i;
//	if(threadIdx.x == 0) {
//		*single = 0;
//		for(i = 0; i < nrows; i++) {
//			*single += idata[i];
//		}
//		printf(" end %d\n",*single);
//	}
//}

//__global__ void reduceSingleSequentialAddressing(int *idata, int *single, int nrows)
//{
//	// Reduce rows to the first element in each row
//	int i;
//    int rowStartPos;
//    int colsPerThread;
//	extern __shared__ int parts[]; // shared array that holds temporary sums
//	
//    // A block gets a row, each thread will reduce part of a row
//
//    // Calculate our offset into the row
//	rowStartPos = threadIdx.x * (nrows/blockDim.x); //number of rows / number of threads in a block
//    // The number of cols per thread
//    colsPerThread = nrows/blockDim.x; //number of rows / block dimention = 8k/1024 = 8
//
//	int tid = threadIdx.x;
//	// Sum my part of 1D array and put it in shared memory
//	parts[tid] = 0;
//	for (i = rowStartPos; i < colsPerThread+rowStartPos; i++) {
//		parts[tid] += idata[i];
//	}
//
//	//sequential addressing method
//	if (tid < TOTROWS / 2) { parts[tid] += parts[tid + TOTROWS / 2]; }
//	__syncthreads(); 
//
//	if (tid < TOTROWS / 4) { parts[tid] += parts[tid + TOTROWS / 4]; }
//	__syncthreads(); 
//
//	if (tid < TOTROWS / 8) { parts[tid] += parts[tid + TOTROWS / 8]; }
//	__syncthreads(); 
//
//	if (tid < TOTROWS / 16) { parts[tid] += parts[tid + TOTROWS / 16]; }
//	__syncthreads(); 
//
//	if (tid < TOTROWS / 32) { parts[tid] += parts[tid + TOTROWS / 32]; }
//	__syncthreads(); 
//
//	if(threadIdx.x == 0) {
//		*single = 0;
//		for(i = 0; i < 32; i++) {
//			*single += parts[i];
//		}
//	}
//}


__global__ void reduceSingle(int *idata, int *single, int nrows) {
	// Reduce rows to the first element in each row
	int i;
    int rowStartPos;
    int colsPerThread;
	extern __shared__ int parts[]; // shared array that holds temporary sums
	
    // A block gets a row, each thread will reduce part of a row

    // Calculate our offset into the row
	rowStartPos = threadIdx.x * (nrows/blockDim.x); //number of rows / number of threads in a block
    // The number of cols per thread
    colsPerThread = nrows/blockDim.x; //number of rows / block dimention = 8k/1024 = 8

	// Sum my part of 1D array and put it in shared memory
	parts[threadIdx.x] = 0;
	for (i = threadIdx.x; i < nrows; i+=blockDim.x) { // everyone will start in a chunk together grabbing a chunk at a time and processing it later
		parts[threadIdx.x] += idata[i];
	}

	int tid = threadIdx.x;
	if (tid < TOTROWS / 2) { parts[tid] += parts[tid + TOTROWS / 2]; }
	__syncthreads(); 

	if (tid < TOTROWS / 4) { parts[tid] += parts[tid + TOTROWS / 4]; }
	__syncthreads(); 

	if (tid < TOTROWS / 8) { parts[tid] += parts[tid + TOTROWS / 8]; }
	__syncthreads(); 

	if (tid < TOTROWS / 16) { parts[tid] += parts[tid + TOTROWS / 16]; }
	__syncthreads(); 

	if (tid < TOTROWS / 32) { parts[tid] += parts[tid + TOTROWS / 32]; }
	__syncthreads(); 

	if(threadIdx.x == 0) {
		*single = 0;
		for(i = 0; i < 32; i++) {
			*single += parts[i];
		}
	}
}

__global__ void reduceSum(int *idata, int *odata, unsigned int ncols)
{
	// Reduce rows to the first element in each row
	int i;
    int blockOffset;
    int rowStartPos;
    int colsPerThread;
    int *mypart;
	
    // Each block gets a row, each thread will reduce part of a row

	// Calculate the offset of the row
    blockOffset = blockIdx.x * ncols;
    // Calculate our offset into the row
	rowStartPos = threadIdx.x * (ncols/blockDim.x);
    // The number of cols per thread
    colsPerThread = ncols/blockDim.x;

	mypart = idata + blockOffset + rowStartPos;

	// Sum all of the elements in my thread block and put them 
    // into the first column spot
	for (i = 1; i < colsPerThread; i++) {
		mypart[0] += mypart[i];
	}
	__syncthreads(); // Wait for everyone to complete
        // Now reduce all of the threads in my block into the first spot for my row
	if(threadIdx.x == 0) {
		odata[blockIdx.x] = 0;
		for(i = 0; i < blockDim.x; i++) {
			odata[blockIdx.x] += mypart[i*colsPerThread];
		}
	}
	// We cant synchronize between blocks, so we will have to start another kernel
}
	
void *Compute()
{
	printf("Entered compute\n");
	int *keepgoing_single;
	int *keepgoing_sums;
	int keepgoing;
	int blocksize = BLOCKSIZE;
	int iteration;

	ncols = TOTCOLS;
	nrows = TOTROWS;

	printf("About to init arrays\n");
	// One block per row
	InitArrays<<< nrows, blocksize >>>(iplate, oplate, fixed, lkeepgoing, ncols, nrows);


	float** plate = (float**)malloc(nrows * ncols * sizeof(float));
	cudaMemcpy(plate, iplate, sizeof(float)*nrows*ncols, cudaMemcpyDeviceToHost);
	int q,w;
	for (q = 0; q < nrows; q++) {
		for (w = 0; w < ncols; w++) {
			printf("%f, ", plate[q][w]);
		}
		printf("\n");
	}
	exit(-1);


	cudaMalloc((void **)&keepgoing_single, 1 * sizeof(int));
	keepgoing = 1;
	cudaMalloc((void **)&keepgoing_sums, nrows * sizeof(int));
 	int *peek = (int *)malloc(nrows*sizeof(int));

	for (iteration = 0; (iteration < MAXIT) && keepgoing; iteration++)
	{
		doCalc<<< nrows, blocksize, TOTROWS*TOTCOLS*sizeof(double)>>>(iplate, oplate, ncols);
		doCheck<<< nrows, blocksize >>>(iplate, oplate, fixed, lkeepgoing, ncols);
		reduceSum<<< nrows, blocksize>>>(lkeepgoing, keepgoing_sums, ncols);
		cudaMemcpy(peek, keepgoing_sums, nrows*sizeof(int), cudaMemcpyDeviceToHost);
		printf("after cudaMemcpy \n");
		int i;
 		for(i = 0; i < nrows; i++) {
			printf("%d, ",peek[i]);
		}
		// Now we have the sum for each row in the first column, 
		//  reduce to one value
		double t0 = When();
		int timeit;
	//	for (timeit=0; timeit< 10000; timeit++) {
	//		//reduceSingle<<<1, blocksize>>>(keepgoing_sums, keepgoing_single, nrows); // for the bad one 
	//		reduceSingle<<<1, blocksize, blocksize*sizeof(int)>>>(keepgoing_sums, keepgoing_single, nrows); //third argument is for how much memory to give to dynamic __shared__ array
	//	}
		double endit = When() - t0;
		keepgoing = 0;
		cudaMemcpy(&keepgoing, keepgoing_single, 1 * sizeof(int), cudaMemcpyDeviceToHost);
		printf("keepgoing = %d, time 10000 = %f\n", keepgoing, endit);

		/* swap the new value pointer with the old value pointer */
		tmp = oplate;
		oplate = iplate;
		iplate = tmp;
	}
	free(peek);
	cudaFree(keepgoing_single);
	cudaFree(keepgoing_sums);
	printf("Finished in %d iterations\n", iteration);
}

/* Return the current time in seconds, using a double precision number.       */
double When()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}
