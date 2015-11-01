#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


#define STEPS      2000
#define PI25DT      3.141592653598793238462643

__global__ void pithread()

void pi_calc(int intervals){
  int  i;
  double sum, pi;
  sum=0.0;
  for (i=1; i<=intervals; i++){ 
    double x=((double) i-0.5)/ ((double)intervals);
    sum=sum+(4.0/(1.0+x*x));
  }
  pi=sum/intervals;
  printf("CPU - pi %.7f\n", pi);
}

int main(void){
  clock_t cpu_start_time;
  clock_t cpu_end_time;
  int intervals = 100000;

  cpu_start_time = clock();
  pi_calc(intervals);
  cpu_end_time  = clock();

  clock_t cpu_time = cpu_end_time - cpu_start_time;


  double cpu_time_double = (double)cpu_time;

  printf("cpu_time: %f\n", cpu_time_double);
  
  return 0;
}
