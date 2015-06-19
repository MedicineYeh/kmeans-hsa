// ------------------------------------
// variable
float data[N_DCNT][N_DIM]; /* 原始資料   */
float cent[N_K][N_DIM];    /* 重心       */
float dis_k[N_K][N_DIM];   /* 叢聚距離   */
int table[N_DCNT];       /* 資料所屬叢聚*/
int cent_c[N_K];         /* 該叢聚資料數*/

// ------------------------------------
// 計算二點距離
float cal_dis(float *x, float *y, int dim)
{
    int i;
    float t, sum=0.0;
    for(i=0; i<dim; ++i)
        t=x[i]-y[i], sum+=t*t;
    return sum;
}

float update_table(int* ch_pt)
{
    int i, j, k, min_k;
    float dis, min_dis, t_sse=0.0;

    *ch_pt=0;                          // 變動點數設0
    memset(cent_c, 0, sizeof(cent));   // 各叢聚資料數清0
    memset(dis_k, 0, sizeof(dis_k));   // 各叢聚距離和清0

    for(i=0; i<N_DCNT; ++i){
        // 尋找所屬重心
        min_dis = cal_dis(data[i], cent[0], N_DIM);
        min_k   = 0;
        for(k=1;k<N_K; ++k){
            dis = cal_dis(data[i], cent[k], N_DIM);
            if(dis < min_dis) 
                min_dis=dis, min_k = k;
        }
        *ch_pt+=(table[i]!=min_k); // 更新變動點數
        table[i] = min_k;          // 更新所屬重心
        ++cent_c[min_k];           // 累計重心資料數         
        t_sse += min_dis;          // 累計總重心距離
        for(j=0; j<N_DIM; ++j)       // 更新各叢聚總距離
            dis_k[min_k][j]+=data[i][j];         
    }
    return t_sse;
}


void kmeans_main()
{
    int    ch_pt = 0;      /* 紀錄變動之點 */
    int    iter = 0;       /* 迭代計數器   */
    float  sse1 = 0.0;     /* 上一迭代之sse */
    float  sse2 = 0.0;     /* 此次迭代之sse */

    //C
    tic(&timer_1);
    sse2 = update_table(&ch_pt); 
    do {
        sse1 = sse2, ++iter;
        update_cent();                  // step 3 - 更新重心
        sse2=update_table(&ch_pt);  // step 4 - 更新對應表
    }while(iter < MAX_ITER && sse1 != sse2 && ch_pt > MIN_PT); // 收斂條件
    toc("CPU Execution Time: ", &timer_1, &timer_2);
    print_cent(); // 顯示最後重心位置
    printf("sse   = %.2lf\n", sse2);
    printf("ch_pt = %d\n", ch_pt);
    printf("iter  = %d\n", iter);
}


