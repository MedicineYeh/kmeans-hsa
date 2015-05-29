#include <stdio.h>
#include <stdlib.h>    
#include <stdint.h>
#include <string.h>
#include <time.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

// param define
#define DCNT     100000 /* 資料個數   */
#define DIM      10   /* 資料維度   */
#define K        10   /* 叢聚個數   */
#define MAX_ITER 20  /* 最大迭代   */
#define MIN_PT   0   /* 最小變動點 */
#define LOW      20  /* 資料下限   */
#define UP       300 /* 資料上限   */

// ------------------------------------
// variable
float data[DCNT][DIM]; /* 原始資料   */
float cent[K][DIM]; /* 重心       */
float centc[K][DIM]; /* 重心       */
float dis_k[K][DIM];   /* 叢聚距離   */
int table[DCNT];        /* 資料所屬叢聚*/
int cent_c[K];          /* 該叢聚資料數*/
float dis_k_c[K][DIM];   /* 叢聚距離   */
int table_c[DCNT];        /* 資料所屬叢聚*/
int cent_c_c[K];          /* 該叢聚資料數*/

// ------------------------------------
// function declare
float cal_dis(float *x, float *y, int dim);
void   get_data();               // 取得資料
void   kmeans_init();            // 演算法初始化
float update_table_cl(int* ch_pt); // 更新table
float update_table_c(int* ch_pt_c); // 更新table
void   update_cent_cl();            // 更新重心位置
void   update_cent_c();
void   print_cent_c();             // 顯示重心位置
void   print_cent_cl();             // 顯示重心位置
void initial_kernel();
static inline void tic(struct timespec *t1);
static inline void toc(char *str, struct timespec *t1, struct timespec *t2);
// ------------------------------------
// main function
int main()
{

    int     ch_pt,ch_pt_c;         /* 紀錄變動之點 */
    int     iter=0,iter_c=0;        /* 迭代計數器   */
    float sse1,sse1_c;           /* 上一迭代之sse */
    float sse2,sse2_c;           /* 此次迭代之sse */
    struct timespec timer_1, timer_2;

    srand((unsigned)time(NULL));     
    get_data();                      /* step 0 - 取得資料            */
    kmeans_init();                   /* step 1 - 初始化,隨機取得重心 */
    /*OpenCL*/
    initial_kernel();
    tic(&timer_1);
    sse2 = update_table_cl(&ch_pt);     /* step 2 - 更新一次對應表      */
    do{
        sse1 = sse2, ++iter;
        update_cent_cl();             /* step 3 - 更新重心            */
        sse2=update_table_cl(&ch_pt); /* step 4 - 更新對應表          */
    }while(iter<MAX_ITER && sse1!=sse2 && ch_pt>MIN_PT); // 收斂條件
    toc("OpenCL Execution Time: ", &timer_1, &timer_2);
    print_cent_cl(); // 顯示最後重心位置
    printf("sse   = %.2lf\n", sse2);
    printf("ch_pt = %d\n", ch_pt);
    printf("iter = %d\n", iter);

    /*C*/
    tic(&timer_1);
    sse2_c=update_table_c(&ch_pt_c); 
    do{
        sse1_c = sse2_c, ++iter_c;
        update_cent_c();             /* step 3 - 更新重心            */
        sse2_c=update_table_c(&ch_pt_c); /* step 4 - 更新對應表          */
    }while(iter_c<MAX_ITER && sse1_c!=sse2_c && ch_pt_c>MIN_PT); // 收斂條件
    toc("CPU Execution Time: ", &timer_1, &timer_2);
    print_cent_c(); // 顯示最後重心位置
    printf("sse_c   = %.2lf\n", sse2_c);
    printf("ch_pt_c = %d\n", ch_pt_c);
    printf("iter_c = %d\n", iter_c);

#ifdef __WINDOWS__
    system("PAUSE");
#endif
    return 0;
}

// ------------------------------------
// 取得資料，此處以隨機給
void get_data()
{
    int i, j;
    for(i=0; i<DCNT; ++i)
        for(j=0; j<DIM; ++j)
            data[i][j] = \
                         LOW + (float)rand()*(UP-LOW) / RAND_MAX;
}
// ------------------------------------
// 演算化初始化
void kmeans_init()
{
    int i, j, k, rnd;
    int pick[K];
    // 隨機找K 個不同資料點
    for(k=0; k<K; ++k){
        rnd = rand() % DCNT; // 隨機取一筆
        for(i=0; i<k && pick[i]!=rnd; ++i);
        if(i==k) pick[k]=rnd; // 沒重覆
        else --k; // 有重覆, 再找一次
    }
    // 將K 個資料點內容複制到重心cent
    for(k=0; k<K; ++k)
        for(j=0; j<DIM; ++j)
        {cent[k][j] = data[pick[k]][j];
            centc[k][j] = data[pick[k]][j];}
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
    printf("\033[1;32m"); //Color Code - Green 
    printf("%s: %0.3lf ms\n\n", str, (double)period / 1000000.0);
    printf("\033[0;00m"); //Terminate Color Code
}

#include "cl_version.c"
#include "c_version.c"
