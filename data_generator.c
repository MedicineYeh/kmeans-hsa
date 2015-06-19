#include <stdio.h>
#include <stdlib.h>    
#include <string.h>
#include <time.h>

#define LOW      20  /* 資料下限   */
#define UP       300 /* 資料上限   */

int N_DCNT = 0, N_DIM = 0;
float **data = NULL;

void get_data()
{
    int i, j;

    data = (float **)malloc(sizeof(float *) * N_DCNT);
    for(i = 0; i < N_DCNT; i++) {
        data[i] = (float *)malloc(sizeof(float) * N_DIM);
        for(j = 0; j < N_DIM; j++) {
            data[i][j] = LOW + (float)rand()*(UP-LOW) / RAND_MAX;
        }
    }
}

void save_data(char *filename)
{
    int i, j;
    FILE *fp = fopen(filename, "wt");
    
    fprintf(fp, "%d,%d\n", N_DCNT, N_DIM);
    for(i = 0; i < N_DCNT ; i++) {
        for (j = 0; j < N_DIM; j++) {
            fprintf(fp, "%f,", data[i][j]);
        }
        fprintf(fp, "\n");
    }

    fclose(fp); 
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        printf("usage: %s filename DCNT DIM\n\n", argv[0]);
        return 1;
    }
    N_DCNT = atoi(argv[2]);
    N_DIM = atoi(argv[3]);

    printf("file name = %s\nDCNT = %d\nDIM = %d\n", argv[1], N_DCNT, N_DIM);
    srand((unsigned)time(NULL)); 
    get_data();                      /* step 0 - 取得資料            */
    save_data(argv[1]);

    return 0;
}



