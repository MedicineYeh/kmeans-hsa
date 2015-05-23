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

float update_table_c(int* ch_pt)
{
    int i, j, k, min_k;
    float dis, min_dis, t_sse=0.0;

    *ch_pt=0;                          // 變動點數設0
    memset(cent_c_c, 0, sizeof(cent_c)); // 各叢聚資料數清0
    memset(dis_k_c, 0, sizeof(dis_k));   // 各叢聚距離和清0

    for(i=0; i<DCNT; ++i){
        // 尋找所屬重心
        min_dis = cal_dis(data[i], centc[0], DIM);
        min_k   = 0;
        for(k=1;k<K; ++k){
            dis = cal_dis(data[i], centc[k], DIM);
            if(dis < min_dis) 
                min_dis=dis, min_k = k;
        }
        *ch_pt+=(table_c[i]!=min_k); // 更新變動點數
        table_c[i] = min_k;          // 更新所屬重心
        ++cent_c_c[min_k];           // 累計重心資料數         
        t_sse += min_dis;          // 累計總重心距離
        for(j=0; j<DIM; ++j)       // 更新各叢聚總距離
            dis_k_c[min_k][j]+=data[i][j];         
    }
    return t_sse;
}

// ------------------------------------
// 更新重心位置
void update_cent_c()
{
    int k, j;
    for(k=0; k<K; ++k)
        for(j=0; j<DIM; ++j)
            centc[k][j]=dis_k_c[k][j]/cent_c_c[k];
}

// ------------------------------------
// 顯示重心位置
void print_cent_c()
{
    int j, k;
    for(k=0; k<K; ++k) {
        for(j=0; j<DIM; ++j)
            printf("%6.2lf ", centc[k][j]);
        putchar('\n');
    }
}


