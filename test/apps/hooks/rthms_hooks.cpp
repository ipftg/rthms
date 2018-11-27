#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <omp.h>
#include "rthms_hooks.h"

#define LEN 81920
double global_data = 1.1;
double global_dataarr[LEN]={1.1};
double global_bss;
double global_bssarr[LEN];
static double static_var0 = 2.2;
static double static_arr0[LEN]={2.2};
static double static_var1;
static double static_arr1[LEN];

int main(){
  const size_t length = (size_t)LEN;
  double stack_arr[length];
  double *malloc_arr = (double*)malloc(sizeof(double)*length);
  double *calloc_arr = (double*)calloc(sizeof(double),length);
  double *realloc_arr= (double*)realloc(calloc_arr, sizeof(double)*length*2);
  double *valloc_arr = (double*)valloc(sizeof(double)*length);
  double *pvalloc_arr= (double*)pvalloc(sizeof(double)*length);
  double *memalign_arr;
  double *posix_memalign_arr;
  int err = posix_memalign((void**)&posix_memalign_arr, 64, sizeof(double)*length);

#ifdef _OPENMP_
#pragma omp parallel
{
  if(omp_get_thread_num()==(omp_get_num_threads()-1))
    memalign_arr= (double*)memalign(64, sizeof(double)*length);
}
#else
 memalign_arr= (double*)memalign(64, sizeof(double)*length);
#endif
 
  printf("stack_arr %p\n",stack_arr);
  printf("malloc_arr %p\n",malloc_arr);
  printf("calloc_arr %p\n",calloc_arr);
  printf("realloc_arr %p\n",realloc_arr);
  printf("valloc_arr %p\n",valloc_arr);
  printf("pvalloc_arr %p\n",pvalloc_arr);
  printf("memalign_arr %p\n",memalign_arr);
  printf("posix_memalign_arr %p\n",posix_memalign_arr);
  
  //initialize
  size_t i;
#pragma omp parallel for
  for(i=0;i<length;i++){
    global_bssarr[i] = 1.1;
    static_arr1[i]= 2.2;
    stack_arr[i]  = 3.3;
    malloc_arr[i] = 4.4;
    calloc_arr[i] = 5.5;
    realloc_arr[i]= 6.6;
    valloc_arr[i] = 7.7;
    posix_memalign_arr[i] = 8.8;
  }

  //single-threaded access
  for(i=0;i<length;i++){
    static_arr0[i] = static_arr0[i]*0.5;
  }

  double sum = 0.0;
  rthms_hook_open();
#pragma omp parallel for
  for(i=0;i<length;i++){
    sum += global_bssarr[i] + static_arr1[i] + stack_arr[i] + malloc_arr[i] + calloc_arr[i]+
           memalign_arr[i]  + realloc_arr[i] + valloc_arr[i] + posix_memalign_arr[i];
  }
  rthms_hook_close();
  printf("Sum = %f\n", sum);

  
  free(malloc_arr);
  free(pvalloc_arr);
  free(realloc_arr);
  free(valloc_arr);
  free(memalign_arr);
  free(posix_memalign_arr);
  return 0;
}
