#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#define SIZE 15


double When() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double) tp.tv_sec + (double) tp.tv_usec * 1e-6);
}

int* createArray() {
    int i, iproc;
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

	int* values = (int*)malloc(sizeof(int) * SIZE);
	srand(time(0));
	for (i = 0; i < SIZE; i++) {
		values[i] = abs(rand()) % SIZE;
	}
	return values;
}

void swap(int* a, int* b) {
	int temp;
	temp = *b;
	*b = *a;
	*a = temp;
}

void quicksort(int l, int r, int* values) {
	int j;
	int pivot = values[l];                              
	int i = l + 1;                                      
	if (l + 1 < r)                                      
	{                                                   
		for (j = l + 1; j <= r; j++)                
		{                                               
			if (values[j] < pivot)                      
			{                                           
				swap(&values[j], &values[i]);             
				i++;                                    
			}                                           
		}                                               
		swap(&values[i - 1], &values[l]);                 
		quicksort(l, i, values);                                
		quicksort(i, r, values);                                
	}                                                   
}                                                       

int medianOfMedians(int *a, int s, int e, int k){
    // if the partition length is less than or equal to 5
    // we can sort and find the kth element of it
    // this way we can find the median of n/5 partitions
    if(e-s+1 <= 5){
        //sort(a+s, a+e);
        return s+k-1;
    }
    
    // if array is bigger 
    // we partition the array in subarrays of size 5
    // no. of partitions = n/5 = (e+1)/5
    // iterate through each partition
    // and recursively calculate the median of all of them
    // and keep putting the medians in the starting of the array
	int i;
    for(i=0; i<(e+1)/5; i++){
        int left = 5*i;
        int right = left + 4;
        if(right > e) right = e;
        int median = medianOfMedians(a, 5*i, 5*i+4, 3);
        swap(&a[median], &a[i]);
    }
    
    // now we have array 
    // a[0] = median of 1st 5 sized partition
    // a[1] = median of 2nd 5 sized partition
    // and so on till n/5
    // to find out the median of these n/5 medians
    // we need to select the n/10th element of this set (i.e. middle of it)
    return medianOfMedians(a, 0, (e+1)/5, (e+1)/10);
}

void main(int argc, char *argv[]) {
//	setbuf(stdout, 0);
    MPI_Init(&argc, &argv);
	char host[255];
	gethostname(host,253);

    int nproc, iproc;
	int tempNproc, tempIproc;
    MPI_Status status;
	MPI_Request request;

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
	
	int* values = createArray();

	int dimentions = ceil(log2(nproc));
	MPI_Comm comms[dimentions + 1];
	int mask = pow(2, (dimentions-1)) - 1; //if 2 dimentions then 011
	int maskPlusOne = mask + 1;   // if 2 dimentions then 100
	MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &comms[0]);

	int i;
	int curSize = SIZE;

	int p;
	printf("[%d] ", iproc);
	for (p = 0; p < SIZE; p++)
		printf("%d ", values[i]);
	printf("\n");

	for (i = dimentions; i > 0; i--) {
		MPI_Comm_size(comms[dimentions-i], &tempNproc);
		MPI_Comm_rank(comms[dimentions-i], &tempIproc);

		int pivotValue = 5;
		int arraySize = curSize;

		if (tempIproc == 0 && arraySize > 0) {
			pivotValue = values[(int)ceil(arraySize/2)];
		}

		MPI_Bcast(&pivotValue, 1, MPI_INT, 0, comms[dimentions-i]);
		int k;
		for (k = 0; k < arraySize; k++) {
			if (values[k] >= pivotValue)
				break;
		}

		int otherHalf = arraySize - k;
		int incomingAmount, otherNode;

		if ((tempIproc & maskPlusOne) != 0) { //sends the items below k 
			otherNode = tempIproc - maskPlusOne; //source node where we get an incoming array size
			MPI_Send(&k, 1, MPI_INT, otherNode, 0, comms[dimentions - i]);
			MPI_Recv(&incomingAmount, 1, MPI_INT, otherNode, 0, comms[dimentions-i], &status);

			int incomingArray[incomingAmount];
			MPI_Send(values, k, MPI_INT, otherNode, 0, comms[dimentions-i]);
			MPI_Recv(incomingArray, incomingAmount, MPI_INT, otherNode, 0, comms[dimentions-i], &status);
			
			int myArrHalf[otherHalf];
			int j;
			for (j = 0; j < otherHalf; j++) {
				myArrHalf[j] = values[j+1];
			}
			int newNumberSize = incomingAmount + otherHalf;
			curSize = newNumberSize;
			values = (int*)malloc(sizeof(int) * (curSize));
			int incomingIndex = 0;
			int myArrIndex = 0;
			for (j = 0; j < (incomingAmount + otherHalf); j++) {
				if (myArrIndex == otherHalf) {
					values[j] = incomingArray[incomingIndex];
					incomingIndex++;
				}
				else if (incomingIndex == incomingAmount) {
					values[j] = myArrHalf[myArrIndex];
					myArrIndex++;
				}
				else if (myArrHalf[myArrIndex] <= incomingArray[incomingIndex]) {
					values[j] = myArrHalf[myArrIndex];
					myArrIndex++;
				}
				else {
					values[j] = incomingArray[incomingIndex];
					incomingIndex++;
				}
			}
		}
		else { //send the items above k
			otherNode = tempIproc + maskPlusOne;
			MPI_Send(&otherHalf, 1, MPI_INT, otherNode, 0, comms[dimentions-i]);
			MPI_Recv(&incomingAmount, 1, MPI_INT, otherNode, 0, comms[dimentions-i], &status);

			int incomingArray[incomingAmount];
			MPI_Send((values+k), otherHalf, MPI_INT, otherNode, 0, comms[dimentions-i]);
			MPI_Recv(incomingArray, incomingAmount, MPI_INT, otherNode, 0, comms[dimentions-i], &status);

			int myArrHalf[k];
			int j;
			for (j = 0; j < k; j++) {
				myArrHalf[j] = values[j];
			}
			int newNumberSize = k + incomingAmount;
			curSize = newNumberSize;
			values = (int*)malloc(sizeof(int) * curSize);
			int incomingIndex = 0;
			int myArrIndex = 0;
			for (j = 0; j < (incomingAmount + k); j++) {
				if (myArrIndex == sizeof(myArrHalf) / sizeof(int)) {
					values[j] = incomingArray[incomingIndex];
					incomingIndex++;
				}
				else if (incomingIndex == sizeof(incomingArray) / sizeof(int)) {
					values[j] = myArrHalf[myArrIndex];
					myArrIndex++;
				}
				else if (myArrHalf[myArrIndex] <= incomingArray[incomingIndex]) {
					values[j] = myArrHalf[myArrIndex];
					myArrIndex++;
				}
				else {
					values[j] = incomingArray[incomingIndex];
					incomingIndex++;
				}
			}
		}

		printf("[%d] Dimention %d. Values: ", iproc, i);
		int q;
		for (q = 0; q < curSize; q++) {
			printf("%d, ", values[q]);
		}
		printf("\n");
		int dimentionBit;
		if ((tempIproc & maskPlusOne) == 0){
			dimentionBit = 0;
		}
		else {
			dimentionBit = 1;
		}
		MPI_Comm_split(comms[dimentions-i], dimentionBit, 0, &comms[dimentions-i+1]);
		maskPlusOne = maskPlusOne >> 1;
		mask = mask >> 1;
	}
	
    MPI_Finalize();
}
