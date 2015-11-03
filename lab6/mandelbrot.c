/*
 * File:            mandelbrot_set.c
 * Author:          Artem Golotin
 * File location:   https://github.com/agolotin/CS484/blob/master/cpp/mandelbrot.cpp
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>


////////////////////////////////////////////////////////////////////////////////

const int MAX_WIDTH_HEIGHT = 30000;
const int HUE_PER_ITERATION = 5;
const bool DRAW_ON_KEY = true;

////////////////////////////////////////////////////////////////////////////////

class State {
    public:
        double centerX;
        double centerY;
        double zoom;
        int maxIterations;
        int w;
        int h;
        State() {
            //centerX = -.75;
            //centerY = 0;
            centerX = -1.186340599860225;
            centerY = -0.303652988644423;
            zoom = 1;
            maxIterations = 100;
            w = 28000;
            h = 28000;
        }
};

////////////////////////////////////////////////////////////////////////////////
float iterationsToEscape(double, double, int);
int hue2rgb(float);
void writeImage(unsigned char*, int, int);
unsigned char *createImage(State);
void draw(State);

//============= COMPUTE ITERATIONS TO ESCAPE ================== ||
float iterationsToEscape(double x, double y, int maxIterations) {
    double tempa;
    double a = 0;
    double b = 0;
    for (int i = 0 ; i < maxIterations ; i++) {
        tempa = a*a - b*b + x;
        b = 2*a*b + y;
        a = tempa;
        if (a*a+b*b > 64) {
            // return i; // discrete
            return i - log(sqrt(a*a+b*b))/log(8); //continuous
        }
    }
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

    FILE *f;
    f = fopen("temp.bmp","wb");
    fwrite(bmpfileheader,1,14,f);
    fwrite(bmpinfoheader,1,40,f);
    for (int i=0; i<h; i++) {
        long long offset = ((long long)w*(h-i-1)*3);
        fwrite(img+offset,3,w,f);
        fwrite(bmppad,1,(4-(w*3)%4)%4,f);
    }
    fclose(f);
}
//============================================================= ||


unsigned char *createImage(State state) {
    int w = state.w;
    int h = state.h;

    if (w > MAX_WIDTH_HEIGHT) w = MAX_WIDTH_HEIGHT;
    if (h > MAX_WIDTH_HEIGHT) h = MAX_WIDTH_HEIGHT;

    unsigned char r, g, b;
    unsigned char *img = NULL;
    if (img) free(img);
    long long size = (long long)w*(long long)h*3;
    printf("Malloc w %zu, h %zu,  %zu\n",w,h,size);
    img = (unsigned char *)malloc(size);
    printf("malloc returned %X\n",img);

    double xs[MAX_WIDTH_HEIGHT], ys[MAX_WIDTH_HEIGHT];
    for (int px=0; px<w; px++) {
        xs[px] = (px - w/2)/state.zoom + state.centerX;
    }
    for (int py=0; py<h; py++) {
        ys[py] = (py - h/2)/state.zoom + state.centerY;
    }

    for (int px=0; px<w; px++) {
        for (int py=0; py<h; py++) {
            r = g = b = 0;
            float iterations = iterationsToEscape(xs[px], ys[py], state.maxIterations);
            if (iterations != -1) {
                float h = HUE_PER_ITERATION * iterations;
                r = hue2rgb(h + 120);
                g = hue2rgb(h);
                b = hue2rgb(h + 240);
            }
            long long loc = ((long long)px+(long long)py*(long long)w)*3;
            img[loc+2] = (unsigned char)(r);
            img[loc+1] = (unsigned char)(g);
            img[loc+0] = (unsigned char)(b);
        }
    }
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

void draw(State state) {
    unsigned char *img = createImage(state);
    writeImage(img, state.w, state.h);
}

int main(int argc, char *argv[]) {
	setbuf(stdout, 0);
    MPI_Init(&argc, &argv);

    State state;
    draw(state);

    MPI_Finalize();
	return EXIT_SUCCESS;
}
