/*
 * File:            mandelbrot_set.c
 * Author:          Artem Golotin
 * File location:   https://github.com/agolotin/CS484/blob/master/cpp/mandelbrot.cpp
 */

#include <omp.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <float.h>

#define TRUE 1
#define FALSE 0
#define SIZE 28000
#define _step 100

////////////////////////////////////////////////////////////////////////////////

const int MAX_WIDTH_HEIGHT = 30000;
const int HUE_PER_ITERATION = 5;
const int DRAW_ON_KEY = TRUE;


////////////////////////////////////////////////////////////////////////////////

typedef struct State {
	double centerX;
	double centerY;
	double zoom;
	int maxIterations;
	int w;
	int h;
} _state;

typedef struct Info {
	int start;
	int end;
	int bin_size;
	int terminate;
} _info;


typedef struct Locations {
	long long location;
	unsigned char r; //loc+2
	unsigned char g; //loc+1
	unsigned char b; //loc+0
} _loc;

struct State* state_new() {
	//centerX = -.75;
	//centerY = 0;
	struct State* st = (struct State*)malloc(sizeof(struct State));

	st->centerX = -1.186340599860225;
	st->centerY = -0.303652988644423;
	st->zoom = 1;
	st->maxIterations = 100;
	st->w = SIZE;
	st->h = SIZE;

	return st;
}

////////////////////////////////////////////////////////////////////////////////
float iterationsToEscape(double, double, int);
int hue2rgb(float);
void writeImage(unsigned char*, int, int);
void master(struct State*);
void worker(struct State*);
unsigned char *createImage(struct State);
void draw(struct State*);
int* makeBins();
void createMPILocationType();
void createMPIInfoType();
////////////////////////////////////////////////////////////////////////////////

//========================= DECLARE MPI DATATYPES ====================== ||
MPI_Datatype MPI_Location_type;
MPI_Datatype MPI_Info_type;

void createMPIInfoType() {
	struct Info* info = (struct Info*)malloc(sizeof(struct Info));

	int i, base, blocks[4] = {1,1,1,1};
	MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
	MPI_Aint disp[4];

	MPI_Address(info, disp);
	MPI_Address(&info->end, disp+1);
	MPI_Address(&info->bin_size, disp+2);
	MPI_Address(&info->terminate, disp+3);

	base = disp[0];

	for (i=0; i<4; i++) disp[i] -= base;

	MPI_Type_struct(4, blocks, disp, types, &MPI_Info_type);
	MPI_Type_commit(&MPI_Info_type);

	free(info);
}

void createMPILocationType() {

	struct Locations* loc = (struct Locations*)malloc(sizeof(struct Locations));

	int i, base, blocks[4] = {1,1,1,1};
	MPI_Datatype types[4] = {MPI_LONG_LONG, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR};
	MPI_Aint disp[4];

	MPI_Address(loc, disp);
	MPI_Address(&loc->r, disp+1);
	MPI_Address(&loc->g, disp+2);
	MPI_Address(&loc->b, disp+3);
	base = disp[0];

	for (i=0; i<4; i++) disp[i] -= base;

	MPI_Type_struct(4, blocks, disp, types, &MPI_Location_type);
	MPI_Type_commit(&MPI_Location_type);

	free(loc);
}

//====================================================================== ||

//============= COMPUTE ITERATIONS TO ESCAPE ================== ||
float iterationsToEscape(double x, double y, int maxIterations) {
    double tempa, i;
    double a = 0;
    double b = 0;
	char abort = FALSE;
	float output = FLT_MAX;

//	#pragma omp parallel for private(i) schedule(dynamic)
    for (i = 0 ; i < maxIterations ; i++) {
//		#pragma omp flush(abort)
		if (!abort) {
			tempa = a*a - b*b + x;
			b = 2*a*b + y;
			a = tempa;
			if (a*a+b*b > 64) {
				abort = TRUE;
	//				#pragma omp flush(abort)
				// return i; // discrete
				output = i - log(sqrt(a*a+b*b))/log(8); //continuous

			}
		}
    }
	if (output != FLT_MAX)
		return output;

    return -1;
}
//============================================================= ||

