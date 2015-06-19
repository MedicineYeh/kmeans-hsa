// ------------------------------------
// variable
float data[N_DCNT][N_DIM]; /* 原始資料   */
float cent[N_K][N_DIM]; /* 重心       */
float dis_k[N_K][N_DIM];   /* 叢聚距離   */
int table[N_DCNT];        /* 資料所屬叢聚*/
int cent_c[N_K];          /* 該叢聚資料數*/
int cent_c_ker[N_DCNT];
float min_dis[N_DCNT];
int chpt[N_DCNT];

void kmeans_main()
{
    int    ch_pt = 0;      /* 紀錄變動之點 */
    int    iter = 0;       /* 迭代計數器   */
    float  sse1 = 0.0;     /* 上一迭代之sse */
    float  sse2 = 0.0;     /* 此次迭代之sse */
    float  t_sse = 0.0;    /* 此次迭代之sse */
    int i, j;

    tic(&timer_1);
    SNK_INIT_LPARM(lparm, N_DCNT);
    toc("SNACN_K initial Time", &timer_1, &timer_2);

    tic(&timer_1);
    ch_pt=0;
//    memset(cent_c, 0, sizeof(cent_c)); // ?叢?è?
//    memset(dis_k, 0, sizeof(dis_k));   // ?叢?è?
/*
    for (i=0; i<N_DCNT;++i) {
        for(k=0; k<N_DIM;++k) {
            data[i*N_DIM+k]=data[i][k];
        }
    }
    for (i=0; i<N_K;++i) {
        for(k=0; k<N_DIM;++k) {
            cent[i*N_DIM+k]=cent[i][k];
        }
    }
*/
    cal_diskernel((float *)data,(float *)cent,table,N_K,N_DIM,N_DCNT,chpt,cent_c_ker,min_dis,lparm);

    for(i=0;i<N_DCNT;i++) {
        ch_pt+=chpt[i];
        ++cent_c[cent_c_ker[i]];
        t_sse+=min_dis[i];
        for(j=0;j<N_DIM;j++) {
            dis_k[table[i]][j]+=data[i][j];
        }
    }
    sse2=t_sse;


    //    sse2 = update_table_cl(&ch_pt);     /* step 2 - 更新一次對應表      */
    do{
        sse1 = sse2, ++iter;
        update_cent();             /* step 3 - 更新重心            */

        t_sse=0.0;
        ch_pt=0;
        memset(cent_c, 0, sizeof(cent_c)); // ?<84>??<9a>è?
        memset(dis_k, 0, sizeof(dis_k));   // ?<84>??<9a>è?
        /*
        for (i=0; i<N_DCNT;++i) {
            for(k=0; k<N_DIM;++k) {
                data[i*N_DIM+k]=data[i][k];
            }
        }
        for (i=0; i<N_K;++i) {
            for(k=0; k<N_DIM;++k) {
                cent[i*N_DIM+k]=cent[i][k];
            }
        }*/
        //SNK_INIT_LPARM(lparm,N_DCNT);
        cal_diskernel((float *)data,(float *)cent,table,N_K,N_DIM,N_DCNT,chpt,cent_c_ker,min_dis,lparm);
        for(i=0;i<N_DCNT;i++) {
            ch_pt+=chpt[i];
            ++cent_c[cent_c_ker[i]];
            t_sse+=min_dis[i];
            for(j=0;j<N_DIM;j++) {
                dis_k[table[i]][j]+=data[i][j];
            }
        }
        sse2=t_sse;


        //     sse2=update_table_cl(&ch_pt); /* step 4 - 更新對應表          */
    }while(iter<MAX_ITER && sse1!=sse2 && ch_pt>MIN_PT); // 收斂條件
    toc("HSA Execution Time", &timer_1, &timer_2);

    print_cent(); // 顯示最後重心位置
    printf("sse   = %.2lf\n", sse2);
    printf("ch_pt = %d\n", ch_pt);
    printf("iter = %d\n", iter);

}

