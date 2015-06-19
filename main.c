#include <stdio.h>
#include <stdlib.h>    
#include <stdint.h>
#include <string.h>
#include <time.h>

// param define
#define N_DCNT     100000 /* 資料個數   */
#define N_DIM      10   /* 資料維度   */
#define N_K        10   /* 叢聚個數   */
#define MAX_ITER 20  /* 最大迭代   */
#define MIN_PT   0   /* 最小變動點 */
#define LOW      20  /* 資料下限   */
#define UP       300 /* 資料上限   */

// ------------------------------------
// function declare
void get_data();               // 取得資料
void load_data(char *filename);
void kmeans_init();            // 演算法初始化
void update_cent();
void print_cent();
static inline void tic(struct timespec *t1);
static inline void toc(char *str, struct timespec *t1, struct timespec *t2);


// ------------------------------------
// different version of kmeans
#ifdef __USE_SNACK__
//此行是snack.sh hw.cl 所產生的  必須加入！！！！！！ hw.h
#include "shader_hsa.h"
#include "snack_version.c"
#endif

#ifdef __USE_HSA__
#include "hsa.h"
#include "hsa_ext_finalize.h"
#include "hsa_version.c"
#endif

#ifdef __USE_CL__
#include <CL/cl.h>
#include "cl_version.c"
#endif

#ifdef __USE_C__
#include "c_version.c"
#endif

// ------------------------------------
// main function
int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: %s filename\n\n", argv[0]);
        return 1;
    }

#ifdef __USE_RANDOM__
    srand((unsigned)time(NULL)); 
    get_data();                      /* step 0 - 取得資料            */
#else
    srand(100); 
    load_data(argv[1]);
#endif
    kmeans_init();                   /* step 1 - 初始化,隨機取得重心 */
    
    kmeans_main();

    return 0;
}

void load_data(char *filename)
{
    int i, j, tmp;
    FILE *fp = fopen(filename, "rt");

    if (fp == NULL) {printf("file %s not found\n", filename); exit(1);}
    fscanf(fp, "%d,", &tmp);
    if (tmp != N_DCNT) {printf("DCNT does not match (%d, %d)\n", tmp, N_DCNT); exit(1);}
    fscanf(fp, "%d", &tmp);
    if (tmp != N_DIM) {printf("DIM does not match (%d, %d)\n", tmp, N_DIM); exit(1);}
    for(i = 0; i < N_DCNT ; i++) {
        for (j = 0; j < N_DIM; j++) {
            fscanf(fp, "%f,", &data[i][j]);
        }
    }

    fclose(fp);
/*    
    for(i = 0; i < N_DCNT ; i++) {
        for (j = 0; j < N_DIM; j++) {
            printf("%f, ", data[i][j]);
        }
        printf("\n");
    }
*/
}

// ------------------------------------
// 取得資料，此處以隨機給
void get_data()
{
    int i, j;
    for(i=0; i<N_DCNT; ++i)
        for(j=0; j<N_DIM; ++j)
            data[i][j] = LOW + (float)rand()*(UP-LOW) / RAND_MAX;
}

// ------------------------------------
// 演算化初始化
void kmeans_init()
{
    int i, j, k, rnd;
    int pick[N_K];
    // 隨機找N_K 個不同資料點
    for(k=0; k<N_K; ++k){
        rnd = rand() % N_DCNT; // 隨機取一筆
        for(i=0; i<k && pick[i]!=rnd; ++i);
        if(i==k) pick[k]=rnd; // 沒重覆
        else --k; // 有重覆, 再找一次
    }
    // 將N_K 個資料點內容複制到重心cent
    for(k=0; k<N_K; ++k)
        for(j=0; j<N_DIM; ++j)
            cent[k][j] = data[pick[k]][j];
}

// ------------------------------------
// 更新重心位置
void update_cent()
{
    int k, j;
    for(k=0; k<N_K; ++k)
        for(j=0; j<N_DIM; ++j)
            cent[k][j]=dis_k[k][j]/cent_c[k];
}

// ------------------------------------
// 顯示重心位置
void print_cent()
{
    int j, k;
    for(k=0; k<N_K; ++k) {
        for(j=0; j<N_DIM; ++j)
            printf("%6.2lf ", cent[k][j]);
        putchar('\n');
    }
}


static inline void tic(struct timespec *t1)
{
    clock_gettime(CLOCK_REALTIME, t1);
}

static inline void toc(char *str, struct timespec *t1, struct timespec *t2)
{
    long period = 0;

    clock_gettime(CLOCK_REALTIME, t2);
    period = (t2->tv_sec - t1->tv_sec) * 1e9 + t2->tv_nsec - t1->tv_nsec;
    fprintf(stderr, "\033[1;32m"); //Color Code - Green 
    fprintf(stderr, "%s: %0.3lf ms\n\n", str, (double)period / 1000000.0);
    fprintf(stderr, "\033[0;00m"); //Terminate Color Code
}