//======================= WRITE IMAGE ========================= ||
void writeImage(unsigned char *img, int w, int h) {
    long long filesize = 54 + 3*(long long)w*(long long)h;
    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[3] = {0,0,0};

    bmpfileheader[ 2] = (unsigned char)(filesize    );
    bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);

    bmpinfoheader[ 4] = (unsigned char)(       w    );
    bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
    bmpinfoheader[ 6] = (unsigned char)(       w>>16);
    bmpinfoheader[ 7] = (unsigned char)(       w>>24);
    bmpinfoheader[ 8] = (unsigned char)(       h    );
    bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
    bmpinfoheader[10] = (unsigned char)(       h>>16);
    bmpinfoheader[11] = (unsigned char)(       h>>24);

	int i;
    FILE *f;
    f = fopen("temp.bmp","wb");
    fwrite(bmpfileheader,1,14,f);
    fwrite(bmpinfoheader,1,40,f);
    for (i=0; i<h; i++) {
        long long offset = ((long long)w*(h-i-1)*3);
        fwrite(img+offset,3,w,f);
        fwrite(bmppad,1,(4-(w*3)%4)%4,f);
    }
    fclose(f);
}
//============================================================= ||

int* makeBins() {
	int bin = SIZE / _step;
	int i, j, temp;
	int* bins = (int*)malloc(sizeof(int) * _step * 2);
	for (i=0, j=1, temp=1; i < _step*2; i+=2, j+=2, temp=temp+bin) {
		bins[i] = temp-1;
		bins[j] = temp + bin-1;
	}
	return bins;
}


void worker(struct State* state) {
    int nproc, iproc;
	MPI_Request request;
	int bin_size = SIZE / _step;

    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);
	printf("[worker %d] Waiting for workload to come\n", iproc);
	struct Info* info = (struct Info*)malloc(sizeof(struct Info));
	struct Locations* loc = (struct Locations*)malloc(sizeof(struct Locations) * bin_size * bin_size);

	for (;;) {
		MPI_Recv(info, 1, MPI_Info_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if (info->terminate) break;

		int start = info->start;
		int end = info->end;
		printf("[worker %d] Received pixels from %d to %d\n", iproc, info->start, info->end);

		int px, py, q = 0;
		double xs[MAX_WIDTH_HEIGHT], ys[MAX_WIDTH_HEIGHT];

		for (px=start; px<end; px++) {
			xs[px] = (px - state->w/2)/state->zoom + state->centerX;
		}
		for (py=start; py<end; py++) {
			ys[py] = (py - state->h/2)/state->zoom + state->centerY;
		}

    	unsigned char r, g, b;
		for (px=start; px<end; px++) {
			for (py=start; py<end; py++) {
				r = g = b = 0;
				float iterations = iterationsToEscape(xs[px], ys[py], state->maxIterations);
				if (iterations != -1) {
					float h = HUE_PER_ITERATION * iterations;
					r = hue2rgb(h + 120);
					g = hue2rgb(h);
					b = hue2rgb(h + 240);
				}
				long long location = ((long long)px+(long long)py*(long long)state->w)*3;
				loc[q].location = location;
				loc[q].r = (unsigned char)(r); //loc+2
				loc[q].g = (unsigned char)(g); //loc+1
				loc[q].b = (unsigned char)(b); //loc+0
				q++;
			}
		}
		printf("[worker %d] Sending processed image pixels back to the master\n", iproc);
		MPI_Isend(loc, info->bin_size * info->bin_size, MPI_Location_type, 0, 0, MPI_COMM_WORLD, &request);
	}

	free(info);
	free(loc);

}

unsigned char *masterCreateImage(struct State* state) {
    int nproc;
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if (state->w > MAX_WIDTH_HEIGHT) state->w = MAX_WIDTH_HEIGHT;
    if (state->h > MAX_WIDTH_HEIGHT) state->h = MAX_WIDTH_HEIGHT;

    unsigned char *img = NULL;
    if (img) free(img);

	long long size = (long long)state->w*(long long)state->h*3;
    img = (unsigned char *)malloc(size);

	int i, j, k, terminatedIndex = 0;
	int* bins = makeBins();
	int message_count = 0;

	printf("[master] About to send the initial workload to every processor\n");
	struct Info* info = (struct Info*)malloc(sizeof(struct Info));
	info->bin_size = SIZE / _step;

	//Send out initial workloads
	MPI_Request request;
	for (i=0, j=1, k=1; k < nproc; i+=2, j+=2, k++) {
		info->start = bins[i];
		info->end = bins[j];
		info->terminate = FALSE;
		printf("[master] Sending initial workload, rows from %d to %d to worker %d\n", info->start, info->end, k);
		MPI_Isend(info, 1, MPI_Info_type, k, 0, MPI_COMM_WORLD, &request);
		message_count++;
	}

	struct Locations* loc = (struct Locations*)malloc(sizeof(struct Locations) * info->bin_size * info->bin_size);
	int* terminated = (int*)malloc(sizeof(int)*nproc);
	for (;;) {
		MPI_Status status;

		printf("[master] Waiting for incoming messages with data\n");
		MPI_Recv(loc, info->bin_size * info->bin_size, MPI_Location_type, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
		message_count--;

		printf("[master] Received from %d recently calculated image pixels. Message count is %d\n", 
				status.MPI_SOURCE, message_count);
		int r;
		for (r = 0; r < info->bin_size*info->bin_size; r++) {
			img[loc[r].location+2] = loc[r].r;
			img[loc[r].location+1] = loc[r].g;
			img[loc[r].location+0] = loc[r].b;
		}

		if (k < _step+1) {
			//Send out more work
			info->start = bins[i];
			info->end = bins[j];
			info->terminate = FALSE;
			printf("[master] Sending more work to %d, start=%d, end=%d, i=%d, j=%d\n", 
					status.MPI_SOURCE, info->start, info->end, i, j);

			MPI_Isend(info, 1, MPI_Info_type, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &request);
			message_count++;
			i += 2;
			j += 2;
		}
		else {
			printf("[master] Sending initial termination message to %d, k=%d\n", status.MPI_SOURCE, k);
			//Send termination message
			info->terminate = TRUE;
			terminated[terminatedIndex] = status.MPI_SOURCE;
			terminatedIndex++;
			MPI_Isend(info, 1, MPI_Info_type, status.MPI_SOURCE, 0, MPI_COMM_WORLD, &request);
		}

		if (k >= _step+1 && message_count == 0) break; //termination should occur
		k++;
	}

	//send out termination messages just in case 
	//info->terminate = TRUE;
	//for(k=1; k<nproc; k++) {
	//	int u, send = TRUE;
	//	for (u=0; u<terminatedIndex+1; u++) {
	//		if (k == terminated[u]) send = FALSE;
	//	}
	//	if (send) {
	//		printf("[master] Sending termination message to %d\n", k);
	//		MPI_Isend(info, 1, MPI_Info_type, k, 0, MPI_COMM_WORLD, &request);
	//	}
	//}

	printf("[master] I'm free!!\n");

	free(loc);
	free(bins);
	free(info);
	free(terminated);

    return img;
}

int hue2rgb(float t){
    while (t>360) {
        t -= 360;
    }
    if (t < 60) return 255.*t/60.;
    if (t < 180) return 255;
    if (t < 240) return 255. * (4. - t/60.);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

void draw(struct State* state) {
    unsigned char *img = masterCreateImage(state);
//    writeImage(img, state->w, state->h);
	free(img);
}

int main(int argc, char *argv[]) {
	setbuf(stdout, 0);
    int iproc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &iproc);

	//Create custom MPI struct datatypes
	createMPILocationType();
	createMPIInfoType();

    struct State* state = state_new();

	if (iproc == 0) draw(state);
	else worker(state);

	MPI_Type_free(&MPI_Location_type);
	MPI_Type_free(&MPI_Info_type);
	free(state);

    MPI_Finalize();
	return EXIT_SUCCESS;
}
